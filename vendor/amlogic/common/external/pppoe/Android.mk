LOCAL_PATH:= $(call my-dir)

PPPOE_VERSION="\"3.0\""

#MAKE_JAR
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-subdir-java-files)
LOCAL_JNI_SHARED_LIBRARIES := libpppoejni
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := droidlogic.external.pppoe

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_JAVA_LIBRARY)

#MAKE_XML
include $(CLEAR_VARS)
LOCAL_MODULE := droidlogic.software.pppoe.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/etc/permissions
else
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions
endif

LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := jni/src/pppoe.c \
                   jni/src/if.c \
                   jni/src/debug.c \
                   jni/src/common.c \
                   jni/src/ppp.c \
                   jni/src/discovery.c \
                   jni/src/netwrapper.c
#LOCAL_C_INCLUDES := $(KERNEL_HEADERS)
#LOCAL_C_INCLUDES += external/selinux/libselinux/include/ \
                    external/selinux/libselinux/src

LOCAL_SHARED_LIBRARIES := liblog libcutils libselinux
LOCAL_MODULE = pppoe
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DVERSION=$(PPPOE_VERSION)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= jni/src/pppoe_status.c \
        jni/pppoe_jni.cpp \
        jni/src/netwrapper.c


LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog \
        libselinux


LOCAL_SHARED_LIBRARIES += libandroid_runtime   libnativehelper
LOCAL_SHARED_LIBRARIES += libc libcutils libnetutils
LOCAL_C_INCLUDES :=  $(JNI_H_INCLUDE) \
    $(LOCAL_PATH)/jni/src \
    libnativehelper/include/nativehelper

#LOCAL_C_INCLUDES += external/selinux/libselinux/include/ \
                    external/selinux/libselinux/src
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libpppoejni
LOCAL_PRELINK_MODULE := false

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_SRC_FILES:= jni/src/pppoe_cli.c \
        jni/src/common.c \
        jni/src/netwrapper.c

#LOCAL_C_INCLUDES += external/selinux/libselinux/include/ \
                    external/selinux/libselinux/src

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := pcli
LOCAL_SHARED_LIBRARIES := liblog libcutils libnetutils libselinux

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= jni/src/pppoe_wrapper.c \
        jni/src/common.c \
        jni/src/netwrapper.c

LOCAL_SHARED_LIBRARIES := \
        liblog libcutils libcrypto libnetutils libselinux

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/include

#LOCAL_C_INCLUDES += external/selinux/libselinux/include/ \
                    external/selinux/libselinux/src

LOCAL_CFLAGS := -DANDROID_CHANGES

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= pppoe_wrapper

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)

