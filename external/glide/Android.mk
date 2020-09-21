LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, library/src)
LOCAL_SRC_FILES += $(call all-java-files-under, third_party/disklrucache/src)
LOCAL_SRC_FILES += $(call all-java-files-under, third_party/gif_decoder/src)
LOCAL_SRC_FILES += $(call all-java-files-under, third_party/gif_encoder/src)
LOCAL_MANIFEST_FILE := library/src/main/AndroidManifest.xml

LOCAL_JAVA_LIBRARIES := volley

LOCAL_SHARED_ANDROID_LIBRARIES := \
    android-support-fragment \
    android-support-core-ui \
    android-support-compat

LOCAL_MODULE := glide
LOCAL_MODULE_TAGS := optional
LOCAL_USE_AAPT2 := true
LOCAL_SDK_VERSION := current

include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
