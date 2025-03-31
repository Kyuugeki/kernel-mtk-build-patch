/*
 * *  ffu.c
 */

#include <linux/bug.h>
#include <linux/errno.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
//#include <linux/mmc/ffu.h>
#include <linux/slab.h>

#include <linux/swap.h>
#include <linux/scatterlist.h>
#include <linux/mmc/ioctl.h>
#include <linux/uaccess.h>
#include <linux/reboot.h>

#include "core.h"
#include "mmc_ops.h"


#define CONFIG_ENABLE_FFU_SOFTRESET

#if IS_ENABLED(CONFIG_FACTORY_BUILD)
#define MY_FFU_Enable 1
#else
#define MY_FFU_Enable 0
#endif

#if MY_FFU_Enable

#define EXT_CSD_FFU					(1<<0)
#define EXT_CSD_CMD_SET_NORMAL		(1<<0)
#define EXT_CSD_UPDATE_DISABLE		(1<<0)
#define EXT_CSD_FFU_MODE			(0x01)
#define EXT_CSD_NORMAL_MODE			(0x00)

#define EXT_CSD_REV_V5_1		8
#define EXT_CSD_REV_V5_0		7

#define EXT_CSD_CMDQ_MODE_EN            15  /* R/W */
#define EXT_CSD_FFU_STATUS              26  /* RO */
#define EXT_CSD_FFU_ERROR_NUMBER		64	/* R */

#define EXT_CSD_SUPPORTED_MODES		493	/* RO */
#define EXT_CSD_FFU_FEATURES		492	/* RO */

#define EXT_CSD_FFU_ARG_3		490	/* RO */
#define EXT_CSD_FFU_ARG_2		489	/* RO */
#define EXT_CSD_FFU_ARG_1		488	/* RO */
#define EXT_CSD_FFU_ARG_0		487	/* RO */

#define EXT_CSD_NUM_OF_FW_SEC_PROG_3	305	/* RO */
#define EXT_CSD_NUM_OF_FW_SEC_PROG_2	304	/* RO */
#define EXT_CSD_NUM_OF_FW_SEC_PROG_1	303	/* RO */
#define EXT_CSD_NUM_OF_FW_SEC_PROG_0	302	/* RO */

#define MMC_STATE_FFUED  (1<<22)  /* card has been FFUed */

#define EXT_CSD_MODE_CONFIG		30
#define MAX_STEP_SIZE   (64 * 1024) // sizeof(g_ffu_bin_buffer)

#define err_printf 	pr_err
#define info_printf pr_info

unsigned char g_ffu_bin_buffer[] = {
	#include "EMMC128-TY29-GA5E_FFU.txt"
};
extern int mmc_send_ext_csd(struct mmc_card *card, u8 *ext_csd);
extern int mmc_go_idle(struct mmc_host *host);
extern int mmc_init_card_ffu(struct mmc_host *host, u32 ocr,
	struct mmc_card *oldcard);

#if 0
static int mmc_stop_transmission(struct mmc_card *card)
{
	struct mmc_request mrq = {0};
	struct mmc_command cmd = {0};

	mrq.cmd = &cmd;

	cmd.opcode = MMC_STOP_TRANSMISSION;
	cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;

	mmc_wait_for_req(card->host, &mrq);

	if (cmd.error)
		return cmd.error;

	return 0;

}
#endif

static int mmc_set_blockcount_FFU(struct mmc_card *card, unsigned int blockcount,
			bool is_rel_write)
{
	struct mmc_command cmd = {0};

	cmd.opcode = MMC_SET_BLOCK_COUNT;
	cmd.arg = blockcount & 0x0000FFFF;
	if (is_rel_write)
		cmd.arg |= 1 << 31;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;
	return mmc_wait_for_cmd(card->host, &cmd, 5);
}


static int mmc_write_blk(struct mmc_card *card, u32 lba, u16 blks, void *pbuf, u32 len)
{
	int err = 0;

	struct mmc_request mrq = {0};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct mmc_command stop = {0};
	struct scatterlist sg = {0};
	u8 *__buf = NULL;

	__buf = kzalloc(len, GFP_KERNEL);
	if (!__buf)
		return -ENOMEM;

	memcpy(__buf, pbuf, len);
	sg_init_one(&sg, __buf, len);

	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	mrq.cmd = &cmd;
	mrq.data = &data;

#if 0
	if (blks > 1) {
		mrq.stop = &stop;
		mrq.stop->opcode = MMC_STOP_TRANSMISSION;
		mrq.stop->arg = 0;
		mrq.stop->flags = MMC_RSP_R1B | MMC_CMD_AC;
	}
#else
	mrq.stop = NULL;

	err = mmc_set_blockcount_FFU(card, blks, 0);
	if(err)
		pr_err("## [mmc_set_blockcount_FFU] err=%d\n" , err);
#endif

	cmd.opcode = (blks > 1 ? MMC_WRITE_MULTIPLE_BLOCK : MMC_WRITE_BLOCK);
	cmd.arg = lba;

	if (!mmc_card_is_blockaddr(card))
		cmd.arg <<= 9;

	data.blksz = 512;
	data.blocks = blks;
	data.flags = MMC_DATA_WRITE;
	data.sg = &sg;
	data.sg_len = 1;

	mmc_set_data_timeout(&data, card);
	mmc_wait_for_req(card->host, &mrq);

	if (cmd.error) {
		err = cmd.error;
		goto out;
	}

	if (data.error) {
		err = data.error;
		goto out;
	}

	if (blks > 1 && stop.error) {
		err = stop.error;
		goto out;
	}

out:

	if (__buf) kfree(__buf);

	printk("[%s] ret: %s, err: %d,  lba: %u, blk: %hu, ptr: %p, len: %u\n",
		__func__,
		err == 0 ? "ok" : "failed",
		err,
		lba,
		blks,
		pbuf,
		len);

	return err;
}

static int mmc_read_ext_csd(struct mmc_card *card, u8 *ext_csd)
{
	u8 *__ext_csd = NULL;
	int err = mmc_get_ext_csd(card, &__ext_csd);
	if (0 == err) {
		memcpy(ext_csd, __ext_csd, 512);
		kfree(__ext_csd);
	}
	return err;
}


int MY_card_FFU_update(struct mmc_card *card)
{
	int err = 0;
	u8 ext_csd[512] = {0};
	u8 *fw_data;
	u8 cmdq_enable = 0;
	u32 fw_size;
	u32 arg;
	u32 sect_size;
	u32 sect_left;
	u32 sect_step;
	u32 sect_done;
	// u32 cid[4];
	u8 old_fwrev[8];
#ifdef CONFIG_ENABLE_FFU_SOFTRESET
	u8 new_fwrev[8];
#endif
	// u8 tag_fwrev[8] = { 0, 0, 0, 0 }; // todo: the old firmware version you want to upgrade
	// s8 tag_pnm[6] = {'X', 'X', 'X', 'X', 'X', 'X'}; // todo: the pnm to identify the emmc
	u8 tag_fwrev[8] = { 0x5e, 0x00, 0x00, 0x00 };	// Old {0x5b, 0x00, 0x04, 0x00}
	s8 tag_pnm[6] = {'Y', '2', '9', '1', '2', '8'}; // todo: the pnm to identify the emmc

	printk("cid pnm: %s\n", card->cid.prod_name);

    if (0 != memcmp(tag_pnm, card->cid.prod_name, sizeof(tag_pnm))) {
        info_printf("this emmc does not need to be upgraded.\n");
        err = 0;
        goto out;
    }

	printk("MY_card_FFU_update enter\n");

	err = mmc_read_ext_csd(card, ext_csd);
	if (err) {
		pr_err("FFU: %s: error %d sending ext_csd\n",
			mmc_hostname(card->host), err);
		return 1;
	}

	cmdq_enable = ext_csd[EXT_CSD_CMDQ_MODE_EN];
	printk("CMDQ Mode: %2x\n", cmdq_enable);
	printk("Into FFU ext_csd[254]=0x%x\n", ext_csd[254]);

        memcpy(old_fwrev, ext_csd + 254, 8);
	err_printf("old fwrev: %02X %02X %02X %02X\n", old_fwrev[0], old_fwrev[1], old_fwrev[2], old_fwrev[3]);

	printk("oldfw=%lx, tagfw=%lx\n", *(u32 *)old_fwrev, *(u32 *)tag_fwrev);

    if (*(u32 *)old_fwrev >= *(u32 *)tag_fwrev) {
        info_printf("this firmware does not need to be upgraded.\n");
        err = 0;
        goto out;
    }

	if (ext_csd[EXT_CSD_REV] < EXT_CSD_REV_V5_0) {
		err_printf("The FFU feature is only available on devices >= "
			"MMC 5.0, not supported in this device\n");
		err = -1;
		goto out;
	}

	if (!(ext_csd[EXT_CSD_SUPPORTED_MODES] & EXT_CSD_FFU)) {
		err_printf("FFU is not supported in this device\n");
		err = -1;
		goto out;
	}

	if (ext_csd[EXT_CSD_FW_CONFIG] & EXT_CSD_UPDATE_DISABLE) {
		err_printf("Firmware update was disabled in this device\n");
		err = -1;
		goto out;
	}

	if (cmdq_enable) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_CMDQ_MODE_EN, 0, card->ext_csd.generic_cmd6_time);
		if (err) {
			err_printf("failed to turn off cmdq mode, errno: %d\n", err);
			goto out;
		}
	}

	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_MODE_CONFIG, EXT_CSD_FFU_MODE, card->ext_csd.generic_cmd6_time);
	if (err) {
		err_printf("failed to switch ffu mode, errno: %d\n", err);
		goto out;
	}

	fw_size = sizeof(g_ffu_bin_buffer);
	fw_data = g_ffu_bin_buffer;
	sect_size = (ext_csd[EXT_CSD_DATA_SECTOR_SIZE] == 0) ? 512 : 4096;

	printk("##Start send FFU Bin:%d, %d\n", fw_size, sect_size);
	/* set CMD ARG */
	arg = ext_csd[EXT_CSD_FFU_ARG_0] |
		ext_csd[EXT_CSD_FFU_ARG_1] << 8 |
		ext_csd[EXT_CSD_FFU_ARG_2] << 16 |
		ext_csd[EXT_CSD_FFU_ARG_3] << 24;

	for (sect_left = fw_size / sect_size, sect_step = MAX_STEP_SIZE / sect_size;
		 sect_left != 0;
		 sect_left -= sect_step, fw_data += sect_step * sect_size) {

			if (sect_left < sect_step) sect_step = sect_left;

			err = mmc_write_blk(card, arg, sect_step, fw_data, sect_step * sect_size);
			if (0 != err) {
				err_printf("failed to write firmware data, errno: %d\n", err);
				goto out2;
			}

			err = mmc_read_ext_csd(card, ext_csd);
			if (0 != err){
				err_printf("failed to read ext csd from mmc, errno: %d\n", err);
				goto out2;
			}

			/* Test if we need to restart the download */
			sect_done = ext_csd[EXT_CSD_NUM_OF_FW_SEC_PROG_0] |
				ext_csd[EXT_CSD_NUM_OF_FW_SEC_PROG_1] << 8 |
				ext_csd[EXT_CSD_NUM_OF_FW_SEC_PROG_2] << 16 |
				ext_csd[EXT_CSD_NUM_OF_FW_SEC_PROG_3] << 24;

			info_printf("FFU installing: %u/%u\n", sect_done, fw_size / sect_size);
	}

	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_MODE_CONFIG, EXT_CSD_NORMAL_MODE, card->ext_csd.generic_cmd6_time);
	if (err) {
		err_printf("failed to switch normal mode, errno: %d\n", err);
		goto out;
	}

	if (cmdq_enable) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_CMDQ_MODE_EN, cmdq_enable, card->ext_csd.generic_cmd6_time);
		if (err) {
			err_printf("failed to turn on cmdq mode, errno: %d\n", err);
			goto out;
		}
	}

	err = mmc_read_ext_csd(card, ext_csd);
	if (err){
		err_printf("failed to read ext csd from mmc, errno: %d\n", err);
		goto out;
	}

	if (ext_csd[EXT_CSD_FFU_STATUS]) {
		err = -1;
		err_printf("FFU result %02X error %02X during FFU install\n",
			ext_csd[EXT_CSD_FFU_STATUS], ext_csd[EXT_CSD_FFU_ERROR_NUMBER]);
		goto out;
	}


#ifdef CONFIG_ENABLE_FFU_SOFTRESET
	do {
#if 0
		u32 max_dtr;

 		max_dtr = card->ext_csd.hs_max_dtr;
		// set host clk to 260kHz
		mmc_set_clock(card->host, 260*1000);

		// soft reset to take effect ffu
		err = mmc_init_card_ffu(card->host, card->ocr, card);

		// resume host clk
		mmc_set_clock(card->host, max_dtr);
#else
		struct mmc_host *host = card->host;

		host->ios.timing = MMC_TIMING_LEGACY;
		mmc_set_clock(card->host, 400000);
		mmc_set_bus_width(card->host, MMC_BUS_WIDTH_1);

		card->state |= MMC_STATE_FFUED;	// MMC_QUIRK_FFUED

		err = mmc_init_card_ffu(card->host, card->ocr, card);
		pr_notice("FFU: mmc_init_card_ffu ret %d\n", err);
		if (!err)
			card->state &= ~MMC_STATE_FFUED;	// MMC_QUIRK_FFUED
#endif
        if (err) {
            err_printf("failed to reset emmc, errno: %d\n", err);
            goto out;
        }

        err = mmc_read_ext_csd(card, ext_csd);
        if (err){
            err_printf("failed to read ext csd from mmc, errno: %d\n", err);
            goto out;
        }

        memcpy(new_fwrev, ext_csd + 254, 8);
        if (0 == memcmp(old_fwrev, new_fwrev, 4)) {
            err = -EPERM;
            err_printf("firmware revision does not change\n");
            err_printf("old fwrev: %02X %02X %02X %02X\n", old_fwrev[0], old_fwrev[1], old_fwrev[2], old_fwrev[3]);
            err_printf("new fwrev: %02X %02X %02X %02X\n", new_fwrev[0], new_fwrev[1], new_fwrev[2], new_fwrev[3]);
            goto out;
        }

        err_printf("new fwrev: %02X %02X %02X %02X\n", new_fwrev[0], new_fwrev[1], new_fwrev[2], new_fwrev[3]);

    } while(0);

#endif
	info_printf("FFU finished successfully\n");

out:
	printk("##MY_card_FFU_update END:%d\n", err);
	return err;

out2:
	// Exit FFU Mode
	err_printf("FFU failed to write data\n");
	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_MODE_CONFIG, EXT_CSD_NORMAL_MODE, card->ext_csd.generic_cmd6_time);
	if (err) {
		err_printf("failed to switch normal mode, errno: %d\n", err);
		// return err;
	}

	if (cmdq_enable) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_CMDQ_MODE_EN, cmdq_enable, card->ext_csd.generic_cmd6_time);
		if (err) {
			err_printf("failed to turn on cmdq mode, errno: %d\n", err);
			// return err;
		}
	}
	return err;
}
EXPORT_SYMBOL(MY_card_FFU_update);

#endif
