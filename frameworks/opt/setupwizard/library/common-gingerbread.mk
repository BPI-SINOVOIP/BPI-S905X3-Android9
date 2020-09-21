#
# Include this make file to build your application against this module.
#
# Make sure to include it after you've set all your desired LOCAL variables.
# Note that you must explicitly set your LOCAL_RESOURCE_DIR before including this file.
#
# For example:
#
#   LOCAL_RESOURCE_DIR := \
#        $(LOCAL_PATH)/res
#
#   include frameworks/opt/setupwizard/library/common-gingerbread.mk
#

# Path to directory of setup wizard lib (e.g. frameworks/opt/setupwizard/library)
suwlib_dir := $(dir $(lastword $(MAKEFILE_LIST)))

ifeq (,$(findstring setup-wizard-lib-gingerbread-compat,$(LOCAL_STATIC_ANDROID_LIBRARIES)))
  LOCAL_STATIC_ANDROID_LIBRARIES += setup-wizard-lib-gingerbread-compat
endif

ifeq (,$(findstring android-support-v7-appcompat,$(LOCAL_STATIC_ANDROID_LIBRARIES)))
  LOCAL_STATIC_ANDROID_LIBRARIES += android-support-v7-appcompat
endif

ifeq (,$(findstring android-support-v7-recyclerview,$(LOCAL_STATIC_ANDROID_LIBRARIES)))
  LOCAL_STATIC_ANDROID_LIBRARIES += android-support-v7-recyclerview
endif
