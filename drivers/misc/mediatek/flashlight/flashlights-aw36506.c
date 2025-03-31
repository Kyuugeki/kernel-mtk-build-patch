// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 awinic. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/of.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include "flashlight.h"
#include "flashlight-dt.h"
#include "flashlight-core.h"

/* device tree should be defined in flashlight-dt.h */
#ifndef AW36506_DTNAME
#define AW36506_DTNAME "awinic,aw36506"
#endif
#ifndef AW36506_DTNAME_I2C
#define AW36506_DTNAME_I2C "awinic,aw36506"
#endif
#define AW36506_NAME "aw36506"

#define AW36506_DRIVER_VERSION "V1.0.0"

#define AW36506_REG_BOOST_CONFIG	(0x07)
#define AW36506_BIT_SOFT_RST_MASK	(~(1<<7))
#define AW36506_BIT_SOFT_RST_ENABLE	(1<<7)
#define AW36506_BIT_SOFT_RST_DISABLE	(0<<7)

/* define registers */
#define AW36506_REG_CHIPID		(0x00)
#define AW36506_CHIPID			(0x30)
#define AW36506_REG_ENABLE		(0x01)
#define AW36506_MASK_ENABLE_LED1	(0x01)
#define AW36506_DISABLE			(0x00)
#define AW36506_ENABLE_LED1		(0x03)
#define AW36506_ENABLE_LED1_TORCH	(0x0B)
#define AW36506_REG_DEVICE_ID		(0x0C)
#define AW36506_DEVICE_ID		(0x0A)
#define AW36506_ENABLE_LED1_FLASH	(0x0F)
#define AW36506_REG_DUMMY		(0x09)

#define AW36506_REG_FLASH_LEVEL_LED1	(0x03)
#define AW36506_REG_TORCH_LEVEL_LED1	(0x05)

#define AW36506_REG_CTRL1		(0x31)
#define AW36506_REG_CTRL2		(0x69)
#define AW36506_REG_CHIP_VENDOR_ID	(0x25)
#define AW36506_CHIP_VENDOR_ID		(0x04)

#define AW36506_REG_TIMING_CONF		(0x08)
#define AW36506_TORCH_RAMP_TIME		(0x10)
#define AW36506_FLASH_TIMEOUT			(0x0A)
#define AW36506_CHIP_STANDBY			(0x80)

/* define channel, level */
#define AW36506_CHANNEL_NUM				1
#define AW36506_CHANNEL_CH1				0

#define AW36506_LEVEL_NUM				26
#define AW36506_LEVEL_TORCH				7

#define AW_I2C_RETRIES					5
#define AW_I2C_RETRY_DELAY				2

/* define mutex and work queue */
static DEFINE_MUTEX(aw36506_mutex);
static struct work_struct aw36506_work_ch1;

struct i2c_client *aw36506_flashlight_client;

/* define usage count */
static int use_count;

/* define i2c */
static struct i2c_client *aw36506_i2c_client;

/* platform data
 * torch_pin_enable: TX1/TORCH pin isa hardware TORCH enable
 * pam_sync_pin_enable: TX2 Mode The ENVM/TX2 is a PAM Sync. on input
 * thermal_comp_mode_enable: LEDI/NTC pin in Thermal Comparator Mode
 * strobe_pin_disable: STROBE Input disabled
 * vout_mode_enable: Voltage Out Mode enable
 */
struct aw36506_platform_data {
	u8 torch_pin_enable;
	u8 pam_sync_pin_enable;
	u8 thermal_comp_mode_enable;
	u8 strobe_pin_disable;
	u8 vout_mode_enable;
};

/* aw36506 chip data */
struct aw36506_chip_data {
	struct i2c_client *client;
	struct aw36506_platform_data *pdata;
	struct mutex lock;
	unsigned int chipID;
	unsigned int deviceID;
	bool is_mode_switch_init;
	u8 last_flag;
	u8 no_pdata;
};

/******************************************************************************
 * aw36506 operations
 *****************************************************************************/
static const unsigned char aw36506_torch_level[AW36506_LEVEL_NUM] = {
	0x06, 0x0F, 0x17, 0x1F, 0x27, 0x2F, 0x58, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const unsigned char aw36506_flash_level[AW36506_LEVEL_NUM] = {
	0x01, 0x0F, 0x17, 0x1F, 0x27, 0x2F, 0x37, 0x3F, 0x47, 0x4F,
    0x57, 0x5F, 0x67, 0x6F, 0x77, 0x7F, 0x87, 0x8F, 0x97, 0x9F,
    0xA7, 0xAF, 0xB7, 0xBF, 0xC7, 0xCF};

static unsigned char aw36506_reg_enable;
static int aw36506_level_ch1 = -1;

static int aw36506_is_torch(int level)
{
	if (level >= AW36506_LEVEL_TORCH)
		return -1;

	return 0;
}

static int aw36506_verify_level(int level)
{
	if (level < 0)
		level = 0;
	else if (level >= AW36506_LEVEL_NUM)
		level = AW36506_LEVEL_NUM - 1;

	return level;
}

/* i2c wrapper function */
static int aw36506_i2c_write(struct i2c_client *client, unsigned char reg, unsigned char val)
{
	int ret;
	unsigned char cnt = 0;

	while (cnt < AW_I2C_RETRIES) {
		ret = i2c_smbus_write_byte_data(client, reg, val);
		if (ret < 0) {
			pr_info("%s: i2c_write addr=0x%02X, data=0x%02X, cnt=%d, error=%d\n",
				   __func__, reg, val, cnt, ret);
		} else {
			break;
		}
		cnt++;
		msleep(AW_I2C_RETRY_DELAY);
	}

	return ret;
}

static int aw36506_i2c_read(struct i2c_client *client, unsigned char reg, unsigned char *val)
{
	int ret;
	unsigned char cnt = 0;

	while (cnt < AW_I2C_RETRIES) {
		ret = i2c_smbus_read_byte_data(client, reg);
		if (ret < 0) {
			pr_info("%s: i2c_read addr=0x%02X, cnt=%d, error=%d\n",
				   __func__, reg, cnt, ret);
		} else {
			*val = ret;
			break;
		}
		cnt++;
		msleep(AW_I2C_RETRY_DELAY);
	}

	return ret;
}

static void aw36506_mode_cfg_init(void)
{
	struct aw36506_chip_data *pdata = NULL;

	if (aw36506_i2c_client == NULL)
		printk("aw36506_i2c_client == NULL");

	pdata = aw36506_i2c_client->dev.driver_data;
	if (pdata->is_mode_switch_init) {
		aw36506_i2c_write(aw36506_i2c_client, AW36506_REG_CTRL2, 0x02);
		aw36506_i2c_write(aw36506_i2c_client, AW36506_REG_CTRL1, 0x0C);
	}
}

static void aw36506_soft_reset(void)
{
	unsigned char reg_val;

	aw36506_i2c_read(aw36506_i2c_client, AW36506_REG_BOOST_CONFIG, &reg_val);
	reg_val &= AW36506_BIT_SOFT_RST_MASK;
	reg_val |= AW36506_BIT_SOFT_RST_ENABLE;
	aw36506_i2c_write(aw36506_i2c_client, AW36506_REG_BOOST_CONFIG, reg_val);
	msleep(20);
}

/* flashlight enable function */
static int aw36506_enable_ch1(void)
{
	unsigned char reg, val;

	aw36506_mode_cfg_init();
	reg = AW36506_REG_ENABLE;
	if (!aw36506_is_torch(aw36506_level_ch1)) {
		/* torch mode */
		pr_info("%s enter torch mode set.\n", __func__);
		aw36506_reg_enable |= AW36506_ENABLE_LED1_TORCH;
	} else {
		/* flash mode */
		pr_info("%s enter flash mode set.\n", __func__);
		aw36506_reg_enable |= AW36506_ENABLE_LED1_FLASH;
	}

	val = aw36506_reg_enable;

	return aw36506_i2c_write(aw36506_i2c_client, reg, val);
}

/* flashlight disable function */
static int aw36506_disable_ch1(void)
{
	unsigned char reg, val;

	aw36506_mode_cfg_init();
	reg = AW36506_REG_ENABLE;
	aw36506_reg_enable &= (~AW36506_ENABLE_LED1_FLASH);
	val = aw36506_reg_enable;

	return aw36506_i2c_write(aw36506_i2c_client, reg, val);
}

static int aw36506_enable(int channel)
{
	if (channel == AW36506_CHANNEL_CH1)
		aw36506_enable_ch1();
	else {
		pr_err("%s, Error channel\n", __func__);
		return -1;
	}

	return 0;
}

static int aw36506_disable(int channel)
{
	if (channel == AW36506_CHANNEL_CH1)
		aw36506_disable_ch1();
	else {
		pr_err("Error channel\n");
		return -1;
	}

	return 0;
}

/* set flashlight level */
static int aw36506_set_level_ch1(int level)
{
	int ret;
	unsigned char reg, val;
	level = aw36506_verify_level(level);

	/* set torch brightness level */
	reg = AW36506_REG_TORCH_LEVEL_LED1;
	val = aw36506_torch_level[level];
	ret = aw36506_i2c_write(aw36506_i2c_client, reg, val);
	aw36506_level_ch1 = level;

	/* set flash brightness level */
	reg = AW36506_REG_FLASH_LEVEL_LED1;
	val = aw36506_flash_level[level];
	ret = aw36506_i2c_write(aw36506_i2c_client, reg, val);
	return ret;
}

static int aw36506_set_level(int channel, int level)
{
	if (channel == AW36506_CHANNEL_CH1)
		aw36506_set_level_ch1(level);
	else {
		pr_err("%s, Error channel\n", __func__);
		return -1;
	}

	return 0;
}
int aw36506_read_vendor_id(void)
{
	unsigned char val, reg_data;
	struct aw36506_chip_data *pdata = NULL;

	if (aw36506_i2c_client == NULL)
		printk("aw36506_i2c_client == NULL");

	usleep_range(2000, 2500);

	pdata = aw36506_i2c_client->dev.driver_data;
	/* read chip vendor information */
	aw36506_i2c_read(aw36506_i2c_client, AW36506_REG_CHIP_VENDOR_ID, &val);
	pr_info("aw36506 0x25 vendorID = 0x%2x\n", val);

	aw36506_i2c_read(aw36506_i2c_client, AW36506_REG_DUMMY, &reg_data);
	pr_info("aw36506 0x09 ver = 0x%2x\n", reg_data);

	if (((val & 0x04) == AW36506_CHIP_VENDOR_ID) && !(reg_data & 0x01)) {
		pr_info("aw36506 is_mode_switch_init is true\n");
		pdata->is_mode_switch_init = true;
	} else {
		pdata->is_mode_switch_init = false;
		pr_info("aw36506 is_mode_switch_init is false\n");
	}

	return 0;
}
/* flashlight init */
int aw36506_init(void)
{
	int ret;
	unsigned char reg, val;

	/* clear enable register */
	aw36506_mode_cfg_init();
	reg = AW36506_REG_ENABLE;
	val = AW36506_DISABLE;
	ret = aw36506_i2c_write(aw36506_i2c_client, reg, val);

	aw36506_reg_enable = val;

	/* set torch current ramp time and flash timeout */
	reg = AW36506_REG_TIMING_CONF;
	val = AW36506_TORCH_RAMP_TIME | AW36506_FLASH_TIMEOUT;
	ret = aw36506_i2c_write(aw36506_i2c_client, reg, val);

	return ret;
}

/* flashlight uninit */
int aw36506_uninit(void)
{
	aw36506_disable(AW36506_CHANNEL_CH1);
	return 0;
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer aw36506_timer_ch1;
static unsigned int aw36506_timeout_ms[AW36506_CHANNEL_NUM];

static void aw36506_work_disable_ch1(struct work_struct *data)
{
	pr_info("%s, ht work queue callback\n", __func__);

	aw36506_disable_ch1();
}

static enum hrtimer_restart aw36506_timer_func_ch1(struct hrtimer *timer)
{
	schedule_work(&aw36506_work_ch1);
	return HRTIMER_NORESTART;
}


int aw36506_timer_start(int channel, ktime_t ktime)
{
	if (channel == AW36506_CHANNEL_CH1)
		hrtimer_start(&aw36506_timer_ch1, ktime, HRTIMER_MODE_REL);
	else {
		pr_err("%s, Error channel\n", __func__);
		return -1;
	}

	return 0;
}

int aw36506_timer_cancel(int channel)
{
	if (channel == AW36506_CHANNEL_CH1)
		hrtimer_cancel(&aw36506_timer_ch1);
	else {
		pr_err("Error channel\n");
		return -1;
	}

	return 0;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int aw36506_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;

	/* verify channel */
	if (channel < 0 || channel >= AW36506_CHANNEL_NUM) {
		pr_err("%s, Failed with error channel\n", __func__);
		return -EINVAL;
	}

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		pr_err("%s, FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				__func__, channel, (int)fl_arg->arg);
		aw36506_timeout_ms[channel] = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		pr_err("%s. FLASH_IOC_SET_DUTY(%d): %d\n",
				__func__, channel, (int)fl_arg->arg);
		aw36506_set_level(channel, fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		pr_err("%s. FLASH_IOC_SET_ONOFF(%d): %d\n",
				__func__, channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (aw36506_timeout_ms[channel]) {
				ktime =
				ktime_set(aw36506_timeout_ms[channel] / 1000,
				(aw36506_timeout_ms[channel] % 1000) * 1000000);
				aw36506_timer_start(channel, ktime);
			}
			aw36506_enable(channel);
		} else {
			aw36506_disable(channel);
			aw36506_timer_cancel(channel);
		}
		break;

	default:
		pr_err("No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int aw36506_open(void)
{
	/* Actual behavior move to set driver function */
	/*since power saving issue */
	return 0;
}

static int aw36506_release(void)
{
	/* uninit chip and clear usage count */
	mutex_lock(&aw36506_mutex);
	use_count--;
	if (!use_count)
		aw36506_uninit();
	if (use_count < 0)
		use_count = 0;
	mutex_unlock(&aw36506_mutex);

	pr_info("Release: %d\n", use_count);

	return 0;
}

static int aw36506_set_driver(int set)
{
	/* init chip and set usage count */
	mutex_lock(&aw36506_mutex);
	if (!use_count)
		aw36506_init();
	use_count++;
	mutex_unlock(&aw36506_mutex);

	pr_info("Set driver: %d\n", use_count);

	return 0;
}

static ssize_t aw36506_strobe_store(struct flashlight_arg arg)
{
	aw36506_set_driver(1);
	aw36506_set_level(arg.ct, arg.level);
	aw36506_enable(arg.ct);
	msleep(arg.dur);
	aw36506_disable(arg.ct);
	aw36506_set_driver(0);

	return 0;
}

static struct flashlight_operations aw36506_ops = {
	aw36506_open,
	aw36506_release,
	aw36506_ioctl,
	aw36506_strobe_store,
	aw36506_set_driver
};

/******************************************************************************
 * I2C device and driver
 *****************************************************************************/
static int aw36506_chip_init(struct aw36506_chip_data *chip)
{
	/* NOTE: Chip initialication move to
	 *"set driver" operation for power saving issue.
	 */
	int ret = 0;

	ret = aw36506_init();

	return ret;
}

/***************************************************************************/
/*AW36506 Debug file */
/***************************************************************************/
static ssize_t reg_show(struct device *cd,
		struct device_attribute *attr, char *buf)
{
	unsigned char reg_val;
	unsigned char i;
	ssize_t len = 0;

	for (i = 0; i < 0x0E; i++) {
		aw36506_i2c_read(aw36506_i2c_client, i, &reg_val);
		len += snprintf(buf+len, PAGE_SIZE-len,
			"reg0x%2X = 0x%2X\n", i, reg_val);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\r\n");
	return len;
}

static ssize_t reg_store(struct device *cd,
		struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned int databuf[2];

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2)
		aw36506_i2c_write(aw36506_i2c_client, databuf[0], databuf[1]);
	return len;
}

static DEVICE_ATTR_RW(reg);

static int aw36506_create_sysfs(struct i2c_client *client)
{
	int err;
	struct device *dev = &(client->dev);

	err = device_create_file(dev, &dev_attr_reg);

	return err;
}

static int
aw36506_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct aw36506_chip_data *chip;
	struct aw36506_platform_data *pdata = client->dev.platform_data;
	int err;

	pr_info("%s Probe start.\n", __func__);

	/* check i2c */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s, Failed to check i2c functionality.\n", __func__);
		err = -ENODEV;
		goto err_out;
	}

	/* init chip private data */
	chip = kzalloc(sizeof(struct aw36506_chip_data), GFP_KERNEL);
	if (!chip) {
		err = -ENOMEM;
		goto err_out;
	}
	chip->client = client;

	/* init platform data */
	if (!pdata) {
		pr_err("%s, Platform data does not exist\n", __func__);
		pdata =
		kzalloc(sizeof(struct aw36506_platform_data), GFP_KERNEL);
		if (!pdata) {
			pr_err("%s, Failed to kzalloc memory.\n", __func__);
			err = -ENOMEM;
			goto err_init_pdata;
		}
		chip->no_pdata = 1;
	}
	chip->pdata = pdata;
	i2c_set_clientdata(client, chip);
	aw36506_i2c_client = client;

	/* init mutex and spinlock */
	mutex_init(&chip->lock);

	/* read 0x25 vendor id */
	aw36506_read_vendor_id();

	/* soft rst */
	aw36506_soft_reset();

	/* init work queue */
	INIT_WORK(&aw36506_work_ch1, aw36506_work_disable_ch1);

	/* init timer */
	hrtimer_init(&aw36506_timer_ch1, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	aw36506_timer_ch1.function = aw36506_timer_func_ch1;
	aw36506_timeout_ms[AW36506_CHANNEL_CH1] = 100;

	/* init chip hw */
	aw36506_chip_init(chip);

	/* register flashlight operations */
	if (flashlight_dev_register(AW36506_NAME, &aw36506_ops)) {
		pr_err("%s,Failed to register flashlight device.\n", __func__);
		err = -EFAULT;
		goto err_free;
	}

	/* clear usage count */
	use_count = 0;

	aw36506_create_sysfs(client);

	pr_info("%s Probe done.\n", __func__);

	return 0;

err_free:
	kfree(chip->pdata);
err_init_pdata:
	i2c_set_clientdata(client, NULL);
	kfree(chip);
err_out:
	return err;
}

static int aw36506_i2c_remove(struct i2c_client *client)
{
	struct aw36506_chip_data *chip = i2c_get_clientdata(client);

	pr_info("Remove start.\n");

	/* flush work queue */
	flush_work(&aw36506_work_ch1);

	/* unregister flashlight operations */
	flashlight_dev_unregister(AW36506_NAME);

	/* free resource */
	if (chip->no_pdata)
		kfree(chip->pdata);
	kfree(chip);

	pr_info("Remove done.\n");

	return 0;
}

static void aw36506_i2c_shutdown(struct i2c_client *client)
{
	pr_info("aw36506 shutdown start.\n");

	aw36506_mode_cfg_init();

	aw36506_i2c_write(aw36506_i2c_client, AW36506_REG_ENABLE,
						AW36506_CHIP_STANDBY);

	pr_info("aw36506 shutdown done.\n");
}


static const struct i2c_device_id aw36506_i2c_id[] = {
	{AW36506_NAME, 0},
	{}
};

#ifdef CONFIG_OF
static const struct of_device_id aw36506_i2c_of_match[] = {
	{.compatible = AW36506_DTNAME_I2C},
	{},
};
#endif

static struct i2c_driver aw36506_i2c_driver = {
	.driver = {
		   .name = AW36506_NAME,
#ifdef CONFIG_OF
		   .of_match_table = aw36506_i2c_of_match,
#endif
		   },
	.probe = aw36506_i2c_probe,
	.remove = aw36506_i2c_remove,
	.shutdown = aw36506_i2c_shutdown,
	.id_table = aw36506_i2c_id,
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int aw36506_probe(struct platform_device *dev)
{
	pr_info("%s Probe start.\n", __func__);

	if (i2c_add_driver(&aw36506_i2c_driver)) {
		pr_err("Failed to add i2c driver.\n");
		return -1;
	}

	pr_info("%s Probe done.\n", __func__);

	return 0;
}

static int aw36506_remove(struct platform_device *dev)
{
	pr_info("Remove start.\n");

	i2c_del_driver(&aw36506_i2c_driver);

	pr_info("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id aw36506_of_match[] = {
	{.compatible = AW36506_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, aw36506_of_match);
#else
static struct platform_device aw36506_platform_device[] = {
	{
		.name = AW36506_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, aw36506_platform_device);
#endif

static struct platform_driver aw36506_platform_driver = {
	.probe = aw36506_probe,
	.remove = aw36506_remove,
	.driver = {
		.name = AW36506_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = aw36506_of_match,
#endif
	},
};

static int __init flashlight_aw36506_init(void)
{
	int ret;

	pr_info("%s driver version %s.\n", __func__, AW36506_DRIVER_VERSION);

#ifndef CONFIG_OF
	ret = platform_device_register(&aw36506_platform_device);
	if (ret) {
		pr_err("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&aw36506_platform_driver);
	if (ret) {
		pr_err("Failed to register platform driver\n");
		return ret;
	}

	pr_info("flashlight_aw36506 Init done.\n");

	return 0;
}

static void __exit flashlight_aw36506_exit(void)
{
	pr_info("flashlight_aw36506-Exit start.\n");

	platform_driver_unregister(&aw36506_platform_driver);

	pr_info("flashlight_aw36506 Exit done.\n");
}

module_init(flashlight_aw36506_init);
module_exit(flashlight_aw36506_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Awinic");
MODULE_DESCRIPTION("AW36506 Flashlight Driver");
