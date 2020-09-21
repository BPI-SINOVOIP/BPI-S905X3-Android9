LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_JAVA_LIBRARIES := android.test.runner android.test.base android.test.mock

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := ManagedProvisioningTests
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_CERTIFICATE := platform

LOCAL_STATIC_JAVA_LIBRARIES := \
    android-support-test \
    mockito-target-minus-junit4 \
    espresso-core \
    espresso-intents

LOCAL_INSTRUMENTATION_FOR := ManagedProvisioning
LOCAL_COMPATIBILITY_SUITE := device-tests

include $(BUILD_PACKAGE)
