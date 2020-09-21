LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := libzvbi
ifeq ($(BOARD_COMPILE_IN_SYSTEM), true)
    LOCAL_VENDOR_MODULE := false
else
    LOCAL_VENDOR_MODULE := true
endif
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := src/bit_slicer.c src/cache.c src/caption.c src/conv.c src/dvb_mux.c src/dvb_demux.c src/exp-html.c \
	src/exp-templ.c src/exp-txt.c src/exp-vtx.c src/exp-gfx.c src/export.c src/hamm.c src/idl_demux.c src/io.c src/io-bktr.c src/io-dvb.c \
	src/io-sim.c src/io-v4l.c src/io-v4l2.c src/io-v4l2k.c src/lang.c src/misc.c src/packet.c src/teletext.c src/page_table.c \
	src/pfc_demux.c src/proxy-client.c src/raw_decoder.c src/sampling_par.c src/search.c src/ure.c src/sliced_filter.c \
	src/tables.c src/trigger.c src/vbi.c src/vps.c src/wss.c src/xds_demux.c src/decoder.c src/dtvcc.c

LOCAL_CFLAGS+=-D_REENTRANT -D_GNU_SOURCE -DENABLE_DVB=1 -DENABLE_V4L=1 -DENABLE_V4L2=1 -DHAVE_ICONV=1 -DPACKAGE=\"zvbi\" -DVERSION=\"0.2.33\" -DANDROID
LOCAL_CLANG_CFLAGS+=-Wno-error=tautological-pointer-compare
ifeq ($(ANDROID_BUILD_TYPE), 64)
LOCAL_CFLAGS+=-DHAVE_S64_U64
endif

LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := external/icu4c/common
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../dvb/include/am_adp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../dvb/include/am_adp
LOCAL_C_INCLUDES += vendor/amlogic/dvb/include/am_adp

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28&& echo OK),OK)
    ifeq ($(BOARD_COMPILE_IN_SYSTEM), true)
        LOCAL_C_INCLUDES += $(LOCAL_PATH)/../icu/icu4c/source/common
        LOCAL_STATIC_LIBRARIES += libicuuc libicuuc_stubdata
        LOCAL_SHARED_LIBRARIES += liblog libam_adp
    else
        LOCAL_C_INCLUDES += $(LOCAL_PATH)/../icu/icu4c/source/common
        LOCAL_STATIC_LIBRARIES += libicuuc_vendor libicuuc_stubdata_vendor
        LOCAL_SHARED_LIBRARIES += liblog libam_adp
    endif
else
LOCAL_C_INCLUDES += external/icu/icu4c/source/common
LOCAL_SHARED_LIBRARIES += libicuuc liblog libam_adp
endif
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28&& echo OK),OK)
include $(CLEAR_VARS)

LOCAL_MODULE    := libzvbi
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := src/bit_slicer.c src/cache.c src/caption.c src/conv.c src/dvb_mux.c src/dvb_demux.c src/exp-html.c \
	src/exp-templ.c src/exp-txt.c src/exp-vtx.c src/exp-gfx.c src/export.c src/hamm.c src/idl_demux.c src/io.c src/io-bktr.c src/io-dvb.c \
	src/io-sim.c src/io-v4l.c src/io-v4l2.c src/io-v4l2k.c src/lang.c src/misc.c src/packet.c src/teletext.c src/page_table.c \
	src/pfc_demux.c src/proxy-client.c src/raw_decoder.c src/sampling_par.c src/search.c src/ure.c src/sliced_filter.c \
	src/tables.c src/trigger.c src/vbi.c src/vps.c src/wss.c src/xds_demux.c src/decoder.c src/dtvcc.c

LOCAL_CFLAGS+=-D_REENTRANT -D_GNU_SOURCE -DENABLE_DVB=1 -DENABLE_V4L=1 -DENABLE_V4L2=1 -DHAVE_ICONV=1 -DPACKAGE=\"zvbi\" -DVERSION=\"0.2.33\" -DANDROID
LOCAL_CLANG_CFLAGS+=-Wno-error=tautological-pointer-compare
ifeq ($(ANDROID_BUILD_TYPE), 64)
LOCAL_CFLAGS+=-DHAVE_S64_U64
endif

LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := external/icu4c/common
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../dvb/include/am_adp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../dvb/include/am_adp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../icu/icu4c/source/common
LOCAL_STATIC_LIBRARIES += libicuuc libicuuc_stubdata libam_adp
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_PRELINK_MODULE := false

include $(BUILD_STATIC_LIBRARY)
endif
include $(LOCAL_PATH)/ntsc_decode/Android.mk
#include $(LOCAL_PATH)/test/Android.mk
