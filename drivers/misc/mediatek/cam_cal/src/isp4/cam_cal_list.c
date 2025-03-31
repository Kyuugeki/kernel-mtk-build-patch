// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
#include "cam_cal_list.h"
#include "eeprom_i2c_common_driver.h"
#include "eeprom_i2c_custom_driver.h"
#include "kd_imgsensor.h"

struct stCAM_CAL_LIST_STRUCT g_camCalList[] = {
	/*Below is commom sensor */
	{S5KJNSSQ_SENSOR_ID, 0xA0, Common_read_region},
	{S5K3P9SP_SENSOR_ID, 0xA2, Common_read_region},
	{SC202CS_SENSOR_ID, 0x6c, sc202cs_read_region},
	{S5K4H7_SENSOR_ID, 0x5A, s5k4h7sub_read_region},
	/*  ADD before this line */
	{0, 0, 0}       /*end of list */
};

unsigned int cam_cal_get_sensor_list(
	struct stCAM_CAL_LIST_STRUCT **ppCamcalList)
{
	if (ppCamcalList == NULL)
		return 1;

	*ppCamcalList = &g_camCalList[0];
	return 0;
}


