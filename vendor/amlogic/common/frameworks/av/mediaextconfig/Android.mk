LOCAL_PATH:= $(call my-dir)

ifeq ($(BOARD_USE_CUSTOM_MEDIASERVEREXTENSIONS),true)
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
	register.cpp \
	AmSharedLibrary.cpp\
	AmLoadAmlogicPlayers.cpp\
	AmSupportModules.cpp\


LOCAL_SHARED_LIBRARIES := \
        libbinder \
        libcutils \
        libdl \
        libutils \
        liblog \


LOCAL_C_INCLUDES+= \
	$(TOP)/frameworks/av/media/libstagefright/include  \
	$(LOCAL_PATH)/include/

LOCAL_MODULE := libregistermsext
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Werror -Wall
include $(BUILD_STATIC_LIBRARY)
endif


