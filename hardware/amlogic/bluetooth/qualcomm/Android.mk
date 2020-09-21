ifeq ($(BOARD_HAVE_BLUETOOTH_QCOM),true)
LOCAL_PATH := $(call my-dir)
$(info qcabt)
include $(call all-subdir-makefiles)
endif
