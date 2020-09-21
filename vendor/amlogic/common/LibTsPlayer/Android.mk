
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_REQUIRED_MODULES :=libCTC_MediaProcessorjni


#LOCAL_PROGUARD_FLAGS := -include $(LOCAL_PATH)/proguard.cfg

LOCAL_PACKAGE_NAME := mediaProcessorDemo
LOCAL_CERTIFICATE := platform

LOCAL_PROGUARD_ENABLED := disabled
#LOCAL_STATIC_JAVA_LIBRARIES := amlogic.subtitle

LOCAL_JNI_SHARED_LIBRARIES := \
	libCTC_MediaProcessorjni

LOCAL_SDK_VERSION := current
LOCAL_DEX_PREOPT := false
	
include $(BUILD_PACKAGE)
##################################################

include $(call all-makefiles-under,$(LOCAL_PATH))
