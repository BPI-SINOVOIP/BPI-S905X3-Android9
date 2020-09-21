LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_PACKAGE_NAME := OpenWnn
LOCAL_PRIVATE_PLATFORM_APIS := true

LOCAL_JNI_SHARED_LIBRARIES := \
	 libWnnEngDic libWnnJpnDic libwnndict

LOCAL_AAPT_FLAGS += -c hdpi

include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
