# Build only if both hardware/google/av and device/google/cheets2/codec2 are
# visible; otherwise, don't build any target under this repository.
ifneq (,$(findstring hardware/google/av,$(PRODUCT_SOONG_NAMESPACES)))
ifneq (,$(findstring device/google/cheets2/codec2,$(PRODUCT_SOONG_NAMESPACES)))

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        C2VDAComponent.cpp \
        C2VDAAdaptor.cpp   \

LOCAL_C_INCLUDES += \
        $(TOP)/device/google/cheets2/codec2/vdastore/include \
        $(TOP)/external/libchrome \
        $(TOP)/external/gtest/include \
        $(TOP)/external/v4l2_codec2/include \
        $(TOP)/external/v4l2_codec2/vda \
        $(TOP)/frameworks/av/media/libstagefright/include \
        $(TOP)/hardware/google/av/codec2/include \
        $(TOP)/hardware/google/av/codec2/vndk/include \
        $(TOP)/hardware/google/av/media/codecs/base/include \

LOCAL_MODULE:= libv4l2_codec2
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libbinder \
                          libchrome \
                          liblog \
                          libmedia \
                          libstagefright \
                          libstagefright_codec2 \
                          libstagefright_codec2_vndk \
                          libstagefright_simple_c2component \
                          libstagefright_foundation \
                          libutils \
                          libv4l2_codec2_vda \
                          libvda_c2componentstore \

# -Wno-unused-parameter is needed for libchrome/base codes
LOCAL_CFLAGS += -Werror -Wall -Wno-unused-parameter -std=c++14
LOCAL_CFLAGS += -Wno-unused-lambda-capture -Wno-unknown-warning-option
LOCAL_CLANG := true
LOCAL_SANITIZE := unsigned-integer-overflow signed-integer-overflow

LOCAL_LDFLAGS := -Wl,-Bsymbolic

# Build C2VDAAdaptorProxy only for ARC++ case.
ifneq (,$(findstring cheets_,$(TARGET_PRODUCT)))
LOCAL_CFLAGS += -DV4L2_CODEC2_ARC
LOCAL_SRC_FILES += \
                   C2VDAAdaptorProxy.cpp \

LOCAL_SRC_FILES := $(filter-out C2VDAAdaptor.cpp, $(LOCAL_SRC_FILES))
LOCAL_SHARED_LIBRARIES += libarcbridge \
                          libarcbridgeservice \
                          libmojo \
                          libv4l2_codec2_arcva_factory \

endif # ifneq (,$(findstring cheets_,$(TARGET_PRODUCT)))

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))

endif  #ifneq (,$(findstring device/google/cheets2/codec2,$(PRODUCT_SOONG_NAMESPACES)))
endif  #ifneq (,$(findstring hardware/google/av,$(PRODUCT_SOONG_NAMESPACES)))
