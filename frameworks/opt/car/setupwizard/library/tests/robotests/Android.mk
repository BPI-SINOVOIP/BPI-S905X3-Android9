LOCAL_PATH := $(call my-dir)

############################################################
# CarSetupWizardLib app just for Robolectric test target.  #
############################################################
include $(CLEAR_VARS)

LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res

LOCAL_PACKAGE_NAME := CarSetupWizardLib
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_MODULE_TAGS := optional

LOCAL_USE_AAPT2 := true

LOCAL_PRIVILEGED_MODULE := true

include frameworks/opt/car/setupwizard/library/common.mk

include $(BUILD_PACKAGE)

#############################################
# Car Setup Wizard Library Robolectric test target. #
#############################################
include $(CLEAR_VARS)

LOCAL_MODULE := CarSetupWizardLibRoboTests

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_RESOURCE_DIR := \
    $(LOCAL_PATH)/res

LOCAL_JAVA_RESOURCE_DIRS := config

# Include the testing libraries
LOCAL_JAVA_LIBRARIES := \
    robolectric_android-all-stub \
    Robolectric_all-target \
    mockito-robolectric-prebuilt \
    truth-prebuilt

LOCAL_INSTRUMENTATION_FOR := CarSetupWizardLib

LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_JAVA_LIBRARY)

#############################################################
# Car Setup Wizard Library runner target to run the previous target. #
#############################################################
include $(CLEAR_VARS)
LOCAL_MODULE := RunCarSetupWizardLibRoboTests

LOCAL_JAVA_LIBRARIES := \
    CarSetupWizardLibRoboTests \
    robolectric_android-all-stub \
    Robolectric_all-target \
    mockito-robolectric-prebuilt \
    truth-prebuilt

LOCAL_TEST_PACKAGE := CarSetupWizardLib

LOCAL_ROBOTEST_FILES := $(filter-out %/BaseRobolectricTest.java,\
    $(call find-files-in-subdirs,$(LOCAL_PATH)/src,*Test.java,.))

LOCAL_INSTRUMENT_SOURCE_DIRS := $(dir $(LOCAL_PATH))../src

include external/robolectric-shadows/run_robotests.mk