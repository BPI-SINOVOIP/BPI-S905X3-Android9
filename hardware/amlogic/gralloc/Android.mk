# 
# Copyright (C) 2016-2017 ARM Limited. All rights reserved.
# 
# Copyright (C) 2008 The Android Open Source Project
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

# Include platform specific makefiles
include $(if $(wildcard $(LOCAL_PATH)/Android.$(TARGET_BOARD_PLATFORM).mk), $(LOCAL_PATH)/Android.$(TARGET_BOARD_PLATFORM).mk,)

#
# Static hardware defines
#
# These defines are used in case runtime detection does not find the
# user-space driver to read out hardware capabilities

# GPU support for AFBC 1.0
MALI_GPU_SUPPORT_AFBC_BASIC?=0
# GPU support for AFBC 1.1 block split
MALI_GPU_SUPPORT_AFBC_SPLITBLK?=0
# GPU support for AFBC 1.1 wide block
MALI_GPU_SUPPORT_AFBC_WIDEBLK?=0
# GPU support for AFBC 1.2 tiled headers
MALI_GPU_SUPPORT_AFBC_TILED_HEADERS?=0
# GPU support YUV AFBC formats in wide block
MALI_GPU_USE_YUV_AFBC_WIDEBLK?=0

# VPU version we support
MALI_VIDEO_VERSION?=0
# DPU version we support
MALI_DISPLAY_VERSION?=0

#
# Software behaviour defines
#

# Gralloc1 support
GRALLOC_USE_GRALLOC1_API?=1
# Use ION DMA heap for all allocations. Default is system heap.
GRALLOC_USE_ION_DMA_HEAP?=1
# Use ION Compound heap for all allocations. Default is system heap.
GRALLOC_USE_ION_COMPOUND_PAGE_HEAP?=0
# Properly initializes an empty AFBC buffer
GRALLOC_INIT_AFBC?=0
# fbdev bitdepth to use
GRALLOC_DEPTH?=GRALLOC_32_BITS
# When enabled, forces display framebuffer format to BGRA_8888
GRALLOC_FB_SWAP_RED_BLUE?=1
# Disables the framebuffer HAL device. When a hwc impl is available.
GRALLOC_DISABLE_FRAMEBUFFER_HAL?=1
# When enabled, buffers will never be allocated with AFBC
GRALLOC_ARM_NO_EXTERNAL_AFBC?=0
# Minimum buffer dimensions in pixels when buffer will use AFBC
GRALLOC_DISP_W?=0
GRALLOC_DISP_H?=0
# Vsync backend(not used)
GRALLOC_VSYNC_BACKEND?=default

# HAL module implemenation, not prelinked and stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)
include $(BUILD_SYSTEM)/version_defaults.mk

ifneq ($(findstring $(GPU_ARCH), midgard bifrost),)
	GRALLOC_INIT_AFBC = 1
endif
$(warning "the value of GPU_ARCH is $(GPU_ARCH)")
ifeq ($(TARGET_BOARD_PLATFORM), juno)
ifeq ($(MALI_MMSS), 1)

# Use latest default MMSS build configuration if not already defined
ifeq ($(MALI_DISPLAY_VERSION), 0)
MALI_DISPLAY_VERSION = 650
endif
ifeq ($(MALI_VIDEO_VERSION), 0)
MALI_VIDEO_VERSION = 550
endif

GRALLOC_FB_SWAP_RED_BLUE = 0
GRALLOC_USE_ION_DMA_HEAP = 1
endif
endif

ifeq ($(TARGET_BOARD_PLATFORM), armboard_v7a)
ifeq ($(GRALLOC_MALI_DP),true)
	GRALLOC_FB_SWAP_RED_BLUE = 0
	GRALLOC_DISABLE_FRAMEBUFFER_HAL=1
	MALI_DISPLAY_VERSION = 550
	GRALLOC_USE_ION_DMA_HEAP=1
endif
endif

ifneq ($(MALI_DISPLAY_VERSION), 0)
#if Mali display is available, should disable framebuffer HAL
GRALLOC_DISABLE_FRAMEBUFFER_HAL := 1
#if Mali display is available, AFBC buffers should be initialised after allocation
GRALLOC_INIT_AFBC := 1
endif

ifeq ($(GRALLOC_USE_ION_DMA_HEAP), 1)
ifeq ($(GRALLOC_USE_ION_COMPOUND_PAGE_HEAP), 1)
$(error GRALLOC_USE_ION_DMA_HEAP and GRALLOC_USE_ION_COMPOUND_PAGE_HEAP can't be enabled at the same time)
endif
endif

PLATFORM_SDK_GREATER_THAN_24 := $(shell expr $(PLATFORM_SDK_VERSION) \> 24)

ifeq ($(PLATFORM_SDK_GREATER_THAN_24), 1)
ifeq ($(GRALLOC_EXPERIMENTAL), 1)
	GRALLOC_USE_GRALLOC1_API := 1
endif
endif

LOCAL_C_INCLUDES := $(MALI_LOCAL_PATH) $(MALI_DDK_INCLUDES)

# General compilation flags
LOCAL_CFLAGS := -Werror -DLOG_TAG=\"gralloc\" -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

# Amlogic extend
LOCAL_CFLAGS += -DGRALLOC_AML_EXTEND
BOARD_RESOLUTION_RATIO ?= 1080
LOCAL_CFLAGS += -DBOARD_RESOLUTION_RATIO=$(BOARD_RESOLUTION_RATIO)
$(warning "the value of BOARD_RESOLUTION_RATIO is $(BOARD_RESOLUTION_RATIO)")

ifeq ($(TARGET_APP_LAYER_USE_CONTINUOUS_BUFFER),true)
LOCAL_CFLAGS += -DAML_ALLOC_SCANOUT_FOR_COMPOSE
endif

$(warning "the value of HWC_PRIMARY_DISP_SUPPORT_AFBC is $(HWC_PRIMARY_DISP_SUPPORT_AFBC)")
$(warning "the value of HWC_EXTEND_DISP_SUPPORT_AFBC is $(HWC_EXTEND_DISP_SUPPORT_AFBC)")
ifneq ($(HWC_PRIMARY_DISP_SUPPORT_AFBC),)
LOCAL_CFLAGS += -DPRIMARY_DISP_SUPPORT_AFBC=$(HWC_PRIMARY_DISP_SUPPORT_AFBC)
endif

ifneq ($(HWC_EXTEND_DISP_SUPPORT_AFBC),)
LOCAL_CFLAGS += -DEXTEND_DISP_SUPPORT_AFBC=$(HWC_EXTEND_DISP_SUPPORT_AFBC)
endif

ifeq ($(GPU_TYPE), mali450)
LOCAL_CFLAGS += -DGPU_FORMAT_LIMIT=1
endif
$(warning "the value of GPU_TYPE is $(GPU_TYPE)")

# Static hw flags
LOCAL_CFLAGS += -DMALI_GPU_SUPPORT_AFBC_BASIC=$(MALI_GPU_SUPPORT_AFBC_BASIC)
LOCAL_CFLAGS += -DMALI_GPU_SUPPORT_AFBC_SPLITBLK=$(MALI_GPU_SUPPORT_AFBC_SPLITBLK)
LOCAL_CFLAGS += -DMALI_GPU_SUPPORT_AFBC_WIDEBLK=$(MALI_GPU_SUPPORT_AFBC_WIDEBLK)
LOCAL_CFLAGS += -DMALI_GPU_USE_YUV_AFBC_WIDEBLK=$(MALI_GPU_USE_YUV_AFBC_WIDEBLK)
LOCAL_CFLAGS += -DMALI_GPU_SUPPORT_AFBC_TILED_HEADERS=$(MALI_GPU_SUPPORT_AFBC_TILED_HEADERS)

LOCAL_CFLAGS += -DMALI_DISPLAY_VERSION=$(MALI_DISPLAY_VERSION)
LOCAL_CFLAGS += -DMALI_VIDEO_VERSION=$(MALI_VIDEO_VERSION)

# Software behaviour flags
LOCAL_CFLAGS += -DGRALLOC_USE_GRALLOC1_API=$(GRALLOC_USE_GRALLOC1_API)
LOCAL_CFLAGS += -DGRALLOC_DISP_W=$(GRALLOC_DISP_W)
LOCAL_CFLAGS += -DGRALLOC_DISP_H=$(GRALLOC_DISP_H)
LOCAL_CFLAGS += -DDISABLE_FRAMEBUFFER_HAL=$(GRALLOC_DISABLE_FRAMEBUFFER_HAL)
LOCAL_CFLAGS += -DGRALLOC_USE_ION_DMA_HEAP=$(GRALLOC_USE_ION_DMA_HEAP)
LOCAL_CFLAGS += -DGRALLOC_USE_ION_COMPOUND_PAGE_HEAP=$(GRALLOC_USE_ION_COMPOUND_PAGE_HEAP)
LOCAL_CFLAGS += -DGRALLOC_INIT_AFBC=$(GRALLOC_INIT_AFBC)
LOCAL_CFLAGS += -D$(GRALLOC_DEPTH)
LOCAL_CFLAGS += -DGRALLOC_FB_SWAP_RED_BLUE=$(GRALLOC_FB_SWAP_RED_BLUE)
LOCAL_CFLAGS += -DGRALLOC_ARM_NO_EXTERNAL_AFBC=$(GRALLOC_ARM_NO_EXTERNAL_AFBC)
LOCAL_CFLAGS += -DGRALLOC_LIBRARY_BUILD=1

LOCAL_SHARED_LIBRARIES := libhardware liblog libcutils libGLESv1_CM libsync libutils
LOCAL_STATIC_LIBRARIES := libamgralloc_internal_static

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_RELATIVE_PATH := hw
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
else
ifneq ($(TARGET_2ND_ARCH),)
LOCAL_MODULE_RELATIVE_PATH := hw
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
endif

LOCAL_MODULE := gralloc.amlogic

LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both

LOCAL_C_INCLUDES += system/core/libion/include \
	system/core/libion/kernel-headers \
	system/core/libion/ \
	system/core/libcutils/include

LOCAL_SRC_FILES := \
	ion_wrapper.cpp \
	mali_gralloc_module.cpp \
	framebuffer_device.cpp \
	gralloc_buffer_priv.cpp \
	gralloc_vsync_${GRALLOC_VSYNC_BACKEND}.cpp \
	mali_gralloc_bufferaccess.cpp \
	mali_gralloc_bufferallocation.cpp \
	mali_gralloc_bufferdescriptor.cpp \
	mali_gralloc_ion.cpp \
	mali_gralloc_formats.cpp \
	mali_gralloc_reference.cpp \
	mali_gralloc_debug.cpp

ifeq ($(GRALLOC_USE_GRALLOC1_API), 1)
LOCAL_SRC_FILES += \
	mali_gralloc_public_interface.cpp \
	mali_gralloc_private_interface.cpp
else
LOCAL_SRC_FILES += legacy/alloc_device.cpp
endif

LOCAL_MODULE_OWNER := arm

include $(BUILD_SHARED_LIBRARY)

# Amlogic usage & flags api.
include $(LOCAL_PATH)/amlogic/Android.mk
