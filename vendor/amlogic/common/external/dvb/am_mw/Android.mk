LOCAL_PATH := $(call my-dir)

ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 25)))
AMADEC_C_INCLUDES:=hardware/amlogic/media/amcodec/include \
       hardware/amlogic/LibAudio/amadec/include
else
AMADEC_C_INCLUDES:=$(LOCAL_PATH)/../../../frameworks/av/LibPlayer/amcodec/include \
       $(LOCAL_PATH)/../../../frameworks/av/LibPlayer/amadec/include
endif
include $(CLEAR_VARS)

LOCAL_MODULE    := libam_mw
ifeq ($(BOARD_COMPILE_IN_SYSTEM), true)
    LOCAL_VENDOR_MODULE := false
else
    LOCAL_VENDOR_MODULE := true
endif
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := am_db/am_db.c\
		   am_epg/am_epg.c\
		   am_rec/am_rec.c\
		   am_scan/am_scan.c\
		   am_sub2/am_sub.c am_sub2/dvb_sub.c \
		   am_tt2/am_tt.c \
		   am_si/am_si.c\
		   am_si/atsc/atsc_rrt.c\
		   am_si/atsc/atsc_vct.c\
		   am_si/atsc/atsc_mgt.c\
		   am_si/atsc/atsc_stt.c\
		   am_si/atsc/atsc_eit.c\
		   am_si/atsc/atsc_ett.c\
		   am_si/atsc/atsc_descriptor.c\
		   am_si/atsc/huffman_decode.c\
		   am_fend_ctrl/am_sec.c\
		   am_fend_ctrl/am_fend_ctrl.c\
		   am_caman/am_caman.c \
		   am_caman/ca_dummy.c \
		   am_cc/am_cc.c \
		   am_cc/cc_json.c \
		   am_isdb/am_isdb.c \
		   am_upd/am_upd.c \
                   am_closecaption/am_cc.c \
                   am_closecaption/am_cc_decoder.c \
                   am_closecaption/am_xds.c \
                   am_closecaption/am_cc_slice.c \
                   am_closecaption/am_vbi/linux_vbi/linux_vbi.c \
                   am_closecaption/am_vbi/am_vbi_api.c \
                   am_check_scramb/am_check_scramb.c \
                   am_scte27/am_scte27.c

LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DFONT_FREETYPE -DCHIP_8226M -DLOG_LEVEL=1 #
ifeq ($(AMLOGIC_LIBPLAYER), y)
LOCAL_CFLAGS+=-DAMLOGIC_LIBPLAYER
endif

LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include/am_adp\
		   $(AMADEC_C_INCLUDES)\
		   $(LOCAL_PATH)/../include/am_mw\
		   $(LOCAL_PATH)/../include/am_adp/libdvbsi\
		   $(LOCAL_PATH)/../include/am_adp/libdvbsi/descriptors\
		   $(LOCAL_PATH)/../include/am_adp/libdvbsi/tables\
		   $(LOCAL_PATH)/am_closecaption/am_vbi\
		   $(LOCAL_PATH)/../android/ndk/include\
		   $(LOCAL_PATH)/../android/ndk/include/linux/ \
		   $(LOCAL_PATH)/../../libzvbi/src\
		   external/sqlite/dist\
		   $(LOCAL_PATH)/../am_adp/am_open_lib/am_ci

ifeq ($(strip $(BOARD_TV_USE_NEW_TVIN_PARAM)),true)
LOCAL_CFLAGS += -DCC_TV_USE_NEW_TVIN_PARAM=1
endif

ifeq ($(BOARD_VNDK_VERSION), current)
ifeq ($(BOARD_COMPILE_IN_SYSTEM), true)
    LOCAL_C_INCLUDES += external/icu/icu4c/source/common
    LOCAL_SHARED_LIBRARIES+= libzvbi libsqlite libam_adp $(AMADEC_LIBS) liblog libdl libc libcutils
else
    LOCAL_CFLAGS += -DUSE_VENDOR_ICU
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../icu/icu4c/source/common
    LOCAL_STATIC_LIBRARIES+= libsqlite
    LOCAL_SHARED_LIBRARIES+= libzvbi libam_adp $(AMADEC_LIBS) liblog libdl libc libcutils
endif
else
LOCAL_C_INCLUDES += external/icu/icu4c/source/common
LOCAL_SHARED_LIBRARIES+= libzvbi libsqlite libam_adp $(AMADEC_LIBS) liblog libdl libc libcutils
endif

LOCAL_PRELINK_MODULE := false

ifeq ($(BOARD_COMPILE_IN_SYSTEM), true)
    LOCAL_PROPRIETARY_MODULE := false
else
    LOCAL_PROPRIETARY_MODULE := true
endif

#LOCAL_32_BIT_ONLY := true

ifeq ($(SUPPORT_CAS), y)
LOCAL_CFLAGS+=-DSUPPORT_CAS
endif
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := libam_mw
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES :=  \
		   am_db/am_db.c \
		   am_epg/am_epg.c\
		   am_rec/am_rec.c\
		   am_scan/am_scan.c\
		   am_sub2/am_sub.c am_sub2/dvb_sub.c \
		   am_tt2/am_tt.c \
		   am_si/am_si.c\
		   am_si/atsc/atsc_vct.c\
		   am_si/atsc/atsc_mgt.c\
		   am_si/atsc/atsc_rrt.c\
		   am_si/atsc/atsc_stt.c\
		   am_si/atsc/atsc_eit.c\
		   am_si/atsc/atsc_ett.c\
		   am_si/atsc/atsc_descriptor.c\
		   am_si/atsc/huffman_decode.c\
		   am_fend_ctrl/am_sec.c\
		   am_fend_ctrl/am_fend_ctrl.c\
		   am_caman/am_caman.c \
		   am_caman/ca_dummy.c \
		   am_isdb/am_isdb.c \
		   am_cc/am_cc.c \
		   am_cc/cc_json.c \
		   am_upd/am_upd.c \
		   am_closecaption/am_cc.c \
		   am_closecaption/am_cc_decoder.c \
		   am_closecaption/am_xds.c \
		   am_closecaption/am_cc_slice.c \
		   am_closecaption/am_vbi/linux_vbi/linux_vbi.c \
		   am_closecaption/am_vbi/am_vbi_api.c \
		   am_check_scramb/am_check_scramb.c \
                   am_scte27/am_scte27.c


LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DFONT_FREETYPE -DCHIP_8226M -DLOG_LEVEL=1

LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include/am_adp\
		    $(AMADEC_C_INCLUDES)\
		    $(LOCAL_PATH)/../include/am_mw\
		    $(LOCAL_PATH)/../include/am_adp/libdvbsi\
		    $(LOCAL_PATH)/../include/am_adp/libdvbsi/descriptors\
		    $(LOCAL_PATH)/../include/am_adp/libdvbsi/tables\
		    $(LOCAL_PATH)/am_scan/libsigdetect\
		    $(LOCAL_PATH)/../android/ndk/include\
		    $(LOCAL_PATH)/../android/ndk/include/linux/ \
		    $(LOCAL_PATH)/../../libzvbi/src\
		    external/sqlite/dist\
		    $(LOCAL_PATH)/am_closecaption/am_vbi\
		    $(LOCAL_PATH)/../am_adp/am_open_lib/am_ci

ifeq ($(BOARD_VNDK_VERSION), current)
ifeq ($(BOARD_COMPILE_IN_SYSTEM), true)
    LOCAL_C_INCLUDES += external/icu/icu4c/source/common
    LOCAL_SHARED_LIBRARIES+= libzvbi libsqlite libam_adp liblog libdl libc libcutils libicui18n liblog libdl libc libcutils
else
    LOCAL_CFLAGS += -DUSE_VENDOR_ICU
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../icu/icu4c/source/common
    LOCAL_STATIC_LIBRARIES+= libsqlite libicuuc libam_adp libzvbi
    LOCAL_SHARED_LIBRARIES+= liblog libdl libc libcutils
endif
else
LOCAL_C_INCLUDES += external/icu/icu4c/source/common
LOCAL_SHARED_LIBRARIES+= libzvbi libsqlite libam_adp liblog libdl libc libcutils libicui18n
endif
ifeq ($(SUPPORT_CAS), y)
LOCAL_CFLAGS+=-DSUPPORT_CAS
endif
LOCAL_PRELINK_MODULE := false

#LOCAL_32_BIT_ONLY := true

include $(BUILD_STATIC_LIBRARY)



