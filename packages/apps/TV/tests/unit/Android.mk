LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests


# Include all test java files.
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_STATIC_JAVA_LIBRARIES := \
    android-support-test \
    mockito-target \
    tv-test-common \

LOCAL_JAVA_LIBRARIES := \
    android.test.runner.stubs \
    android.test.base.stubs \
    android.test.mock.stubs \


LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/../common/res

LOCAL_PACKAGE_NAME := TVUnitTests

LOCAL_INSTRUMENTATION_FOR := LiveTv

LOCAL_SDK_VERSION := system_current

LOCAL_PROGUARD_ENABLED := disabled
include $(BUILD_PACKAGE)

