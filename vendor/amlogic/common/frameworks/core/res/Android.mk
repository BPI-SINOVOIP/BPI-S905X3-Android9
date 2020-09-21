
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_JAVA_LIBRARIES := droidlogic \
    droidlogic-tv

LOCAL_JNI_SHARED_LIBRARIES := libremotecontrol_jni libjniuevent

LOCAL_PRIVATE_PLATFORM_APIS := true

LOCAL_PACKAGE_NAME := droidlogic-res

LOCAL_CERTIFICATE := platform

LOCAL_MODULE_TAGS := optional

LOCAL_PROGUARD_ENABLED := disabled

LOCAL_PRIVILEGED_MODULE := true

LOCAL_PRODUCT_MODULE := true

include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
