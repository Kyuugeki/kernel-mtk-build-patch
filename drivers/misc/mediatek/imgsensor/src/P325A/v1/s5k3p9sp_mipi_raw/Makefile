# SPDX-License-Identifier: GPL-2.0

IMGSENSOR_DRIVER_OBJ += \
        $(subdrv-rpath)/s5k3p9spmipiraw_Sensor.o

ifeq ($(CONFIG_MTK_IMGSENSOR_ISP4), m)
obj-$(CONFIG_MTK_IMGSENSOR_ISP4) += $(MTK_PLATFORM_KO).o
$(eval $(MTK_PLATFORM_KO)-objs += $(IMGSENSOR_DRIVER_OBJ))
else ifeq ($(CONFIG_MTK_IMGSENSOR_ISP4), y)
obj-$(CONFIG_MTK_IMGSENSOR_ISP4) += $(IMGSENSOR_DRIVER_OBJ)
endif
