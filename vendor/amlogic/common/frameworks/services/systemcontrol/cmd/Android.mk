#================================
#getbootenv
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(TARGET_BOARD_PLATFORM), meson8)
LOCAL_CFLAGS += -DMESON8_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxbaby)
LOCAL_CFLAGS += -DGXBABY_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxtvbb)
LOCAL_CFLAGS += -DGXTVBB_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxl)
LOCAL_CFLAGS += -DGXL_ENVSIZE
endif

LOCAL_SRC_FILES:= \
	getbootenv.cpp \
	../ubootenv/Ubootenv.cpp

LOCAL_MODULE:= getbootenv

LOCAL_STATIC_LIBRARIES := \
	libcutils \
	liblog \
	libz \
	libc

LOCAL_C_INCLUDES := \
  external/zlib \
  $(BOARD_AML_VENDOR_PATH)/frameworks/services/systemcontrol

LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)


#================================
#setbootenv

include $(CLEAR_VARS)

ifeq ($(TARGET_BOARD_PLATFORM), meson8)
LOCAL_CFLAGS += -DMESON8_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxbaby)
LOCAL_CFLAGS += -DGXBABY_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxtvbb)
LOCAL_CFLAGS += -DGXTVBB_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxl)
LOCAL_CFLAGS += -DGXL_ENVSIZE
endif

LOCAL_SRC_FILES:= \
	setbootenv.cpp \
	../ubootenv/Ubootenv.cpp

LOCAL_MODULE:= setbootenv

LOCAL_STATIC_LIBRARIES := \
	libcutils \
	liblog \
	libz \
	libc

LOCAL_C_INCLUDES := \
  external/zlib \
  $(BOARD_AML_VENDOR_PATH)/frameworks/services/systemcontrol

LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)