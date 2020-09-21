#
# Copyright (C) 2013 The Android Open-Source Project
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

# For now this will allow for dynamic start/stop zygote_secondary
# feature in 64-bit primary,32-bit secondary zygote

# Copy dynamic the 64-bit primary, 32-bit secondary zygote startup script
ifneq ($(BOARD_USES_RECOVERY_AS_BOOT), true)
PRODUCT_COPY_FILES += \
    device/amlogic/common/dynamic_zygote_seondary/init.zygote64_32.rc:root/init.zygote64_32.rc
else
PRODUCT_COPY_FILES += \
    device/amlogic/common/dynamic_zygote_seondary/init.zygote64_32.rc:recovery/root/init.zygote64_32.rc
endif

# Set the zygote property to select the 64-bit primary, 32-bit secondary script
# This line must be parsed before the one in core_minimal.mk
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.zygote=zygote64_32

TARGET_SUPPORTS_32_BIT_APPS := true
TARGET_SUPPORTS_64_BIT_APPS := true

# Set the property to enable dynamic start/stop zygote_secondary feature
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.dynamic.zygote_secondary=enable

# Compile zygote secondary proxy
PRODUCT_PACKAGES += \
    zygote_proxy
