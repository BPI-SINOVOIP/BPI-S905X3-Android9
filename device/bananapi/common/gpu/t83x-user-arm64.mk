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

GPU_TARGET_PLATFORM := default_8a
GPU_TYPE:=t83x
GPU_ARCH:=midgard
GPU_DRV_VERSION:=r21p0
GRALLOC_USE_GRALLOC1_API:=1
GRALLOC_DISABLE_FRAMEBUFFER_HAL:=1
BOARD_INSTALL_VULKAN:=false

ifeq ($(GRALLOC_USE_GRALLOC1_API), 1)
PRODUCT_PACKAGES += \
		libamgralloc_ext \
		libamgralloc_ext_static \
		libamgralloc_internal_static
endif

# The OpenGL ES API level that is natively supported by this device.
PRODUCT_PROPERTY_OVERRIDES += \
	ro.opengles.version=196610

PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/android.hardware.opengles.aep.xml:vendor/etc/permissions/android.hardware.opengles.aep.xml
