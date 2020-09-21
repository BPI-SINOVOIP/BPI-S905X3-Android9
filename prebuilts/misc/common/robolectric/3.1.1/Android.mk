LOCAL_PATH:= $(call my-dir)

############################
# Adding the Robolectric .JAR prebuilts from this directory into a single target.
# This is the one you probably want.
include $(CLEAR_VARS)

LOCAL_STATIC_JAVA_LIBRARIES := \
    platform-robolectric-annotations \
    platform-robolectric-multidex \
    platform-robolectric-resources \
    platform-robolectric-shadows-core \
    platform-robolectric-shadows-core-v16 \
    platform-robolectric-shadows-core-v17 \
    platform-robolectric-shadows-core-v18 \
    platform-robolectric-shadows-core-v19 \
    platform-robolectric-shadows-core-v21 \
    platform-robolectric-shadows-core-v22 \
    platform-robolectric-shadows-core-v23 \
    platform-robolectric-shadows-httpclient \
    platform-robolectric-snapshot \
    platform-robolectric-utils

LOCAL_MODULE := platform-robolectric-3.1.1-prebuilt

LOCAL_SDK_VERSION := current
LOCAL_MIN_SDK_VERSION := o-b1

include $(BUILD_STATIC_JAVA_LIBRARY)

###########################
# Macro for defining individual prebuilts from <name>:<file> lists
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

############################
# Defining the target names for the static prebuilt .JARs.

prebuilts := \
    platform-robolectric-annotations:lib/robolectric-annotations-3.1.1.jar \
    platform-robolectric-multidex:lib/shadows-multidex-3.1.1.jar \
    platform-robolectric-resources:lib/robolectric-resources-3.1.1.jar \
    platform-robolectric-shadows-core:lib/shadows-core-3.1.1.jar \
    platform-robolectric-shadows-core-v16:lib/shadows-core-v16-3.1.1.jar \
    platform-robolectric-shadows-core-v17:lib/shadows-core-v17-3.1.1.jar \
    platform-robolectric-shadows-core-v18:lib/shadows-core-v18-3.1.1.jar \
    platform-robolectric-shadows-core-v19:lib/shadows-core-v19-3.1.1.jar \
    platform-robolectric-shadows-core-v21:lib/shadows-core-v21-3.1.1.jar \
    platform-robolectric-shadows-core-v22:lib/shadows-core-v22-3.1.1.jar \
    platform-robolectric-shadows-core-v23:lib/shadows-core-v23-3.1.1.jar \
    platform-robolectric-shadows-httpclient:lib/shadows-httpclient-3.1.1.jar\
    platform-robolectric-snapshot:lib/robolectric-3.1.1.jar

$(foreach p,$(prebuilts),\
  $(call define-prebuilt,$(p)))

prebuilts :=

include $(CLEAR_VARS)

LOCAL_MODULE := platform-robolectric-utils
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_SRC_FILES := lib/robolectric-utils-3.1.1.jar
LOCAL_UNINSTALLABLE_MODULE := true
LOCAL_MIN_SDK_VERSION := o-b1
LOCAL_SDK_VERSION := current

include $(BUILD_PREBUILT)

##########################
include $(CLEAR_VARS)

LOCAL_STATIC_JAVA_LIBRARIES := \
    asm-5.0.1 \
    asm-commons-5.0.1 \
    asm-tree-5.0.1

LOCAL_MODULE := robo-asm-all

include $(BUILD_STATIC_JAVA_LIBRARY)


############################
# Defining the target names for the static prebuilt .JARs.

prebuilts := \
    asm-5.0.1:lib/asm-5.0.1.jar \
    asm-commons-5.0.1:lib/asm-commons-5.0.1.jar \
    asm-tree-5.0.1:lib/asm-tree-5.0.1.jar

$(foreach p,$(prebuilts),\
  $(call define-prebuilt,$(p)))

prebuilts :=
