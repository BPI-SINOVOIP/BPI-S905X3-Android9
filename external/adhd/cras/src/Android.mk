LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	common/cras_audio_format.c \
	common/cras_config.c \
	common/cras_file_wait.c \
	common/cras_util.c \
	common/edid_utils.c \
	libcras/cras_client.c \
	libcras/cras_helpers.c

LOCAL_SHARED_LIBRARIES := \
	libtinyalsa

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/common \
	$(LOCAL_PATH)/libcras \
	external/tinyalsa/include

LOCAL_CFLAGS += \
	-DCRAS_SOCKET_FILE_DIR=\"/var/run/cras\" \
	-Wall \
	-Werror \
	-Wno-error=missing-field-initializers \
	-Wno-sign-compare \
	-Wno-unused-parameter \

LOCAL_MODULE := libcras

include $(BUILD_STATIC_LIBRARY)
