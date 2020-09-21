LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := am_vbi.c  \
		sliced_vbi.c    \
		ntsc_dmx/am_vbi_dmx.c \
		ntsc_dmx/linux_vbi/linux_ntsc.c

LOCAL_SHARED_LIBRARIES+= libzvbi

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../src \
		$(LOCAL_PATH)/include/ \
		$(LOCAL_PATH)/include/ntsc_dmx/

LOCAL_MODULE := libntsc_decode
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm
LOCAL_CFLAGS += -D_REENTRANT -D_GNU_SOURCE -DENABLE_DVB=1 -DENABLE_V4L=1 -DENABLE_V4L2=1 -DHAVE_ICONV=1 -DPACKAGE=\"zvbi\" -DVERSION=\"0.2.33\" -DANDROID -DHAVE_GETOPT_LONG=1
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28&& echo OK),OK)
LOCAL_STATIC_LIBRARIES += libicuuc_vendor libicuuc_stubdata_vendor
LOCAL_SHARED_LIBRARIES += liblog
else
LOCAL_SHARED_LIBRARIES += libicuuc liblog
endif
include $(BUILD_SHARED_LIBRARY)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28&& echo OK),OK)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := am_vbi.c  \
		sliced_vbi.c    \
		ntsc_dmx/am_vbi_dmx.c \
		ntsc_dmx/linux_vbi/linux_ntsc.c

LOCAL_STATIC_LIBRARIES += libzvbi

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../src \
		$(LOCAL_PATH)/include/ \
		$(LOCAL_PATH)/include/ntsc_dmx/

LOCAL_MODULE := libntsc_decode
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm
LOCAL_CFLAGS += -D_REENTRANT -D_GNU_SOURCE -DENABLE_DVB=1 -DENABLE_V4L=1 -DENABLE_V4L2=1 -DHAVE_ICONV=1 -DPACKAGE=\"zvbi\" -DVERSION=\"0.2.33\" -DANDROID -DHAVE_GETOPT_LONG=1
LOCAL_STATIC_LIBRARIES += libicuuc libicuuc_stubdata
LOCAL_SHARED_LIBRARIES += liblog

include $(BUILD_STATIC_LIBRARY)
endif
