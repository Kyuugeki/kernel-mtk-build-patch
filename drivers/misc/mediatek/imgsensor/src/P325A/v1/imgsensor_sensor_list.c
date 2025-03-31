// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "kd_imgsensor.h"
#include "imgsensor_sensor_list.h"

/* Add Sensor Init function here
 * Note:
 * 1. Add by the resolution from ""large to small"", due to large sensor
 *    will be possible to be main sensor.
 *    This can avoid I2C error during searching sensor.
 * 2. This should be the same as
 *     mediatek\custom\common\hal\imgsensor\src\sensorlist.cpp
 */
struct IMGSENSOR_INIT_FUNC_LIST kdSensorList[MAX_NUM_OF_SUPPORT_SENSOR] = {
#if defined(S5KJNSSQ_MIPI_RAW)
	{S5KJNSSQ_SENSOR_ID,
	SENSOR_DRVNAME_S5KJNSSQ_MIPI_RAW,
	S5KJNSSQ_MIPI_RAW_SensorInit},
#endif
#if defined(S5K4H7_MIPI_RAW)
	{S5K4H7_SENSOR_ID,
	SENSOR_DRVNAME_S5K4H7_MIPI_RAW,
	S5K4H7_MIPI_RAW_SensorInit},
#endif
#if defined(S5K3P9SP_MIPI_RAW)
	{S5K3P9SP_SENSOR_ID,
	SENSOR_DRVNAME_S5K3P9SP_MIPI_RAW,
	S5K3P9SP_MIPI_RAW_SensorInit},
#endif
#if defined(SC202CS_MIPI_RAW)
	{SC202CS_SENSOR_ID,
	SENSOR_DRVNAME_SC202CS_MIPI_RAW,
	SC202CS_MIPI_RAW_SensorInit},
#endif
	/*  ADD sensor driver before this line */
	{0, {0}, NULL}, /* end of list */
};
/* e_add new sensor driver here */

