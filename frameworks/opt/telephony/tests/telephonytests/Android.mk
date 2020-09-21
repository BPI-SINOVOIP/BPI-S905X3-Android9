LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_JAVA_LIBRARIES := \
    android.test.runner \
    ims-common \
    bouncycastle \
    android.test.base \
    android.test.mock

LOCAL_STATIC_JAVA_LIBRARIES := guava \
                               frameworks-base-testutils \
                               services.core \
                               telephony-common \
                               mockito-target-minus-junit4 \
                               android-support-test \
                               platform-test-annotations

LOCAL_PACKAGE_NAME := FrameworksTelephonyTests
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_COMPATIBILITY_SUITE := device-tests

# b/72575505
LOCAL_ERROR_PRONE_FLAGS := -Xep:JUnit4TestNotRun:WARN

include $(BUILD_PACKAGE)
