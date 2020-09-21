LOCAL_PATH:= $(call my-dir)

#
# libscreencontrolservice
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  IScreenControlService.cpp \
  SysWrite.cpp \
  ScreenManager.cpp

LOCAL_C_INCLUDES:= \
        system/libhidl/transport/include/hidl \
        frameworks/native/include/media/openmax \
        frameworks/av/media/libstagefright/mpeg2ts \
	frameworks/av/media/libstagefright/include \
	frameworks/av/media/libmediaextractor/include \
	system/core/libutils/include \
        frameworks/native/include/media/hardware \
        frameworks/native/include/media/ui \
        frameworks/native/include/media \
        $(BOARD_AML_VENDOR_PATH)/frameworks/av \

LOCAL_SHARED_LIBRARIES := \
        vendor.amlogic.hardware.screencontrol@1.0 \
	libgui \
	libbase \
        libutils \
        libcutils \
        liblog \
        libbinder \
        libandroid \
        libhidlbase \
	libhidltransport \
        libui \
        libhardware \
        libmedia \
	libmediaextractor \
        libmediautils \
        libdl                       \
        libnativewindow             \
        libstagefright              \
        libstagefright_foundation   \

LOCAL_MODULE:= libscreencontrolservice
LOCAL_INIT_RC := screencontrol.rc
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
##########################################################################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES:= \
  ScreenControlClient.cpp

LOCAL_C_INCLUDES += \
  system/libhidl/libhidlcache/include \
  system/libhidl/libhidlmemory/include \
  frameworks/native/include \
  $(BOARD_AML_VENDOR_PATH)/frameworks/services/screencontrol

LOCAL_SHARED_LIBRARIES := \
  android.hidl.memory@1.0 \
  vendor.amlogic.hardware.screencontrol@1.0 \
  libbase \
  libutils \
  libcutils \
  liblog \
  libhidlbase \
  libhidlmemory \
  libhidltransport

LOCAL_MODULE:= libscreencontrolclient

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)
##########################################################################
include $(CLEAR_VARS)
include $(BOARD_AML_MEDIA_HAL_CONFIG)
LOCAL_SRC_FILES:= \
        Media2Ts/esconvertor.cpp     \
        Media2Ts/tspack.cpp    \

LOCAL_C_INCLUDES:= \
        $(TOP)/frameworks/av/media/libstagefright/include/media/stagefright/foundation \
	$(TOP)/frameworks/av/media/libmediaextractor/include \
        $(TOP)/frameworks/av/include/media/stagefright/foundation  \
        $(TOP)/frameworks/av/media/libstagefright/media2ts \
        $(TOP)/frameworks/av/media/libmedia/include            \
        $(TOP)/frameworks/native/include/media/openmax \
        $(TOP)/frameworks/native/include/media/ui \
        $(TOP)/frameworks/native/services/systemwrite \
        system/core/libutils/include \
        $(BOARD_AML_VENDOR_PATH)/frameworks/av \
        $(BOARD_AML_VENDOR_PATH)/frameworks/services/screencontrol \

LOCAL_SHARED_LIBRARIES:= \
        vendor.amlogic.hardware.screencontrol@1.0 \
        libbinder \
        libcutils \
        libgui    \
        libmedia  \
        libstagefright \
        libstagefright_foundation \
        libmedia_omx  \
        libui     \
        libutils  \
        liblog \
        libhidlbase \
        libhidltransport \
        libmediaextractor \
        libscreencontrolservice


LOCAL_MODULE_TAGS := optional
LOCAL_CPPFLAGS += -std=c++14
LOCAL_MODULE:= libstagefright_mediaconvertor

include $(BUILD_SHARED_LIBRARY)
################################################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        Media2Ts/videomediaconvertortest.cpp                 \

LOCAL_C_INCLUDES:= \
        external/skia/include/core \
        external/skia/src/core \
        external/skia/include/utils \
	external/skia/include/config \
        $(TOP)/frameworks/av/media/libstagefright/include \
        $(TOP)/frameworks/av/media/libstagefright/media2ts \
        $(TOP)/frameworks/native/include/media/openmax \
        $(TOP)/frameworks/native/include/media/ui \
        $(TOP)/frameworks/native/services/systemwrite \
	system/core/libutils/include \
        $(BOARD_AML_VENDOR_PATH)/frameworks/av

LOCAL_SHARED_LIBRARIES:= \
	vendor.amlogic.hardware.screencontrol@1.0 \
        libbinder                       \
        libgui                          \
        libmedia                        \
        libstagefright                  \
        libstagefright_foundation       \
        libutils                        \
        libcutils                       \
        libmediaextractor               \
	liblog   \
        libstagefright_mediaconvertor        \
        libscreencontrolservice


LOCAL_MODULE:= videomediaconvertortest

LOCAL_MODULE_TAGS := debug

include $(BUILD_EXECUTABLE)
################################################################################

################################################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        Media2Ts/tspacktest.cpp                 \

LOCAL_C_INCLUDES:= \
        external/skia/include/core \
        external/skia/src/core \
        external/skia/include/utils \
	external/skia/include/config \
        $(TOP)/frameworks/av/media/libstagefright \
        $(TOP)/frameworks/av/media/libstagefright/media2ts \
	frameworks/av/media/libstagefright/include \
	system/core/libutils/include \
        $(TOP)/frameworks/native/include/media/openmax \
        $(TOP)/frameworks/native/services/systemwrite \
        $(TOP)/frameworks/native/include/media/ui \
        $(BOARD_AML_VENDOR_PATH)/frameworks/av

LOCAL_SHARED_LIBRARIES:= \
	vendor.amlogic.hardware.screencontrol@1.0 \
        libbinder                       \
        libgui                          \
        libmedia                        \
        libstagefright                  \
        libstagefright_foundation       \
        libutils                        \
        libcutils                       \
	liblog \
        libstagefright_mediaconvertor   \
        libscreencontrolservice


LOCAL_MODULE:= tspacktest

LOCAL_MODULE_TAGS := debug

include $(BUILD_EXECUTABLE)
################################################################################
###############################################################################
include $(CLEAR_VARS)
include $(BOARD_AML_MEDIA_HAL_CONFIG)
LOCAL_SRC_FILES:= \
	ScreenCatch/ScreenCatch.cpp

LOCAL_C_INCLUDES:= \
	$(TOP)/frameworks/av/media/libmediaextractor/include \
        $(TOP)/frameworks/av/media/libstagefright \
        $(TOP)/frameworks/native/include/media/openmax \
        $(TOP)/frameworks/av/media/libstagefright/mpeg2ts \
        $(TOP)/frameworks/native/services/systemwrite \
        $(TOP)/frameworks/native/include/media/hardware \
        $(BOARD_AML_VENDOR_PATH)/frameworks/av

LOCAL_SHARED_LIBRARIES:= \
        libbinder                       \
        libcutils                       \
        liblog                          \
        libgui                          \
        libmedia                        \
	libmediaextractor \
        libstagefright                  \
        libstagefright_foundation       \
        libui                           \
        libutils                        \
        libscreencontrolservice

LOCAL_MODULE:= libstagefright_screencatch

LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)

############################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        ScreenCatch/rgbtest.cpp

LOCAL_C_INCLUDES:= \
        $(TOP)/frameworks/av/media/libstagefright/screencatch \
        $(TOP)/frameworks/native/include/media/openmax \

LOCAL_C_INCLUDES:= \
	$(TOP)/frameworks/av/media/libmediaextractor/include \
        $(TOP)/frameworks/av/media/libstagefright \
        $(TOP)/frameworks/av/media/libstagefright/media2ts \
        $(TOP)/frameworks/native/include/media/openmax \
        $(TOP)/frameworks/native/services/systemwrite \
        $(BOARD_AML_VENDOR_PATH)/frameworks/av

LOCAL_SHARED_LIBRARIES:= \
        libbinder                       \
        libcutils                       \
        liblog                          \
        libgui                          \
        libmedia                        \
	libmediaextractor \
        libstagefright                  \
        libstagefright_foundation       \
        libui                           \
        libutils                        \
        libstagefright_screencatch \
        libscreencontrolservice

LOCAL_MODULE:= rgbtest

LOCAL_MODULE_TAGS:= debug

include $(BUILD_EXECUTABLE)
############################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        ScreenCatch/pngtest.cpp

LOCAL_C_INCLUDES:= \
        $(TOP)/frameworks/av/media/libstagefright/screencatch \
        $(TOP)/frameworks/native/include/media/openmax \

LOCAL_C_INCLUDES:= \
	$(TOP)/frameworks/av/media/libmediaextractor/include \
        $(TOP)/frameworks/av/media/libstagefright \
        $(TOP)/frameworks/av/media/libstagefright/media2ts \
        $(TOP)/frameworks/native/include/media/openmax \
        $(TOP)/frameworks/native/services/systemwrite \
	external/skia/include/core \
	external/skia/src/core \
	external/skia/include/config \
        $(BOARD_AML_VENDOR_PATH)/frameworks/av

LOCAL_SHARED_LIBRARIES:= \
        libbinder                       \
        libcutils                       \
        liblog                          \
        libgui                          \
	libhwui \
        libmedia                        \
	libmediaextractor \
        libstagefright                  \
        libstagefright_foundation       \
        libui                           \
        libutils                        \
        libstagefright_screencatch \
        libscreencontrolservice

LOCAL_MODULE:= pngtest

LOCAL_MODULE_TAGS := debug

include $(BUILD_EXECUTABLE)
################################################################################
# build for screencontrol
# =========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    ScreenControlHal.cpp \
    main_screencontrol.cpp \
    ScreenControlService.cpp  \

LOCAL_SHARED_LIBRARIES := \
    vendor.amlogic.hardware.screencontrol@1.0 \
    android.hidl.allocator@1.0 \
    android.hidl.memory@1.0 \
    libscreencontrolservice \
    libbinder \
    libhwui \
    libgui \
    libui \
    libhardware \
    libhidlbase \
    libhidltransport \
    libhidlmemory \
    libmedia \
    libmediautils \
    libmediaextractor \
    libcutils                   \
    libutils                    \
    liblog                      \
    libdl                       \
    libstagefright              \
    libstagefright_foundation   \
    libstagefright_mediaconvertor \
    libstagefright_screencatch \

LOCAL_C_INCLUDES += \
    frameworks/av/media/libmediaextractor/include \
    frameworks/av/include/media/stagefright \
    system/libhidl/transport/include/hidl \
    system/core/libutils/include \
    system/core/base/include \
    frameworks/native/include/media/openmax \
    frameworks/av/media/libstagefright/mpeg2ts \
    frameworks/av/media/libstagefright/include \
    frameworks/native/include/media/hardware \
    frameworks/native/include/media/ui \
    external/skia/include/core \
    external/skia/src/core \
    external/skia/include/config \
    $(BOARD_AML_VENDOR_PATH)/frameworks/av \
    system/libhidl/libhidlcache/include \
    system/libhidl/libhidlmemory/include \
    frameworks/native/include \

LOCAL_CPPFLAGS += -std=c++14
LOCAL_MODULE:= screencontrol
LOCAL_32_BIT_ONLY := true

LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

include $(BUILD_EXECUTABLE)
