LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:=   decode.c  \
				    ../../sliced_vbi.c    \
				    ../../ntsc_dmx/am_vbi_dmx.c \
				    ../../ntsc_dmx/linux_vbi/linux_ntsc.c  
					
LOCAL_SHARED_LIBRARIES+= libzvbi 

LOCAL_C_INCLUDES:=$(LOCAL_PATH)/../../../src \
				  $(LOCAL_PATH)/../../    \
				  $(LOCAL_PATH)/../../ntsc_dmx/  \
				  $(LOCAL_PATH)/../../ntsc_dmx/include  
				  
LOCAL_MODULE:= libntsc_decode
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm
LOCAL_CFLAGS+=-D_REENTRANT -D_GNU_SOURCE -DENABLE_DVB=1 -DENABLE_V4L=1 -DENABLE_V4L2=1 -DHAVE_ICONV=1 -DPACKAGE=\"zvbi\" -DVERSION=\"0.2.33\" -DANDROID -DHAVE_GETOPT_LONG=1
LOCAL_SHARED_LIBRARIES += libicuuc liblog
include $(BUILD_SHARED_LIBRARY)