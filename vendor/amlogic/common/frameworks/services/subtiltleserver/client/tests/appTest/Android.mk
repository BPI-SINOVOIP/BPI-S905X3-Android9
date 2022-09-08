LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, java)


LOCAL_STATIC_ANDROID_LIBRARIES := androidx.appcompat_appcompat

LOCAL_PACKAGE_NAME := DemoTest
LOCAL_CERTIFICATE := platform
#LOCAL_SDK_VERSION := current
LOCAL_REQUIRED_MODULES := libsubtitleApiTestJni


#LOCAL_STATIC_JAVA_LIBRARIES := droidlogicLib
LOCAL_JAVA_LIBRARIES := droidlogic
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_DEX_PREOPT := false
#LOCAL_PRIVILEGED_MODULE := true
include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
