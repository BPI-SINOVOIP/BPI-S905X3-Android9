LOCAL_PATH:= $(call my-dir)

# The wrapper -DBUILD_FLOAT needs to match
# the lvm library -DBUILD_FLOAT.

# music bundle wrapper
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES:= \
	Bundle/EffectBundle.cpp

LOCAL_CFLAGS += -fvisibility=hidden -DBUILD_FLOAT -DHIGHER_FS
LOCAL_CFLAGS += -Wall -Werror

LOCAL_MODULE:= libbundlewrapper

LOCAL_MODULE_RELATIVE_PATH := soundfx

LOCAL_STATIC_LIBRARIES += libmusicbundle

LOCAL_SHARED_LIBRARIES := \
     libaudioutils \
     libcutils \
     libdl \
     liblog \

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/Bundle \
	$(LOCAL_PATH)/../lib/Common/lib/ \
	$(LOCAL_PATH)/../lib/Bundle/lib/ \
	$(call include-path-for, audio-effects) \
	$(call include-path-for, audio-utils) \

LOCAL_HEADER_LIBRARIES += libhardware_headers
include $(BUILD_SHARED_LIBRARY)


# reverb wrapper
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES:= \
    Reverb/EffectReverb.cpp

LOCAL_CFLAGS += -fvisibility=hidden -DBUILD_FLOAT -DHIGHER_FS
LOCAL_CFLAGS += -Wall -Werror

LOCAL_MODULE:= libreverbwrapper

LOCAL_MODULE_RELATIVE_PATH := soundfx

LOCAL_STATIC_LIBRARIES += libreverb

LOCAL_SHARED_LIBRARIES := \
     libaudioutils \
     libcutils \
     libdl \
     liblog \

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/Reverb \
    $(LOCAL_PATH)/../lib/Common/lib/ \
    $(LOCAL_PATH)/../lib/Reverb/lib/ \
    $(call include-path-for, audio-effects) \
    $(call include-path-for, audio-utils) \

LOCAL_HEADER_LIBRARIES += libhardware_headers

LOCAL_SANITIZE := integer_overflow

include $(BUILD_SHARED_LIBRARY)
