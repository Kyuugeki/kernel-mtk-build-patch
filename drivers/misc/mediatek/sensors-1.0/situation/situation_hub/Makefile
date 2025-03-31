ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/situation
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/sensorHub/inc_v1
ccflags-y += -I$(srctree)/drivers/misc/mediatek/scp/

obj-$(CONFIG_MTK_SENSOR_SUPPORT)	+= situationhub.o
situationhub-y := situation_hub.o
situationhub-$(CONFIG_MTK_INPKHUB) += inpocket/inpocket.o
situationhub-$(CONFIG_MTK_STATHUB) += stationary/stationary.o
situationhub-$(CONFIG_MTK_WAKEHUB) += wake_gesture/wake_gesture.o
situationhub-$(CONFIG_MTK_GLGHUB) += glance_gesture/glance_gesture.o
situationhub-$(CONFIG_MTK_PICKUPHUB) += pickup_gesture/pickup_gesture.o
situationhub-$(CONFIG_MTK_ANSWER_CALL_HUB) += answercall/ancallhub.o
situationhub-$(CONFIG_MTK_DEVICE_ORIENTATION_HUB) += device_orientation/device_orientation.o
situationhub-$(CONFIG_MTK_MOTION_DETECT_HUB) += motion_detect/motion_detect.o
situationhub-$(CONFIG_MTK_TILTDETECTHUB) += tilt_detector/tiltdetecthub.o
situationhub-$(CONFIG_MTK_FLAT_HUB) += flat/flat.o
situationhub-$(CONFIG_MTK_SAR_HUB) += sar/sarhub.o sar/sar_factory.o
#TN Begin modified by ye.yuan/860593 20220825 EKFOGO4G-1638 begin
situationhub-$(CONFIG_MTK_FLIPTWIST_HUB) += flip_twist/flip_twist.o
#TN Begin modified by ye.yuan/860593 20220825 EKFOGO4G-1638 end
situationhub-$(CONFIG_MTK_CHOPCHOP_HUB) += chop_chop/chop_chop.o
situationhub-$(CONFIG_MTK_SIGMOVE_HUB) += sig_move/sig_move.o