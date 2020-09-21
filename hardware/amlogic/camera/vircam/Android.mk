ifneq ( true, true)
ifneq ($(strip $(USE_CAMERA_STUB)),true)

LOCAL_PATH:= $(call my-dir)

CAMERA_HAL_SRC := \
	CameraHal_Module.cpp \
	CameraHal.cpp \
	CameraHalUtilClasses.cpp \
	AppCallbackNotifier.cpp \
	ANativeWindowDisplayAdapter.cpp \
	CameraProperties.cpp \
	MemoryManager.cpp \
	Encoder_libjpeg.cpp \
	SensorListener.cpp  \
	NV12_resize.c

CAMERA_COMMON_SRC:= \
	CameraParameters.cpp \
	ExCameraParameters.cpp \
	CameraHalCommon.cpp

CAMERA_V4L_SRC:= \
	BaseCameraAdapter.cpp \
	V4LCameraAdapter/V4LCameraAdapter.cpp

CAMERA_UTILS_SRC:= \
	utils/ErrorUtils.cpp \
	utils/MessageQueue.cpp \
	utils/Semaphore.cpp \
	utils/util.cpp

CAMERA_HAL_VERTURAL_CAMERA_SRC:= \
	VirtualCamHal.cpp \
	AppCbNotifier.cpp \
	V4LCamAdpt.cpp

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	$(CAMERA_HAL_SRC) \
	$(CAMERA_V4L_SRC) \
	$(CAMERA_COMMON_SRC) \
	$(CAMERA_UTILS_SRC)

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/inc/ \
    $(LOCAL_PATH)/utils \
    $(LOCAL_PATH)/inc/V4LCameraAdapter \
    frameworks/native/include/ui \
    frameworks/native/include/utils \
    frameworks/base/include/media/stagefright \
    external/jhead/ \
    external/jpeg/ \
    hardware/libhardware/modules/gralloc/    \
    frameworks/native/include/media/hardware


LOCAL_SHARED_LIBRARIES:= \
    libui \
    libbinder \
    libutils \
    libcutils \
    libcamera_client \
    libexif \
    libjpeg \
    libgui

LOCAL_CFLAGS := -fno-short-enums -DCOPY_IMAGE_BUFFER

ifeq ($(BOARD_HAVE_FRONT_CAM),true)
    LOCAL_CFLAGS += -DAMLOGIC_FRONT_CAMERA_SUPPORT
endif

ifeq ($(BOARD_HAVE_BACK_CAM),true)
    LOCAL_CFLAGS += -DAMLOGIC_BACK_CAMERA_SUPPORT
endif

ifeq ($(IS_CAM_NONBLOCK),true)
LOCAL_CFLAGS += -DAMLOGIC_CAMERA_NONBLOCK_SUPPORT
endif

ifeq ($(BOARD_USE_USB_CAMERA),true)
    LOCAL_CFLAGS += -DAMLOGIC_USB_CAMERA_SUPPORT
#descrease the number of camera captrue frames,and let skype run smoothly
ifeq ($(BOARD_USB_CAMREA_DECREASE_FRAMES), true)
	LOCAL_CFLAGS += -DAMLOGIC_USB_CAMERA_DECREASE_FRAMES
endif
ifeq ($(BOARD_USBCAM_IS_TWOCH),true)
    LOCAL_CFLAGS += -DAMLOGIC_TWO_CH_UVC
endif
else
    ifeq ($(BOARD_HAVE_MULTI_CAMERAS),true)
        LOCAL_CFLAGS += -DAMLOGIC_MULTI_CAMERA_SUPPORT
    endif
    ifeq ($(BOARD_HAVE_FLASHLIGHT),true)
        LOCAL_CFLAGS += -DAMLOGIC_FLASHLIGHT_SUPPORT
    endif
endif

ifeq ($(BOARD_ENABLE_VIDEO_SNAPSHOT),true)
    LOCAL_CFLAGS += -DAMLOGIC_ENABLE_VIDEO_SNAPSHOT
endif

ifeq ($(BOARD_HAVE_VIRTUAL_CAMERA),true)
    LOCAL_CFLAGS += -DAMLOGIC_VIRTUAL_CAMERA_SUPPORT
    LOCAL_SRC_FILES+= \
	$(CAMERA_HAL_VERTURAL_CAMERA_SRC)
endif

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE:= camera.amlogic
LOCAL_MODULE_TAGS:= optional

#include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)
include $(BUILD_SHARED_LIBRARY)
endif
endif
