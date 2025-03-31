# SPDX-License-Identifier: GPL-2.0
# Copyright (C) 2020 MediaTek Inc.

IMGSENSOR_DRIVER_OBJ += \
        $(subdrv-rpath)/s5kjnssqmipiraw_Sensor.o

ifeq ($(CONFIG_MTK_IMGSENSOR_ISP4), m)
obj-$(CONFIG_MTK_IMGSENSOR_ISP4) += $(MTK_PLATFORM_KO).o
$(eval $(MTK_PLATFORM_KO)-objs += $(IMGSENSOR_DRIVER_OBJ))
else ifeq ($(CONFIG_MTK_IMGSENSOR_ISP4), y)
obj-$(CONFIG_MTK_IMGSENSOR_ISP4) += $(IMGSENSOR_DRIVER_OBJ)
endif
