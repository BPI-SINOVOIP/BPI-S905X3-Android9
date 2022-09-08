# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_CFLAGS += -fno-short-enums -DQEMU_HARDWARE
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-missing-field-initializers
LOCAL_CFLAGS += -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

########################################################################################################
CAMHAL_GIT_VERSION="dc5bbc41841cba51165af129e2573cbed05fa15e"
CAMHAL_GIT_UNCOMMIT_FILE_NUM=0
CAMHAL_LAST_CHANGED="Date:   Mon Oct 14 13:35:19 2019 +0800"
CAMHAL_BUILD_TIME=" $(shell date)"
CAMHAL_BUILD_NAME=" $(shell echo ${LOGNAME})"
CAMHAL_BRANCH_NAME="* (no branch)"
CAMHAL_BUILD_MODE=$(shell echo ${TARGET_BUILD_VARIANT})
CAMHAL_HOSTNAME="$(shell hostname)"
CAMHAL_IP="$(shell ifconfig eth0|grep -oE '([0-9]{1,3}\.?){4}'|head -n 1)"
CAMHAL_PATH="$(shell pwd)/$(LOCAL_PATH)"

LOCAL_CFLAGS+=-DHAVE_VERSION_INFO
LOCAL_CFLAGS+=-DCAMHAL_GIT_VERSION=\"${CAMHAL_GIT_VERSION}${CAMHAL_GIT_DIRTY}\"
LOCAL_CFLAGS+=-DCAMHAL_BRANCH_NAME=\"${CAMHAL_BRANCH_NAME}\"
LOCAL_CFLAGS+=-DCAMHAL_LAST_CHANGED=\"${CAMHAL_LAST_CHANGED}\"
LOCAL_CFLAGS+=-DCAMHAL_BUILD_TIME=\"${CAMHAL_BUILD_TIME}\"
LOCAL_CFLAGS+=-DCAMHAL_BUILD_NAME=\"${CAMHAL_BUILD_NAME}\"
LOCAL_CFLAGS+=-DCAMHAL_GIT_UNCOMMIT_FILE_NUM=${CAMHAL_GIT_UNCOMMIT_FILE_NUM}
LOCAL_CFLAGS+=-DCAMHAL_HOSTNAME=\"${CAMHAL_HOSTNAME}\"
LOCAL_CFLAGS+=-DCAMHAL_IP=\"${CAMHAL_IP}\"
LOCAL_CFLAGS+=-DCAMHAL_PATH=\"${CAMHAL_PATH}\"
########################################################################################################
GE2D_ENABLE := false
GE2D_VERSION_2 := false
ISP_ENABLE := false
LOCAL_SHARED_LIBRARIES:= \
    libbinder \
    liblog \
    libutils \
    libcutils \
    libion \
    libui \
    libdl \
    libjpeg \
    libexpat \
    libexif \
    libcamera_metadata \
    libamgralloc_ext \
    libmediandk

ifeq ($(GE2D_ENABLE),true)
ifeq ($(GE2D_VERSION_2),true)
LOCAL_SHARED_LIBRARIES += libge2d-2.0
else
LOCAL_SHARED_LIBRARIES += libge2d
endif
LOCAL_CFLAGS += -DGE2D_ENABLE
endif
ifeq ($(ISP_ENABLE),true)
LOCAL_SHARED_LIBRARIES += libispaaa
LOCAL_CFLAGS += -DISP_ENABLE
endif
LOCAL_STATIC_LIBRARIES := \
    libyuv_static \
    android.hardware.camera.common@1.0-helper

LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

LOCAL_KK=0
ifeq ($(GPU_TYPE),t83x)
LOCAL_KK:=1
endif
ifeq ($(GPU_ARCH),midgard)
LOCAL_KK:=1
endif
ifeq ($(LOCAL_KK),1)
    LOCAL_CFLAGS += -DMALI_AFBC_GRALLOC=1
else
    LOCAL_CFLAGS += -DMALI_AFBC_GRALLOC=0
endif

MESON_GRALLOC_DIR ?= hardware/amlogic/gralloc

LOCAL_C_INCLUDES += external/jpeg \
                    external/jhead/ \
                    frameworks/native/include/media/hardware \
                    $(TOP)/frameworks/native/libs/nativebase/include \
                    $(TOP)/frameworks/native/libs/ui/include \
                    $(TOP)/frameworks/native/libs/arect/include \
                    $(TOP)/frameworks/native/libs/nativewindow/include/ \
                    $(TOP)/frameworks/native/include \
                    $(TOP)/frameworks/native/include/media/openmax \
                    $(TOP)/frameworks/native/libs/binder/include \
                    external/libyuv/files/include/ \
                    $(TOP)/system/core/include \
                    $(TOP)/system/core/libion/include \
                    $(TOP)/system/core/libion/kernel-headers \
                    $(TOP)/$(MESON_GRALLOC_DIR) \
                    $(LOCAL_PATH)/inc \
                    $(call include-path-for, camera) \
                    $(TOP)/external/expat/lib \
                    $(TOP)/external/libexif \
                    $(LOCAL_PATH)/isplib/inc \
                    $(TOP)/frameworks/av/media/ndk/include \
                    $(TOP)/frameworks/av/media/libstagefright/include/media/

ifeq ($(GE2D_ENABLE),true)
ifeq ($(GE2D_VERSION_2),true)
LOCAL_C_INCLUDES += $(TOP)/vendor/common/system/libge2d/v2/include/
else
LOCAL_C_INCLUDES += $(TOP)/vendor/common/system/libge2d/include/
endif
endif

LOCAL_SRC_FILES := \
    EmulatedCameraHal.cpp \
    EmulatedCameraFactory.cpp \
    EmulatedCameraHotplugThread.cpp \
    EmulatedBaseCamera.cpp \
    EmulatedCamera.cpp \
    EmulatedCameraDevice.cpp \
    EmulatedQemuCamera.cpp \
    EmulatedQemuCameraDevice.cpp \
    EmulatedFakeCamera.cpp \
    EmulatedFakeCameraDevice.cpp \
    Converters.cpp \
    PreviewWindow.cpp \
    CallbackNotifier.cpp \
    QemuClient.cpp \
    JpegCompressor.cpp \
    fake-pipeline2/Scene.cpp \
    fake-pipeline2/Sensor.cpp \
    fake-pipeline2/JpegCompressor.cpp \
    fake-pipeline2/NV12_resize.c \
    fake-pipeline2/CameraUtil.cpp \
    EmulatedCamera3.cpp \
    EmulatedFakeCamera3.cpp \
    EmulatedFakeCamera3Info.cpp \
    fake-pipeline2/camera_hw.cpp \
    fake-pipeline2/util.c \
    VendorTags.cpp \
    fake-pipeline2/USBSensor.cpp \
    fake-pipeline2/MIPISensor.cpp \
    fake-pipeline2/OMXDecoder.cpp \
    fake-pipeline2/HalMediaCodec.cpp \
    fake-pipeline2/CameraIO.cpp \
    fake-pipeline2/CameraDevice.cpp

ifeq ($(GE2D_ENABLE),true)
LOCAL_SRC_FILES += fake-pipeline2/ge2d_stream.cpp \
                   fake-pipeline2/ion_if.cpp
endif

ifeq ($(TARGET_PRODUCT),vbox_x86)
LOCAL_MODULE := camera.vbox_x86
else
LOCAL_MODULE:= camera.amlogic
endif

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libispaaa
ifneq ($(ANDROID_BUILD_TYPE),64)
LOCAL_SRC_FILES := isplib/lib/libispaaa.so
else
LOCAL_SRC_FILES := isplib/lib64/libispaaa.so
endif
LOCAL_MODULE_SUFFIX := $(TARGET_SHLIB_SUFFIX)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
ifneq ($(ANDROID_BUILD_TYPE),64)
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/
else
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib64/
endif

include $(BUILD_PREBUILT)

include $(call all-makefiles-under,$(LOCAL_PATH))

$(info ************************************************************)

$(info pdk or not:    $(TARGET_BUILD_PDK))
$(info target board:  $(TARGET_BOARD_PLATFORM))
$(info git version:   $(CAMHAL_GIT_VERSION))
$(info uncomment:     $(CAMHAL_GIT_UNCOMMIT_FILE_NUM) files)
$(info last changed:  $(CAMHAL_LAST_CHANGED))
$(info build time:    $(CAMHAL_BUILD_TIME))
$(info builder name:  $(CAMHAL_BUILD_NAME))
$(info branch name:   $(CAMHAL_BRANCH_NAME))
$(info build mode:    $(CAMHAL_BUILD_MODE))
$(info host name:     $(CAMHAL_HOSTNAME))
$(info host IP:       $(CAMHAL_IP))
$(info path:          $(CAMHAL_PATH))
$(info ************************************************************)
$(shell sleep 1)

#################################################################
ifneq (true,true)

include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_CFLAGS += -fno-short-enums -DQEMU_HARDWARE
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_SHARED_LIBRARIES:= \
    libcutils \
    liblog \
    libskia \
    libandroid_runtime

LOCAL_C_INCLUDES += external/jpeg \
                    external/skia/include/core/ \
                    frameworks/base/core/jni/android/graphics \
                    frameworks/native/include

LOCAL_SRC_FILES := JpegStub.cpp

LOCAL_MODULE := camera.goldfish.jpeg

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

endif # !PDK
