LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libsubjni
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := sub_teletextdec.c bprint.c sub_jni.c sub_api.c log_print.c sub_subtitle.c sub_vob_sub.c sub_set_sys.c vob_sub.c sub_pgs_sub.c sub_control.c avi_sub.c sub_dvb_sub.c amsysfsutils.c Amsyswrite.cpp MemoryLeakTrackUtilTmp.cpp sub_socket.cpp sub_io.cpp
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(JNI_H_INCLUDE) \
    $(BOARD_AML_VENDOR_PATH)/frameworks/services/systemcontrol/pq/include \
    $(BOARD_AML_VENDOR_PATH)/frameworks/services \
    libnativehelper/include/nativehelper \
    frameworks/native/include \
    vendor/amlogic/common/external/libzvbi/src
LOCAL_C_INCLUDES += \
  bionic/libc/include \
  external/skia/include/core \
  external/skia/include/config \
  libnativehelper/include_jni

LOCAL_C_INCLUDES += vendor/amlogic/common/frameworks/services/systemcontrol

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_PRODUCT_MODULE := true
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_SHARED_LIBRARIES += libutils  libcutils liblog libicuuc libicui18n
LOCAL_STATIC_LIBRARIES += \
  libzvbi
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
