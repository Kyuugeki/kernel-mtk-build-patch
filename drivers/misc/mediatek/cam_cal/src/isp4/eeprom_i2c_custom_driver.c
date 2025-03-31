// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#define PFX "CAM_CAL"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/of.h>
#include "cam_cal.h"
#include "cam_cal_define.h"
#include "cam_cal_list.h"
#include <linux/dma-mapping.h>
#if IS_ENABLED(CONFIG_COMPAT)
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

#define EEPROM_I2C_MSG_SIZE_READ 2

static DEFINE_SPINLOCK(g_spinLock);
static struct i2c_client *g_pstI2CclientG;
extern struct sc202cs_otp_t  sc202cs_otp_info;

struct sc202cs_otp_t {
    uint8_t dataFlag;
    uint8_t awb_golden_param[3];
    uint8_t awb_param[3];
    uint8_t dataChksum;
};

extern struct s5k4h7_otp_t  s5k4h7_otp_info;
struct s5k4h7_otp_t {
	unsigned char page_flag;
	unsigned char module_id[2];
	unsigned char awb_gloden[8];
	unsigned char awb_unint[8];
	unsigned char lscdata[1871];
	unsigned char lscCheckSum;
	unsigned char serial_number[8];
};
/************************************************************
 * I2C read function (Custom)
 * Customer's driver can put on here
 * Below is an example
 ************************************************************/
 #define PAGE_SIZE_ 256
static int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData,
		u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId)
{
	int  i4RetValue = 0;
	struct i2c_msg msg[EEPROM_I2C_MSG_SIZE_READ];

	spin_lock(&g_spinLock);
	g_pstI2CclientG->addr = (i2cId >> 1);
	spin_unlock(&g_spinLock);

	msg[0].addr = g_pstI2CclientG->addr;
	msg[0].flags = g_pstI2CclientG->flags & I2C_M_TEN;
	msg[0].len = a_sizeSendData;
	msg[0].buf = a_pSendData;

	msg[1].addr = g_pstI2CclientG->addr;
	msg[1].flags = g_pstI2CclientG->flags & I2C_M_TEN;
	msg[1].flags |= I2C_M_RD;
	msg[1].len = a_sizeRecvData;
	msg[1].buf = a_pRecvData;

	i4RetValue = i2c_transfer(g_pstI2CclientG->adapter,
				msg,
				EEPROM_I2C_MSG_SIZE_READ);

	if (i4RetValue != EEPROM_I2C_MSG_SIZE_READ) {
		pr_debug("I2C read failed!!\n");
		return -1;
	}
	return 0;
}

unsigned int s5k4h7sub_read_region(struct i2c_client *client,unsigned int addr,unsigned char *data, unsigned int size)
{
    unsigned char *dataTmp = data;

    pr_err("s5k4h7otp region addr = 0x%x, size = %d\n", addr, size);
	if (addr == 0x0 && size == 1) {//0xff
		*(u32 *)data = 0x00000008;
	} else if (addr == 0x0 && size == 1888) {
        unsigned int totalSize = sizeof(s5k4h7_otp_info.page_flag)   +
                                 sizeof(s5k4h7_otp_info.module_id)   +
                                 sizeof(s5k4h7_otp_info.awb_gloden)  +
                                 sizeof(s5k4h7_otp_info.awb_unint)   +
                                 sizeof(s5k4h7_otp_info.lscdata) - 3 +
                                 sizeof(s5k4h7_otp_info.lscCheckSum);
        pr_err("s5k4h7otp region addr = 0x%x, size = %d  totalSize=%d \n", addr, size, totalSize);
        if (size == totalSize) {
            data[0] = 0x8;

            dataTmp += sizeof(s5k4h7_otp_info.page_flag);
            memcpy(dataTmp, s5k4h7_otp_info.module_id, sizeof(s5k4h7_otp_info.module_id));

            dataTmp += sizeof(s5k4h7_otp_info.module_id);
            memcpy(dataTmp, s5k4h7_otp_info.awb_gloden, sizeof(s5k4h7_otp_info.awb_gloden));

            dataTmp += sizeof(s5k4h7_otp_info.awb_gloden);
            memcpy(dataTmp, s5k4h7_otp_info.awb_unint, sizeof(s5k4h7_otp_info.awb_unint));

            dataTmp += sizeof(s5k4h7_otp_info.awb_unint);
            memcpy(dataTmp, s5k4h7_otp_info.lscdata + 1, sizeof(s5k4h7_otp_info.lscdata) - 3);

            data[totalSize - 1] = s5k4h7_otp_info.lscCheckSum;
        } else {
			pr_err("s5k4h7otp size != totalSize");
            size = totalSize;
        }
	} else if (addr == 0x02 && size == 2) { //read module id  0x80
        memcpy(data, s5k4h7_otp_info.module_id, size);
        pr_err("s5k4h7otp add = 0x%x,read module id\n",addr);
    } else if (size == 8) { //read single awb data
        if (addr == 3) {//0x10
            memcpy(data, (s5k4h7_otp_info.awb_gloden), size);
            pr_err("add = 0x%x, read golden\n",addr);
        } else if (addr == 11){ //0x01
            memcpy(data,(s5k4h7_otp_info.awb_unint), size);
            pr_err("add = 0x%x, read awb_unint\n",addr);
        }
    } else if (size >=1868 && size < 2048 && addr == 19) {//0x16
			memcpy(data, (s5k4h7_otp_info.lscdata + 1), size);
	} else if (addr == 1887 && size == 1) {//0x20
		*(u32 *)data = s5k4h7_otp_info.lscCheckSum;
	} else{
        pr_err("s5k4h7otp add = 0x%x, size = %d ,read error !!!\n",addr,size);
    }
	return size;
}

static int custom_read_region(u32 addr, u8 *data, u16 i2c_id, u32 size)
{
	u8 *buff = data;
	u32 size_to_read = size;

	int ret = 0;

	while (size_to_read > 0) {
		u8 page = addr / PAGE_SIZE_;
		u8 offset = addr % PAGE_SIZE_;
		char *Buff = data;

		if (iReadRegI2C(&offset, 1, (u8 *)Buff, 1,
			i2c_id + (page << 1)) < 0) {
			pr_debug("fail addr=0x%x 0x%x, P=%d, offset=0x%x",
				addr, *Buff, page, offset);
			break;
		}
		addr++;
		buff++;
		size_to_read--;
		ret++;
	}
	pr_debug("addr =%x size %d data read = %d\n", addr, size, ret);
	return ret;
}

unsigned int sc202cs_read_region(struct i2c_client *client, unsigned int addr,
    unsigned char *data, unsigned int size)
{
    int i=0;

    pr_err("sc202csotp addr =%x size %d\n", addr, size);
    if (addr == 0x0 && 1 == size) {//0x8008
        *(u32 *)data = 0x0000000ca;
        pr_err("sc202csotp addr =%x data = %x  size = %x\n", addr, *(u32 *)data, size);
    } else if (addr == 0x0 && 8 == size) {
        pr_err("sc202csotp get all otp data");
        for (i = 0; i < size; i++) {
            if ( 0 == i) {
                data[i] = 0x0000000ca;//202
            } else if (i > 0 && i <= 3) {
                data[i] = sc202cs_otp_info.awb_golden_param[i - 1];
            } else if (i >3 && i <= 6) {
                data[i] = sc202cs_otp_info.awb_param[i - 4];
            } else if (i == 7 ) {
                data[i] = sc202cs_otp_info.dataChksum;
            }
        }
    } else if (addr == 0x01 && 3 == size) {//0x8009
        // awb golden info
        pr_err("sc202csotp awb golden flag valid");
        for (i = 0; i < size; i++) {
            data[i] = sc202cs_otp_info.awb_golden_param[i];
        }
    } else if (addr == 0x04 && 3 == size) {//0x8011
        // awb info
        pr_err("sc202csotp awb flag valid");
        for (i = 0; i < size; i++) {
            data[i] = sc202cs_otp_info.awb_param[i];
        }
    } else if (addr == 0x8015) {
        *(u32 *)data = sc202cs_otp_info.dataChksum;
        pr_err("sc202csotp  awbChksum %d", sc202cs_otp_info.dataChksum);
    } else{
        pr_err("sc202csotp add = 0x%x, size = %d ,read error !!!\n", addr, size);
    }
    return size;
}

unsigned int Custom_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
	g_pstI2CclientG = client;
	if (custom_read_region(addr, data, g_pstI2CclientG->addr, size) == 0)
		return size;
	else
		return 0;
}


