
LIBAUDIO_PATH:= $(call my-dir)/../

LOCAL_C_INCLUDES += \
			$(LIBAUDIO_PATH)/amadec/include \
			$(LIBAUDIO_PATH)/amadec/ \

include $(TOP)/hardware/amlogic/media/media_base_config.mk

LOCAL_C_INCLUDES += $(AMACODEC_PATH)/include/
