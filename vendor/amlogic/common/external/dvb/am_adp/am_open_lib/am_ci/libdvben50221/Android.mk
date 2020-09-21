LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libdvben50221
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := asn_1.c                 \
           en50221_app_ai.c        \
           en50221_app_auth.c      \
           en50221_app_ca.c        \
           en50221_app_datetime.c  \
           en50221_app_dvb.c       \
           en50221_app_epg.c       \
           en50221_app_lowspeed.c  \
           en50221_app_mmi.c       \
           en50221_app_rm.c        \
           en50221_app_smartcard.c \
           en50221_app_teletext.c  \
           en50221_app_utils.c     \
           en50221_session.c       \
           en50221_stdcam.c        \
           en50221_stdcam_hlci.c   \
           en50221_stdcam_llci.c   \
           en50221_transport.c

LOCAL_CFLAGS+= -DLOG_LEVEL=1 # FIXME
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/.. $(LOCAL_PATH)/../../include

LOCAL_SHARED_LIBRARIES += liblog libc

LOCAL_PRELINK_MODULE := false

include $(BUILD_STATIC_LIBRARY)

