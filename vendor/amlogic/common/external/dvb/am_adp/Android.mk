LOCAL_PATH := $(call my-dir)
AMLOGIC_LIBPLAYER :=y

ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 25)))
AMADEC_C_INCLUDES:=hardware/amlogic/media/amcodec/include\
       hardware/amlogic/LibAudio/amadec/include
else
AMADEC_C_INCLUDES:=$(LOCAL_PATH)/../../../frameworks/av/LibPlayer/amcodec/include\
       $(LOCAL_PATH)/../../../frameworks/av/LibPlayer/amadec/include
endif

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28&& echo OK),OK)
else
ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 25)))
AMADEC_LIBS:=libamadec
else
AMADEC_LIBS:=libamplayer
endif
endif

include $(CLEAR_VARS)

LOCAL_MODULE    := libam_adp
ifeq ($(BOARD_COMPILE_IN_SYSTEM), true)
    LOCAL_VENDOR_MODULE := false
else
    LOCAL_VENDOR_MODULE := true
endif
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := am_dmx/am_dmx.c am_dmx/linux_dvb/linux_dvb.c\
		   am_fend/am_fend.c am_fend/am_vlfend.c am_fend/am_fend_diseqc_cmd.c am_fend/am_rotor_calc.c am_fend/linux_dvb/linux_dvb.c am_fend/linux_v4l2/linux_v4l2.c\
		   am_av/am_av.c am_av/aml/aml.c\
		   am_dvr/am_dvr.c am_dvr/linux_dvb/linux_dvb.c\
		   am_dmx/dvr/dvr.c\
		   am_aout/am_aout.c\
		   am_vout/am_vout.c\
		   am_vout/aml/aml.c\
		   am_misc/am_adplock.c am_misc/am_misc.c am_misc/am_iconv.c am_misc/am_sig_handler.c\
		   am_time/am_time.c\
		   am_evt/am_evt.c\
		   am_kl/am_kl.c \
		   am_dsc/am_dsc.c am_dsc/aml/aml.c\
		   am_smc/am_smc.c\
		   am_smc/aml/aml.c\
		   am_userdata/am_userdata.c\
		   am_userdata/aml/aml.c\
		   am_userdata/emu/emu.c\
		   am_pes/am_pes.c \
		   am_ad/am_ad.c \
		   am_open_lib/libdvbsi/tables/bat.c\
		   am_open_lib/libdvbsi/tables/sdt.c\
		   am_open_lib/libdvbsi/tables/pat.c\
		   am_open_lib/libdvbsi/tables/cat.c\
		   am_open_lib/libdvbsi/tables/pmt.c\
		   am_open_lib/libdvbsi/tables/tot.c\
		   am_open_lib/libdvbsi/tables/eit.c\
		   am_open_lib/libdvbsi/tables/nit.c\
		   am_open_lib/libdvbsi/tables/atsc_eit.c\
		   am_open_lib/libdvbsi/tables/atsc_ett.c\
		   am_open_lib/libdvbsi/tables/atsc_mgt.c\
		   am_open_lib/libdvbsi/tables/atsc_stt.c\
		   am_open_lib/libdvbsi/tables/atsc_vct.c\
		   am_open_lib/libdvbsi/tables/atsc_cea.c\
		   am_open_lib/libdvbsi/tables/huffman_decode.c\
		   am_open_lib/libdvbsi/demux.c\
		   am_open_lib/libdvbsi/descriptors/dr_0f.c\
		   am_open_lib/libdvbsi/descriptors/dr_44.c\
		   am_open_lib/libdvbsi/descriptors/dr_0a.c\
		   am_open_lib/libdvbsi/descriptors/dr_47.c\
		   am_open_lib/libdvbsi/descriptors/dr_03.c\
		   am_open_lib/libdvbsi/descriptors/dr_5a.c\
		   am_open_lib/libdvbsi/descriptors/dr_05.c\
		   am_open_lib/libdvbsi/descriptors/dr_48.c\
		   am_open_lib/libdvbsi/descriptors/dr_69.c\
		   am_open_lib/libdvbsi/descriptors/dr_02.c\
		   am_open_lib/libdvbsi/descriptors/dr_4d.c\
		   am_open_lib/libdvbsi/descriptors/dr_58.c\
		   am_open_lib/libdvbsi/descriptors/dr_56.c\
		   am_open_lib/libdvbsi/descriptors/dr_4e.c\
		   am_open_lib/libdvbsi/descriptors/dr_4a.c\
		   am_open_lib/libdvbsi/descriptors/dr_45.c\
		   am_open_lib/libdvbsi/descriptors/dr_41.c\
		   am_open_lib/libdvbsi/descriptors/dr_43.c\
		   am_open_lib/libdvbsi/descriptors/dr_0e.c\
		   am_open_lib/libdvbsi/descriptors/dr_04.c\
		   am_open_lib/libdvbsi/descriptors/dr_59.c\
		   am_open_lib/libdvbsi/descriptors/dr_0c.c\
		   am_open_lib/libdvbsi/descriptors/dr_54.c\
		   am_open_lib/libdvbsi/descriptors/dr_09.c\
		   am_open_lib/libdvbsi/descriptors/dr_52.c\
		   am_open_lib/libdvbsi/descriptors/dr_40.c\
		   am_open_lib/libdvbsi/descriptors/dr_55.c\
		   am_open_lib/libdvbsi/descriptors/dr_08.c\
		   am_open_lib/libdvbsi/descriptors/dr_0b.c\
		   am_open_lib/libdvbsi/descriptors/dr_42.c\
		   am_open_lib/libdvbsi/descriptors/dr_07.c\
		   am_open_lib/libdvbsi/descriptors/dr_0d.c\
		   am_open_lib/libdvbsi/descriptors/dr_06.c\
		   am_open_lib/libdvbsi/descriptors/dr_81.c\
		   am_open_lib/libdvbsi/descriptors/dr_83.c\
		   am_open_lib/libdvbsi/descriptors/dr_86.c\
		   am_open_lib/libdvbsi/descriptors/dr_87.c\
		   am_open_lib/libdvbsi/descriptors/dr_87_ca.c\
		   am_open_lib/libdvbsi/descriptors/dr_88.c\
		   am_open_lib/libdvbsi/descriptors/dr_5d.c\
		   am_open_lib/libdvbsi/descriptors/dr_6a.c\
		   am_open_lib/libdvbsi/descriptors/dr_7a.c\
		   am_open_lib/libdvbsi/descriptors/dr_7f.c\
		   am_open_lib/libdvbsi/descriptors/dr_a1.c\
		   am_open_lib/libdvbsi/descriptors/dr_cc.c\
		   am_open_lib/libdvbsi/psi.c\
		   am_open_lib/libdvbsi/dvbpsi.c\
		   am_open_lib/libdvbsi/descriptor.c\
		   am_open_lib/am_ci/libdvben50221/asn_1.c \
		   am_open_lib/am_ci/libdvben50221/en50221_app_ai.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_app_auth.c      \
		   am_open_lib/am_ci/libdvben50221/en50221_app_ca.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_app_datetime.c  \
		   am_open_lib/am_ci/libdvben50221/en50221_app_dvb.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_app_epg.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_app_lowspeed.c  \
		   am_open_lib/am_ci/libdvben50221/en50221_app_mmi.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_app_rm.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_app_smartcard.c \
		   am_open_lib/am_ci/libdvben50221/en50221_app_teletext.c  \
		   am_open_lib/am_ci/libdvben50221/en50221_app_utils.c     \
		   am_open_lib/am_ci/libdvben50221/en50221_session.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_stdcam.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_stdcam_hlci.c   \
		   am_open_lib/am_ci/libdvben50221/en50221_stdcam_llci.c   \
		   am_open_lib/am_ci/libdvben50221/en50221_transport.c \
		   am_open_lib/am_ci/libucsi/dvb/types.c \
		   am_open_lib/am_ci/libdvbapi/dvbca.c \
		   am_open_lib/am_ci/libucsi/mpeg/pmt_section.c \
		   am_open_lib/am_ci/am_ci.c \
		   am_open_lib/am_ci/ca_ci.c \
		   am_open_lib/am_freesat/freesat.c \
		   am_open_lib/am_crypt/am_crypt.c \
		   am_open_lib/am_crypt/des.c \
		   am_tfile/am_tfile.c

LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DCHIP_8226M -DLINUX_DVB_FEND -DLOG_LEVEL=1
ifeq ($(AMLOGIC_LIBPLAYER), y)
LOCAL_CFLAGS+=-DAMLOGIC_LIBPLAYER
endif
LOCAL_CFLAGS+=-std=gnu99

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28&& echo OK),OK)
else
LOCAL_CFLAGS+=-DUSE_ADEC_IN_DVB
endif

LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include/am_adp\
		   $(LOCAL_PATH)/../include/am_adp/libdvbsi\
		   $(LOCAL_PATH)/../include/am_adp/libdvbsi/descriptors\
		   $(LOCAL_PATH)/../include/am_adp/libdvbsi/tables\
		   $(LOCAL_PATH)/am_open_lib/am_ci\
		   $(LOCAL_PATH)/../include/am_mw\
		   $(LOCAL_PATH)/../android/ndk/include\
		   $(AMADEC_C_INCLUDES)\
		   common/include/linux/amlogic

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28&& echo OK),OK)
ifeq ($(BOARD_COMPILE_IN_SYSTEM), true)
    LOCAL_C_INCLUDES += external/icu/icu4c/source/common
    LOCAL_SHARED_LIBRARIES+=$(AMADEC_LIBS) libicuuc libcutils liblog libdl libc
else
    LOCAL_CFLAGS += -DUSE_VENDOR_ICU
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../icu/icu4c/source/common
    LOCAL_SHARED_LIBRARIES+=$(AMADEC_LIBS) libicuuc_vendor libcutils liblog libdl libc
endif
else
LOCAL_C_INCLUDES += external/icu/icu4c/source/common
LOCAL_SHARED_LIBRARIES+=$(AMADEC_LIBS) libicuuc libcutils liblog libdl libc
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

LOCAL_MODULE    := libam_adp
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := am_dmx/am_dmx.c am_dmx/linux_dvb/linux_dvb.c\
		   am_fend/am_fend.c am_fend/am_vlfend.c am_fend/am_fend_diseqc_cmd.c am_fend/am_rotor_calc.c am_fend/linux_dvb/linux_dvb.c am_fend/linux_v4l2/linux_v4l2.c\
	           am_av/am_av.c am_av/aml/aml.c\
	           am_dvr/am_dvr.c am_dvr/linux_dvb/linux_dvb.c\
	           am_dmx/dvr/dvr.c\
	           am_aout/am_aout.c\
	           am_vout/am_vout.c\
	           am_vout/aml/aml.c\
	           am_misc/am_adplock.c am_misc/am_misc.c am_misc/am_iconv.c am_misc/am_sig_handler.c\
	           am_time/am_time.c\
	           am_evt/am_evt.c\
		   am_kl/am_kl.c\
	           am_dsc/am_dsc.c am_dsc/aml/aml.c\
	           am_smc/am_smc.c\
	           am_smc/aml/aml.c\
		   am_userdata/am_userdata.c\
		   am_userdata/aml/aml.c\
		   am_userdata/emu/emu.c\
		   am_pes/am_pes.c \
		   am_ad/am_ad.c \
		   am_open_lib/libdvbsi/tables/bat.c\
		   am_open_lib/libdvbsi/tables/sdt.c\
		   am_open_lib/libdvbsi/tables/pat.c\
		   am_open_lib/libdvbsi/tables/cat.c\
		   am_open_lib/libdvbsi/tables/pmt.c\
		   am_open_lib/libdvbsi/tables/tot.c\
		   am_open_lib/libdvbsi/tables/eit.c\
		   am_open_lib/libdvbsi/tables/nit.c\
		   am_open_lib/libdvbsi/tables/atsc_eit.c\
		   am_open_lib/libdvbsi/tables/atsc_ett.c\
		   am_open_lib/libdvbsi/tables/atsc_mgt.c\
		   am_open_lib/libdvbsi/tables/atsc_stt.c\
		   am_open_lib/libdvbsi/tables/atsc_vct.c\
		   am_open_lib/libdvbsi/tables/atsc_cea.c\
		   am_open_lib/libdvbsi/tables/huffman_decode.c\
		   am_open_lib/libdvbsi/demux.c\
		   am_open_lib/libdvbsi/descriptors/dr_0f.c\
		   am_open_lib/libdvbsi/descriptors/dr_44.c\
		   am_open_lib/libdvbsi/descriptors/dr_0a.c\
		   am_open_lib/libdvbsi/descriptors/dr_47.c\
		   am_open_lib/libdvbsi/descriptors/dr_03.c\
		   am_open_lib/libdvbsi/descriptors/dr_5a.c\
		   am_open_lib/libdvbsi/descriptors/dr_05.c\
		   am_open_lib/libdvbsi/descriptors/dr_48.c\
		   am_open_lib/libdvbsi/descriptors/dr_69.c\
		   am_open_lib/libdvbsi/descriptors/dr_02.c\
		   am_open_lib/libdvbsi/descriptors/dr_4d.c\
		   am_open_lib/libdvbsi/descriptors/dr_58.c\
		   am_open_lib/libdvbsi/descriptors/dr_56.c\
		   am_open_lib/libdvbsi/descriptors/dr_4e.c\
		   am_open_lib/libdvbsi/descriptors/dr_4a.c\
		   am_open_lib/libdvbsi/descriptors/dr_45.c\
		   am_open_lib/libdvbsi/descriptors/dr_41.c\
		   am_open_lib/libdvbsi/descriptors/dr_43.c\
		   am_open_lib/libdvbsi/descriptors/dr_0e.c\
		   am_open_lib/libdvbsi/descriptors/dr_04.c\
		   am_open_lib/libdvbsi/descriptors/dr_59.c\
		   am_open_lib/libdvbsi/descriptors/dr_0c.c\
		   am_open_lib/libdvbsi/descriptors/dr_54.c\
		   am_open_lib/libdvbsi/descriptors/dr_09.c\
		   am_open_lib/libdvbsi/descriptors/dr_52.c\
		   am_open_lib/libdvbsi/descriptors/dr_40.c\
		   am_open_lib/libdvbsi/descriptors/dr_55.c\
		   am_open_lib/libdvbsi/descriptors/dr_08.c\
		   am_open_lib/libdvbsi/descriptors/dr_0b.c\
		   am_open_lib/libdvbsi/descriptors/dr_42.c\
		   am_open_lib/libdvbsi/descriptors/dr_07.c\
		   am_open_lib/libdvbsi/descriptors/dr_0d.c\
		   am_open_lib/libdvbsi/descriptors/dr_06.c\
		   am_open_lib/libdvbsi/descriptors/dr_81.c\
		   am_open_lib/libdvbsi/descriptors/dr_83.c\
		   am_open_lib/libdvbsi/descriptors/dr_86.c\
		   am_open_lib/libdvbsi/descriptors/dr_87.c\
		   am_open_lib/libdvbsi/descriptors/dr_87_ca.c\
		   am_open_lib/libdvbsi/descriptors/dr_88.c\
		   am_open_lib/libdvbsi/descriptors/dr_5d.c\
		   am_open_lib/libdvbsi/descriptors/dr_6a.c\
		   am_open_lib/libdvbsi/descriptors/dr_7a.c\
		   am_open_lib/libdvbsi/descriptors/dr_7f.c\
		   am_open_lib/libdvbsi/descriptors/dr_a1.c\
		   am_open_lib/libdvbsi/descriptors/dr_cc.c\
		   am_open_lib/libdvbsi/psi.c\
		   am_open_lib/libdvbsi/dvbpsi.c\
		   am_open_lib/libdvbsi/descriptor.c\
		   am_open_lib/am_ci/libdvben50221/asn_1.c \
		   am_open_lib/am_ci/libdvben50221/en50221_app_ai.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_app_auth.c      \
		   am_open_lib/am_ci/libdvben50221/en50221_app_ca.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_app_datetime.c  \
		   am_open_lib/am_ci/libdvben50221/en50221_app_dvb.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_app_epg.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_app_lowspeed.c  \
		   am_open_lib/am_ci/libdvben50221/en50221_app_mmi.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_app_rm.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_app_smartcard.c \
		   am_open_lib/am_ci/libdvben50221/en50221_app_teletext.c  \
		   am_open_lib/am_ci/libdvben50221/en50221_app_utils.c     \
		   am_open_lib/am_ci/libdvben50221/en50221_session.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_stdcam.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_stdcam_hlci.c   \
		   am_open_lib/am_ci/libdvben50221/en50221_stdcam_llci.c   \
		   am_open_lib/am_ci/libdvben50221/en50221_transport.c \
		   am_open_lib/am_ci/libucsi/dvb/types.c \
		   am_open_lib/am_ci/libdvbapi/dvbca.c \
		   am_open_lib/am_ci/libucsi/mpeg/pmt_section.c \
		   am_open_lib/am_ci/am_ci.c \
		   am_open_lib/am_ci/ca_ci.c \
		   am_open_lib/am_freesat/freesat.c \
		   am_open_lib/am_crypt/am_crypt.c \
		   am_open_lib/am_crypt/des.c \
		   am_tfile/am_tfile.c



LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DCHIP_8226M -DLINUX_DVB_FEND -DLOG_LEVEL=1 -DNO_SYSFS
ifeq ($(AMLOGIC_LIBPLAYER), y)
LOCAL_CFLAGS+=-DAMLOGIC_LIBPLAYER
endif
ifeq ($(SUPPORT_CAS), y)
LOCAL_CFLAGS+=-DSUPPORT_CAS
endif
LOCAL_CFLAGS+=-std=gnu99

LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include/am_adp\
		   $(AMADEC_C_INCLUDES)\
		   $(LOCAL_PATH)/../include/am_adp/libdvbsi\
		   $(LOCAL_PATH)/../include/am_adp/libdvbsi/descriptors\
		   $(LOCAL_PATH)/../include/am_adp/libdvbsi/tables\
		   $(LOCAL_PATH)/am_open_lib/am_ci\
		   $(LOCAL_PATH)/../include/am_mw\
		   $(LOCAL_PATH)/../android/ndk/include\
		   common/include/linux/amlogic

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28&& echo OK),OK)
    ifeq ($(BOARD_COMPILE_IN_SYSTEM), true)
        LOCAL_C_INCLUDES += external/icu/icu4c/source/common
        LOCAL_SHARED_LIBRARIES+= libcutils liblog libdl libc
        LOCAL_STATIC_LIBRARIES+= libicuuc_product libicui18n_product
    else
        LOCAL_CFLAGS += -DUSE_VENDOR_ICU
        LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../icu/icu4c/source/common
        LOCAL_SHARED_LIBRARIES+= libcutils liblog libdl libc
        LOCAL_STATIC_LIBRARIES+= libicuuc_product libicui18n_product
    endif
else
LOCAL_C_INCLUDES += external/icu/icu4c/source/common
LOCAL_SHARED_LIBRARIES+= libicuuc libcutils liblog libdl libc
endif

LOCAL_PRELINK_MODULE := false

#LOCAL_32_BIT_ONLY := true

include $(BUILD_STATIC_LIBRARY)

###################################################################################################################
include $(CLEAR_VARS)

AMADEC_C_INCLUDES:=hardware/amlogic/media/amcodec/include\
        hardware/amlogic/LibAudio/amadec/include

LOCAL_MODULE    := libam_adp_adec
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := am_dmx/am_dmx.c am_dmx/linux_dvb/linux_dvb.c\
		   am_fend/am_fend.c am_fend/am_vlfend.c am_fend/am_fend_diseqc_cmd.c am_fend/am_rotor_calc.c am_fend/linux_dvb/linux_dvb.c am_fend/linux_v4l2/linux_v4l2.c\
		   am_av/am_av.c am_av/aml/aml.c\
		   am_dvr/am_dvr.c am_dvr/linux_dvb/linux_dvb.c\
		   am_dmx/dvr/dvr.c\
		   am_aout/am_aout.c\
		   am_vout/am_vout.c\
		   am_vout/aml/aml.c\
		   am_misc/am_adplock.c am_misc/am_misc.c am_misc/am_iconv.c am_misc/am_sig_handler.c\
		   am_time/am_time.c\
		   am_evt/am_evt.c\
		   am_kl/am_kl.c \
		   am_dsc/am_dsc.c am_dsc/aml/aml.c\
		   am_smc/am_smc.c\
		   am_smc/aml/aml.c\
		   am_userdata/am_userdata.c\
		   am_userdata/aml/aml.c\
		   am_userdata/emu/emu.c\
		   am_pes/am_pes.c \
		   am_ad/am_ad.c \
		   am_open_lib/libdvbsi/tables/bat.c\
		   am_open_lib/libdvbsi/tables/sdt.c\
		   am_open_lib/libdvbsi/tables/pat.c\
		   am_open_lib/libdvbsi/tables/cat.c\
		   am_open_lib/libdvbsi/tables/pmt.c\
		   am_open_lib/libdvbsi/tables/tot.c\
		   am_open_lib/libdvbsi/tables/eit.c\
		   am_open_lib/libdvbsi/tables/nit.c\
		   am_open_lib/libdvbsi/tables/atsc_eit.c\
		   am_open_lib/libdvbsi/tables/atsc_ett.c\
		   am_open_lib/libdvbsi/tables/atsc_mgt.c\
		   am_open_lib/libdvbsi/tables/atsc_stt.c\
		   am_open_lib/libdvbsi/tables/atsc_vct.c\
		   am_open_lib/libdvbsi/tables/atsc_cea.c\
		   am_open_lib/libdvbsi/tables/huffman_decode.c\
		   am_open_lib/libdvbsi/demux.c\
		   am_open_lib/libdvbsi/descriptors/dr_0f.c\
		   am_open_lib/libdvbsi/descriptors/dr_44.c\
		   am_open_lib/libdvbsi/descriptors/dr_0a.c\
		   am_open_lib/libdvbsi/descriptors/dr_47.c\
		   am_open_lib/libdvbsi/descriptors/dr_03.c\
		   am_open_lib/libdvbsi/descriptors/dr_5a.c\
		   am_open_lib/libdvbsi/descriptors/dr_05.c\
		   am_open_lib/libdvbsi/descriptors/dr_48.c\
		   am_open_lib/libdvbsi/descriptors/dr_69.c\
		   am_open_lib/libdvbsi/descriptors/dr_02.c\
		   am_open_lib/libdvbsi/descriptors/dr_4d.c\
		   am_open_lib/libdvbsi/descriptors/dr_58.c\
		   am_open_lib/libdvbsi/descriptors/dr_56.c\
		   am_open_lib/libdvbsi/descriptors/dr_4e.c\
		   am_open_lib/libdvbsi/descriptors/dr_4a.c\
		   am_open_lib/libdvbsi/descriptors/dr_45.c\
		   am_open_lib/libdvbsi/descriptors/dr_41.c\
		   am_open_lib/libdvbsi/descriptors/dr_43.c\
		   am_open_lib/libdvbsi/descriptors/dr_0e.c\
		   am_open_lib/libdvbsi/descriptors/dr_04.c\
		   am_open_lib/libdvbsi/descriptors/dr_59.c\
		   am_open_lib/libdvbsi/descriptors/dr_0c.c\
		   am_open_lib/libdvbsi/descriptors/dr_54.c\
		   am_open_lib/libdvbsi/descriptors/dr_09.c\
		   am_open_lib/libdvbsi/descriptors/dr_52.c\
		   am_open_lib/libdvbsi/descriptors/dr_40.c\
		   am_open_lib/libdvbsi/descriptors/dr_55.c\
		   am_open_lib/libdvbsi/descriptors/dr_08.c\
		   am_open_lib/libdvbsi/descriptors/dr_0b.c\
		   am_open_lib/libdvbsi/descriptors/dr_42.c\
		   am_open_lib/libdvbsi/descriptors/dr_07.c\
		   am_open_lib/libdvbsi/descriptors/dr_0d.c\
		   am_open_lib/libdvbsi/descriptors/dr_06.c\
		   am_open_lib/libdvbsi/descriptors/dr_81.c\
		   am_open_lib/libdvbsi/descriptors/dr_83.c\
		   am_open_lib/libdvbsi/descriptors/dr_86.c\
		   am_open_lib/libdvbsi/descriptors/dr_87.c\
		   am_open_lib/libdvbsi/descriptors/dr_87_ca.c\
		   am_open_lib/libdvbsi/descriptors/dr_88.c\
		   am_open_lib/libdvbsi/descriptors/dr_5d.c\
		   am_open_lib/libdvbsi/descriptors/dr_6a.c\
		   am_open_lib/libdvbsi/descriptors/dr_7a.c\
		   am_open_lib/libdvbsi/descriptors/dr_7f.c\
		   am_open_lib/libdvbsi/descriptors/dr_a1.c\
		   am_open_lib/libdvbsi/descriptors/dr_cc.c\
		   am_open_lib/libdvbsi/psi.c\
		   am_open_lib/libdvbsi/dvbpsi.c\
		   am_open_lib/libdvbsi/descriptor.c\
		   am_open_lib/am_ci/libdvben50221/asn_1.c \
		   am_open_lib/am_ci/libdvben50221/en50221_app_ai.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_app_auth.c      \
		   am_open_lib/am_ci/libdvben50221/en50221_app_ca.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_app_datetime.c  \
		   am_open_lib/am_ci/libdvben50221/en50221_app_dvb.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_app_epg.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_app_lowspeed.c  \
		   am_open_lib/am_ci/libdvben50221/en50221_app_mmi.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_app_rm.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_app_smartcard.c \
		   am_open_lib/am_ci/libdvben50221/en50221_app_teletext.c  \
		   am_open_lib/am_ci/libdvben50221/en50221_app_utils.c     \
		   am_open_lib/am_ci/libdvben50221/en50221_session.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_stdcam.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_stdcam_hlci.c   \
		   am_open_lib/am_ci/libdvben50221/en50221_stdcam_llci.c   \
		   am_open_lib/am_ci/libdvben50221/en50221_transport.c \
		   am_open_lib/am_ci/libucsi/dvb/types.c \
		   am_open_lib/am_ci/libdvbapi/dvbca.c \
		   am_open_lib/am_ci/libucsi/mpeg/pmt_section.c \
		   am_open_lib/am_ci/am_ci.c \
		   am_open_lib/am_ci/ca_ci.c \
		   am_open_lib/am_freesat/freesat.c \
		   am_open_lib/am_crypt/am_crypt.c \
		   am_open_lib/am_crypt/des.c \
		   am_tfile/am_tfile.c

LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DCHIP_8226M -DLINUX_DVB_FEND -DLOG_LEVEL=1
ifeq ($(AMLOGIC_LIBPLAYER), y)
LOCAL_CFLAGS+=-DAMLOGIC_LIBPLAYER
endif
LOCAL_CFLAGS+=-std=gnu99

LOCAL_CFLAGS+=-DUSE_ADEC_IN_DVB


LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include/am_adp\
		   $(LOCAL_PATH)/../include/am_adp/libdvbsi\
		   $(LOCAL_PATH)/../include/am_adp/libdvbsi/descriptors\
		   $(LOCAL_PATH)/../include/am_adp/libdvbsi/tables\
		   $(LOCAL_PATH)/am_open_lib/am_ci\
		   $(LOCAL_PATH)/../include/am_mw\
		   $(LOCAL_PATH)/../android/ndk/include\
		   $(AMADEC_C_INCLUDES)\
		   common/include/linux/amlogic

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28&& echo OK),OK)
LOCAL_CFLAGS += -DUSE_VENDOR_ICU
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../icu/icu4c/source/common
LOCAL_SHARED_LIBRARIES+=libicuuc libcutils liblog libdl libc libamadec_system
else
LOCAL_C_INCLUDES += external/icu/icu4c/source/common
LOCAL_SHARED_LIBRARIES+=libicuuc libcutils liblog libdl libc libamadec_system
endif

LOCAL_PRELINK_MODULE := false


#LOCAL_32_BIT_ONLY := true

include $(BUILD_SHARED_LIBRARY)

#########################################################################################################
ifeq ($(BOARD_COMPILE_IN_SYSTEM), true)
include $(CLEAR_VARS)

LOCAL_MODULE    := libam_adp_vendor
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := am_dmx/am_dmx.c am_dmx/linux_dvb/linux_dvb.c\
		   am_fend/am_fend.c am_fend/am_vlfend.c am_fend/am_fend_diseqc_cmd.c am_fend/am_rotor_calc.c am_fend/linux_dvb/linux_dvb.c am_fend/linux_v4l2/linux_v4l2.c\
		   am_av/am_av.c am_av/aml/aml.c\
		   am_dvr/am_dvr.c am_dvr/linux_dvb/linux_dvb.c\
		   am_dmx/dvr/dvr.c\
		   am_aout/am_aout.c\
		   am_vout/am_vout.c\
		   am_vout/aml/aml.c\
		   am_misc/am_adplock.c am_misc/am_misc.c am_misc/am_iconv.c am_misc/am_sig_handler.c\
		   am_time/am_time.c\
		   am_evt/am_evt.c\
		   am_kl/am_kl.c \
		   am_dsc/am_dsc.c am_dsc/aml/aml.c\
		   am_smc/am_smc.c\
		   am_smc/aml/aml.c\
		   am_userdata/am_userdata.c\
		   am_userdata/aml/aml.c\
		   am_userdata/emu/emu.c\
		   am_pes/am_pes.c \
		   am_ad/am_ad.c \
		   am_open_lib/libdvbsi/tables/bat.c\
		   am_open_lib/libdvbsi/tables/sdt.c\
		   am_open_lib/libdvbsi/tables/pat.c\
		   am_open_lib/libdvbsi/tables/cat.c\
		   am_open_lib/libdvbsi/tables/pmt.c\
		   am_open_lib/libdvbsi/tables/tot.c\
		   am_open_lib/libdvbsi/tables/eit.c\
		   am_open_lib/libdvbsi/tables/nit.c\
		   am_open_lib/libdvbsi/tables/atsc_eit.c\
		   am_open_lib/libdvbsi/tables/atsc_ett.c\
		   am_open_lib/libdvbsi/tables/atsc_mgt.c\
		   am_open_lib/libdvbsi/tables/atsc_stt.c\
		   am_open_lib/libdvbsi/tables/atsc_vct.c\
		   am_open_lib/libdvbsi/tables/atsc_cea.c\
		   am_open_lib/libdvbsi/tables/huffman_decode.c\
		   am_open_lib/libdvbsi/demux.c\
		   am_open_lib/libdvbsi/descriptors/dr_0f.c\
		   am_open_lib/libdvbsi/descriptors/dr_44.c\
		   am_open_lib/libdvbsi/descriptors/dr_0a.c\
		   am_open_lib/libdvbsi/descriptors/dr_47.c\
		   am_open_lib/libdvbsi/descriptors/dr_03.c\
		   am_open_lib/libdvbsi/descriptors/dr_5a.c\
		   am_open_lib/libdvbsi/descriptors/dr_05.c\
		   am_open_lib/libdvbsi/descriptors/dr_48.c\
		   am_open_lib/libdvbsi/descriptors/dr_69.c\
		   am_open_lib/libdvbsi/descriptors/dr_02.c\
		   am_open_lib/libdvbsi/descriptors/dr_4d.c\
		   am_open_lib/libdvbsi/descriptors/dr_58.c\
		   am_open_lib/libdvbsi/descriptors/dr_56.c\
		   am_open_lib/libdvbsi/descriptors/dr_4e.c\
		   am_open_lib/libdvbsi/descriptors/dr_4a.c\
		   am_open_lib/libdvbsi/descriptors/dr_45.c\
		   am_open_lib/libdvbsi/descriptors/dr_41.c\
		   am_open_lib/libdvbsi/descriptors/dr_43.c\
		   am_open_lib/libdvbsi/descriptors/dr_0e.c\
		   am_open_lib/libdvbsi/descriptors/dr_04.c\
		   am_open_lib/libdvbsi/descriptors/dr_59.c\
		   am_open_lib/libdvbsi/descriptors/dr_0c.c\
		   am_open_lib/libdvbsi/descriptors/dr_54.c\
		   am_open_lib/libdvbsi/descriptors/dr_09.c\
		   am_open_lib/libdvbsi/descriptors/dr_52.c\
		   am_open_lib/libdvbsi/descriptors/dr_40.c\
		   am_open_lib/libdvbsi/descriptors/dr_55.c\
		   am_open_lib/libdvbsi/descriptors/dr_08.c\
		   am_open_lib/libdvbsi/descriptors/dr_0b.c\
		   am_open_lib/libdvbsi/descriptors/dr_42.c\
		   am_open_lib/libdvbsi/descriptors/dr_07.c\
		   am_open_lib/libdvbsi/descriptors/dr_0d.c\
		   am_open_lib/libdvbsi/descriptors/dr_06.c\
		   am_open_lib/libdvbsi/descriptors/dr_81.c\
		   am_open_lib/libdvbsi/descriptors/dr_83.c\
		   am_open_lib/libdvbsi/descriptors/dr_86.c\
		   am_open_lib/libdvbsi/descriptors/dr_87.c\
		   am_open_lib/libdvbsi/descriptors/dr_87_ca.c\
		   am_open_lib/libdvbsi/descriptors/dr_88.c\
		   am_open_lib/libdvbsi/descriptors/dr_5d.c\
		   am_open_lib/libdvbsi/descriptors/dr_6a.c\
		   am_open_lib/libdvbsi/descriptors/dr_7a.c\
		   am_open_lib/libdvbsi/descriptors/dr_7f.c\
		   am_open_lib/libdvbsi/descriptors/dr_a1.c\
		   am_open_lib/libdvbsi/descriptors/dr_cc.c\
		   am_open_lib/libdvbsi/psi.c\
		   am_open_lib/libdvbsi/dvbpsi.c\
		   am_open_lib/libdvbsi/descriptor.c\
		   am_open_lib/am_ci/libdvben50221/asn_1.c \
		   am_open_lib/am_ci/libdvben50221/en50221_app_ai.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_app_auth.c      \
		   am_open_lib/am_ci/libdvben50221/en50221_app_ca.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_app_datetime.c  \
		   am_open_lib/am_ci/libdvben50221/en50221_app_dvb.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_app_epg.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_app_lowspeed.c  \
		   am_open_lib/am_ci/libdvben50221/en50221_app_mmi.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_app_rm.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_app_smartcard.c \
		   am_open_lib/am_ci/libdvben50221/en50221_app_teletext.c  \
		   am_open_lib/am_ci/libdvben50221/en50221_app_utils.c     \
		   am_open_lib/am_ci/libdvben50221/en50221_session.c       \
		   am_open_lib/am_ci/libdvben50221/en50221_stdcam.c        \
		   am_open_lib/am_ci/libdvben50221/en50221_stdcam_hlci.c   \
		   am_open_lib/am_ci/libdvben50221/en50221_stdcam_llci.c   \
		   am_open_lib/am_ci/libdvben50221/en50221_transport.c \
		   am_open_lib/am_ci/libucsi/dvb/types.c \
		   am_open_lib/am_ci/libdvbapi/dvbca.c \
		   am_open_lib/am_ci/libucsi/mpeg/pmt_section.c \
		   am_open_lib/am_ci/am_ci.c \
		   am_open_lib/am_ci/ca_ci.c \
		   am_open_lib/am_freesat/freesat.c \
		   am_open_lib/am_crypt/am_crypt.c \
		   am_open_lib/am_crypt/des.c \
		   am_tfile/am_tfile.c

LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DCHIP_8226M -DLINUX_DVB_FEND -DLOG_LEVEL=1
ifeq ($(AMLOGIC_LIBPLAYER), y)
LOCAL_CFLAGS+=-DAMLOGIC_LIBPLAYER
endif
LOCAL_CFLAGS+=-std=gnu99

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28&& echo OK),OK)
else
LOCAL_CFLAGS+=-DUSE_ADEC_IN_DVB
endif

LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include/am_adp\
		   $(LOCAL_PATH)/../include/am_adp/libdvbsi\
		   $(LOCAL_PATH)/../include/am_adp/libdvbsi/descriptors\
		   $(LOCAL_PATH)/../include/am_adp/libdvbsi/tables\
		   $(LOCAL_PATH)/am_open_lib/am_ci\
		   $(LOCAL_PATH)/../include/am_mw\
		   $(LOCAL_PATH)/../android/ndk/include\
		   $(AMADEC_C_INCLUDES)\
		   common/include/linux/amlogic

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28&& echo OK),OK)
LOCAL_CFLAGS += -DUSE_VENDOR_ICU
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../icu/icu4c/source/common
LOCAL_SHARED_LIBRARIES+=$(AMADEC_LIBS) libicuuc_vendor libcutils liblog libdl libc
else
LOCAL_C_INCLUDES += external/icu/icu4c/source/common
LOCAL_SHARED_LIBRARIES+=$(AMADEC_LIBS) libicuuc libcutils liblog libdl libc
endif

ifeq ($(SUPPORT_CAS), y)
LOCAL_CFLAGS+=-DSUPPORT_CAS
endif
LOCAL_PRELINK_MODULE := false

LOCAL_PROPRIETARY_MODULE := true

#LOCAL_32_BIT_ONLY := true

include $(BUILD_SHARED_LIBRARY)

endif
