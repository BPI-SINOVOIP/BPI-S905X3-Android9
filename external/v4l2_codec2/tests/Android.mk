# Build the unit tests.
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_MODULE := C2VDACompIntf_test

LOCAL_MODULE_TAGS := tests

LOCAL_SRC_FILES := \
  C2VDACompIntf_test.cpp \

LOCAL_SHARED_LIBRARIES := \
  libchrome \
  libcutils \
  liblog \
  libstagefright_codec2 \
  libstagefright_codec2_vndk \
  libutils \
  libv4l2_codec2 \
  libv4l2_codec2_vda \

LOCAL_C_INCLUDES += \
  $(TOP)/external/v4l2_codec2/include \
  $(TOP)/external/v4l2_codec2/vda \
  $(TOP)/hardware/google/av/codec2/include \
  $(TOP)/hardware/google/av/codec2/vndk/include \
  $(TOP)/hardware/google/av/media/codecs/base/include \

LOCAL_CFLAGS += -Werror -Wall -std=c++14
LOCAL_CLANG := true

LOCAL_LDFLAGS := -Wl,-Bsymbolic

include $(BUILD_NATIVE_TEST)


include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_MODULE := C2VDAComponent_test

LOCAL_MODULE_TAGS := tests

LOCAL_SRC_FILES := \
  C2VDAComponent_test.cpp \

LOCAL_SHARED_LIBRARIES := \
  libchrome \
  libcutils \
  liblog \
  libmedia \
  libmediaextractor \
  libstagefright \
  libstagefright_codec2 \
  libstagefright_codec2_vndk \
  libstagefright_foundation \
  libutils \
  libv4l2_codec2 \
  libv4l2_codec2_vda \
  android.hardware.media.bufferpool@1.0 \

LOCAL_C_INCLUDES += \
  $(TOP)/external/libchrome \
  $(TOP)/external/v4l2_codec2/include \
  $(TOP)/external/v4l2_codec2/vda \
  $(TOP)/frameworks/av/media/libstagefright/include \
  $(TOP)/hardware/google/av/codec2/include \
  $(TOP)/hardware/google/av/codec2/vndk/include \
  $(TOP)/hardware/google/av/media/codecs/base/include \

# -Wno-unused-parameter is needed for libchrome/base codes
LOCAL_CFLAGS += -Werror -Wall -Wno-unused-parameter -std=c++14
LOCAL_CLANG := true

LOCAL_LDFLAGS := -Wl,-Bsymbolic

include $(BUILD_NATIVE_TEST)
