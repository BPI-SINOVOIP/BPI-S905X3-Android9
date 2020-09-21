# Bluetooth Robolectric test target.

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

# Include the testing libraries (JUnit4 + Robolectric libs).
LOCAL_STATIC_JAVA_LIBRARIES := \
    truth-prebuilt \
    mockito-robolectric-prebuilt

LOCAL_JAVA_LIBRARIES := \
    junit \
    platform-robolectric-3.6.1-prebuilt \
    sdk_vcurrent

LOCAL_INSTRUMENTATION_FOR := Bluetooth
LOCAL_MODULE := BluetoothRoboTests

LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_JAVA_LIBRARY)


# Bluetooth runner target to run the previous target.

include $(CLEAR_VARS)

LOCAL_MODULE := RunBluetoothRoboTests

LOCAL_SDK_VERSION := current

LOCAL_STATIC_JAVA_LIBRARIES := \
    BluetoothRoboTests

LOCAL_TEST_PACKAGE := Bluetooth

LOCAL_INSTRUMENT_SOURCE_DIRS := $(dir $(LOCAL_PATH))../src

include prebuilts/misc/common/robolectric/3.6.1/run_robotests.mk
