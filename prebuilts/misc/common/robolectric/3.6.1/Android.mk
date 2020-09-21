LOCAL_PATH:= $(call my-dir)

############################
# Adding the Robolectric .JAR prebuilts from this directory into a single target.
# This is the one you probably want.
include $(CLEAR_VARS)

LOCAL_STATIC_JAVA_LIBRARIES := \
    platform-robolectric-3.6.1-asm \
    platform-robolectric-3.6.1-annotations \
    platform-robolectric-3.6.1-junit \
    platform-robolectric-3.6.1-resources \
    platform-robolectric-3.6.1-sandbox \
    platform-robolectric-3.6.1-shadow-api \
    platform-robolectric-3.6.1-shadows-framework \
    platform-robolectric-3.6.1-shadows-httpclient \
    platform-robolectric-3.6.1-shadows-multidex \
    platform-robolectric-3.6.1-shadows-support-v4 \
    platform-robolectric-3.6.1-snapshot \
    platform-robolectric-3.6.1-utils

LOCAL_MODULE := platform-robolectric-3.6.1-prebuilt

LOCAL_SDK_VERSION := current

include $(BUILD_STATIC_JAVA_LIBRARY)

############################
# Defining the target names for the static prebuilt .JARs.

prebuilts := \
    platform-robolectric-3.6.1-asm:lib/asm-6.0.jar \
    platform-robolectric-3.6.1-annotations:lib/annotations-3.6.1.jar \
    platform-robolectric-3.6.1-junit:lib/junit-3.6.1.jar \
    platform-robolectric-3.6.1-resources:lib/resources-3.6.1.jar \
    platform-robolectric-3.6.1-sandbox:lib/sandbox-3.6.1.jar \
    platform-robolectric-3.6.1-shadow-api:lib/shadowapi-3.6.1.jar \
    platform-robolectric-3.6.1-shadows-framework:lib/shadows-framework-3.6.1.jar \
    platform-robolectric-3.6.1-shadows-httpclient:lib/shadows-httpclient-3.6.1.jar \
    platform-robolectric-3.6.1-shadows-multidex:lib/shadows-multidex-3.6.1.jar \
    platform-robolectric-3.6.1-shadows-support-v4:lib/shadows-supportv4-3.6.1.jar \
    platform-robolectric-3.6.1-snapshot:lib/robolectric-3.6.1.jar \
    platform-robolectric-3.6.1-utils:lib/utils-3.6.1.jar

define define-prebuilt
  $(eval tw := $(subst :, ,$(strip $(1)))) \
  $(eval include $(CLEAR_VARS)) \
  $(eval LOCAL_MODULE := $(word 1,$(tw))) \
  $(eval LOCAL_MODULE_TAGS := optional) \
  $(eval LOCAL_MODULE_CLASS := JAVA_LIBRARIES) \
  $(eval LOCAL_SRC_FILES := $(word 2,$(tw))) \
  $(eval LOCAL_UNINSTALLABLE_MODULE := true) \
  $(eval LOCAL_SDK_VERSION := current) \
  $(eval include $(BUILD_PREBUILT))
endef

$(foreach p,$(prebuilts),\
  $(call define-prebuilt,$(p)))

prebuilts :=
