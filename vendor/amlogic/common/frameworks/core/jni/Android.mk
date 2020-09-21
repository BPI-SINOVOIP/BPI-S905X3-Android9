LOCAL_PATH:= $(call my-dir)

ifeq ($(SUPPORT_HDMIIN),true)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    droid_logic_server_HDMIIN.cpp \
    onload.cpp \
    HDMIIN/audio_utils_ctl.cpp \
    HDMIIN/mAlsa.cpp \
    HDMIIN/audiodsp_ctl.cpp \

LOCAL_C_INCLUDES += \
    $(JNI_H_INCLUDE) \
    frameworks/base/libs/hwui \
    frameworks/base/services \
    frameworks/base/core/jni \
    frameworks/native/services \
    external/skia/include/core \
    libcore/include \
    libcore/include/libsuspend \
    libnativehelper/include/nativehelper \
	  $(call include-path-for, libhardware)/hardware \
	  $(call include-path-for, libhardware_legacy)/hardware_legacy

ifeq ($(strip $(BOARD_ALSA_AUDIO)),tiny)
    LOCAL_C_INCLUDES += external/tinyalsa/include
    LOCAL_CFLAGS += -DBOARD_ALSA_AUDIO_TINY
else
    LOCAL_C_INCLUDES += external/alsa-lib/include
endif

LOCAL_SHARED_LIBRARIES := \
    libbinder \
    libcutils \
    libutils \
    liblog \
    libhardware \
    libhardware_legacy \
    libmedia

LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_CFLAGS += -DEGL_EGLEXT_PROTOTYPES -DGL_GLEXT_PROTOTYPES

ifeq ($(strip $(BOARD_ALSA_AUDIO)),tiny)
    LOCAL_SHARED_LIBRARIES += libtinyalsa
else
    LOCAL_SHARED_LIBRARIES += libasound
endif

ifeq ($(WITH_MALLOC_LEAK_CHECK),true)
    LOCAL_CFLAGS += -DMALLOC_LEAK_CHECK
endif

LOCAL_MODULE:= libhdmiin

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

endif

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	droid_logic_DisplaySetting.cpp

LOCAL_C_INCLUDES := \
  system/core/libutils/include \
  $(JNI_H_INCLUDE) \
  libnativehelper/include/nativehelper

LOCAL_MODULE    := libdisplaysetting

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

########### build libsurfaceoverlay_jni
#include $(CLEAR_VARS)
#
#LOCAL_SRC_FILES := \
#    droid_logic_SurfaceOverlay.cpp
#
#
#LOCAL_C_INCLUDES := $(JNI_H_INCLUDE) \
#    frameworks/native/libs/gui/include \
#    hardware/amlogic/gralloc \
#    frameworks/native/opengl/include \
#    frameworks/native/libs/nativewindow/include \
#    system/libhidl/transport/token/1.0/utils/include \
#    libnativehelper/include/nativehelper \
#    libnativehelper/include_jni
#
#LOCAL_MODULE    := libsurfaceoverlay_jni
#
#LOCAL_SHARED_LIBRARIES := \
#    liblog \
#    libcutils \
#    libutils \
#    libui
#
#LOCAL_C_INCLUDES += \
#    frameworks/base/include \
#    frameworks/native/include \
#    $(JNI_H_INCLUDE)
#
#ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
#endif
#
#include $(BUILD_SHARED_LIBRARY)

########### build libremotecontrol

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    droid_logic_RemoteControl.cpp

LOCAL_C_INCLUDES := $(JNI_H_INCLUDE) \
    system/core/libutils/include \
    libnativehelper/include/nativehelper \
    $(BOARD_AML_VENDOR_PATH)/frameworks/services/remotecontrol

LOCAL_MODULE := libremotecontrol_jni

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libremotecontrolserver

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
