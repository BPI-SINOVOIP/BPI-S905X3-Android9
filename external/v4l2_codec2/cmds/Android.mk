LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        codec2.cpp \

LOCAL_C_INCLUDES += \
        $(TOP)/external/libchrome \
        $(TOP)/external/gtest/include \
        $(TOP)/external/v4l2_codec2/include \
        $(TOP)/external/v4l2_codec2/vda \
        $(TOP)/frameworks/av/media/libstagefright/include \
        $(TOP)/frameworks/native/include \
        $(TOP)/hardware/google/av/codec2/include \
        $(TOP)/hardware/google/av/codec2/vndk/include \
	$(TOP)/hardware/google/av/media/codecs/base/include \

LOCAL_MODULE := v4l2_codec2_testapp
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libbinder \
                          libchrome \
                          libcutils \
                          libgui \
                          liblog \
                          libmedia \
                          libmediaextractor \
                          libstagefright \
                          libstagefright_codec2 \
                          libstagefright_foundation \
                          libstagefright_codec2_vndk \
                          libui \
                          libutils \
                          libv4l2_codec2 \
                          libv4l2_codec2_vda \
                          android.hardware.media.bufferpool@1.0 \

# -Wno-unused-parameter is needed for libchrome/base codes
LOCAL_CFLAGS += -Werror -Wall -Wno-unused-parameter
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)
