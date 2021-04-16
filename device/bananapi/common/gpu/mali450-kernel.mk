#
# Copyright (C) 2015 The Android Open Source Project
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
#
#
BOARD_VENDOR_KERNEL_MODULES += \
	$(PRODUCT_OUT)/obj/lib_vendor/mali.ko

GPU_TYPE:=mali450
GPU_ARCH:=utgard
GPU_DRV_VERSION?=r8p0
GPU_MODS_OUT:=obj/lib_vendor/

CUSTOM_IMAGE_MODULES += mali

ifeq ($(wildcard $(BOARD_AML_VENDOR_PATH)/gpu/gpu-v2.mk),)
ifeq ($(wildcard vendor/amlogic/gpu/gpu-v2.mk),)
MESON_GPU_DIR=vendor/amlogic/gpu
include vendor/amlogic/gpu/gpu-v2.mk
endif
else
MESON_GPU_DIR=$(BOARD_AML_VENDOR_PATH)/gpu
include $(BOARD_AML_VENDOR_PATH)/gpu/gpu-v2.mk
endif
