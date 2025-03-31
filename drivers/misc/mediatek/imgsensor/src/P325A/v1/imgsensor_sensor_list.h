/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __KD_SENSORLIST_H__
#define __KD_SENSORLIST_H__

#include "kd_camera_typedef.h"
#include "imgsensor_sensor.h"

struct IMGSENSOR_INIT_FUNC_LIST {
	MUINT32   id;
	MUINT8    name[32];
	MUINT32 (*init)(struct SENSOR_FUNCTION_STRUCT **pfFunc);
};

UINT32 S5KJNSSQ_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc);
UINT32 S5K4H7_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc);
UINT32 S5K3P9SP_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc);
UINT32 SC202CS_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc);
extern struct IMGSENSOR_INIT_FUNC_LIST kdSensorList[];

#endif

