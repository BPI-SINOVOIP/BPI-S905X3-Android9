MEDIA_BASE_PATH := $(call my-dir)

AMAVUTILS_PATH:=$(MEDIA_BASE_PATH)/amavutils/
AMACODEC_PATH:=$(MEDIA_BASE_PATH)/amcodec/
AMVDEC_PATH:=$(MEDIA_BASE_PATH)/amvdec/

AMADEC_PATH:=$(TOP)/$(BOARD_AML_VENDOR_PATH)/frameworks/av/libaudio/amadec

AMCODEC_NEED_INCLUDE := \
	$(AMAVUTILS_PATH)/include \
	$(AMACODEC_PATH)/include \
	$(AMADEC_PATH)/include

