LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_JAVA_LIBRARIES := libandroid-support-v4 libdlna
LOCAL_JAVA_LIBRARIES := droidlogic
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_PACKAGE_NAME := DLNA 
LOCAL_CERTIFICATE := platform
LOCAL_PROGUARD_ENABLED := disabled

ifeq (($(shell test $(PLATFORM_SDK_VERSION) -ge 26 ) && ($(shell test $(PLATFORM_SDK_VERSION) -lt 28)  && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SDK_VERSION := current
else ifeq ($(shell test $(PLATFORM_SDK_VERSION) -lt 27)  && echo OK),OK)
LOCAL_PRIVILEGED_MODULE := true
else
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_PRODUCT_MODULE := true
endif

include $(BUILD_PACKAGE)

##############################################

include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := \
                                        libandroid-support-v4:libs/android-support-v4.jar \
                                        libdlna:libs/dlna.jar

ifeq (($(shell test $(PLATFORM_SDK_VERSION) -ge 26 ) && ($(shell test $(PLATFORM_SDK_VERSION) -lt 28)  && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_MULTI_PREBUILT)

