LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    AudioMixer.cpp.arm \
    AudioResampler.cpp.arm \
    AudioResamplerCubic.cpp.arm \
    AudioResamplerSinc.cpp.arm \
    AudioResamplerDyn.cpp.arm \
    BufferProviders.cpp \
    RecordBufferConverter.cpp \

LOCAL_C_INCLUDES := \
    $(TOP) \
    $(call include-path-for, audio-utils) \
    $(LOCAL_PATH)/include \

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_SHARED_LIBRARIES := \
    libaudiohal \
    libaudioutils \
    libcutils \
    liblog \
    libnbaio \
    libnblog \
    libsonic \
    libutils \

LOCAL_MODULE := libaudioprocessing

LOCAL_CFLAGS := -Werror -Wall

# uncomment to disable NEON on architectures that actually do support NEON, for benchmarking
#LOCAL_CFLAGS += -DUSE_NEON=false

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
