ifeq ($(BOARD_HAVE_BLUETOOTH_BROADCOM),true)
LOCAL_PATH := $(call my-dir)
include $(call all-subdir-makefiles)
endif
