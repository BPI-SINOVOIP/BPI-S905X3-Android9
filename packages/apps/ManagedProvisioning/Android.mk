LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_JAVA_LIBRARIES := android-support-v4

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res

LOCAL_USE_AAPT2 := true

LOCAL_PACKAGE_NAME := ManagedProvisioning
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true

# Packages to be included in code coverage runs. This does not affect production builds.
LOCAL_JACK_COVERAGE_INCLUDE_FILTER := com.android.managedprovisioning.*

include frameworks/opt/setupwizard/library/common-gingerbread.mk

include $(BUILD_PACKAGE)

# additionally, build tests if we build via mmm / mm
ifeq (,$(ONE_SHOT_MAKEFILE))
include $(call all-makefiles-under,$(LOCAL_PATH))
endif