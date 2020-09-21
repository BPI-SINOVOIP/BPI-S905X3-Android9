LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    src/DeviceDescriptor.cpp \
    src/AudioGain.cpp \
    src/HwModule.cpp \
    src/IOProfile.cpp \
    src/AudioPort.cpp \
    src/AudioProfile.cpp \
    src/AudioRoute.cpp \
    src/AudioPolicyMix.cpp \
    src/AudioPatch.cpp \
    src/AudioInputDescriptor.cpp \
    src/AudioOutputDescriptor.cpp \
    src/AudioCollections.cpp \
    src/EffectDescriptor.cpp \
    src/SoundTriggerSession.cpp \
    src/SessionRoute.cpp \
    src/AudioSourceDescriptor.cpp \
    src/VolumeCurve.cpp \
    src/TypeConverter.cpp \
    src/AudioSession.cpp

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libmedia \
    libutils \
    liblog \

LOCAL_EXPORT_SHARED_LIBRARY_HEADERS := libmedia

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    frameworks/av/services/audiopolicy/common/include \
    frameworks/av/services/audiopolicy \
    frameworks/av/services/audiopolicy/utilities \
    system/media/audio_utils/include \

ifeq ($(USE_XML_AUDIO_POLICY_CONF), 1)

LOCAL_SRC_FILES += src/Serializer.cpp

LOCAL_SHARED_LIBRARIES += libicuuc libxml2

LOCAL_C_INCLUDES += \
    external/libxml2/include \
    external/icu/icu4c/source/common

else

LOCAL_SRC_FILES += \
    src/ConfigParsingUtils.cpp \
    src/StreamDescriptor.cpp \
    src/Gains.cpp

endif #ifeq ($(USE_XML_AUDIO_POLICY_CONF), 1)

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/include

LOCAL_MULTILIB := $(AUDIOSERVER_MULTILIB)

LOCAL_CFLAGS := -Wall -Werror

LOCAL_MODULE := libaudiopolicycomponents

include $(BUILD_STATIC_LIBRARY)
