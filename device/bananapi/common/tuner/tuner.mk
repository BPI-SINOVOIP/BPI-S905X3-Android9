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

#r840 tuner
ifeq ($(TUNER_MODULE), r840)
ifeq ($(KERNEL_A32_SUPPORT), true)
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/32/r840_fe_32.ko:$(PRODUCT_OUT)/obj/lib_vendor/r840_fe.ko
else
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/64/r840_fe_64.ko:$(PRODUCT_OUT)/obj/lib_vendor/r840_fe.ko
endif
endif

#r842 tuner
ifeq ($(TUNER_MODULE), r842)
ifeq ($(KERNEL_A32_SUPPORT), true)
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/32/r842_fe_32.ko:$(PRODUCT_OUT)/obj/lib_vendor/r842_fe.ko
else
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/64/r842_fe_64.ko:$(PRODUCT_OUT)/obj/lib_vendor/r842_fe.ko
endif
endif

#si2151 tuner
ifeq ($(TUNER_MODULE), si2151)
ifeq ($(KERNEL_A32_SUPPORT), true)
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/32/si2151_fe_32.ko:$(PRODUCT_OUT)/obj/lib_vendor/si2151_fe.ko
else
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/64/si2151_fe_64.ko:$(PRODUCT_OUT)/obj/lib_vendor/si2151_fe.ko
endif
endif

#si2159 tuner
ifeq ($(TUNER_MODULE), si2159)
ifeq ($(KERNEL_A32_SUPPORT), true)
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/32/si2159_fe_32.ko:$(PRODUCT_OUT)/obj/lib_vendor/si2159_fe.ko
else
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/64/si2159_fe_64.ko:$(PRODUCT_OUT)/obj/lib_vendor/si2159_fe.ko
endif
endif

#mxl661 tuner
ifeq ($(TUNER_MODULE), mxl661)
ifeq ($(KERNEL_A32_SUPPORT), true)
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/32/mxl661_fe_32.ko:$(PRODUCT_OUT)/obj/lib_vendor/mxl661_fe.ko
else
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/64/mxl661_fe_64.ko:$(PRODUCT_OUT)/obj/lib_vendor/mxl661_fe.ko
endif
endif

#si2168 tuner
ifeq ($(TUNER_MODULE), si2168)
ifeq ($(KERNEL_A32_SUPPORT), true)
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/32/si2168_fe_32.ko:$(PRODUCT_OUT)/obj/lib_vendor/si2168_fe.ko
else
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/64/si2168_fe_64.ko:$(PRODUCT_OUT)/obj/lib_vendor/si2168_fe.ko
endif
endif

#atbm8881
ifeq ($(TUNER_MODULE), atbm8881)
ifeq ($(KERNEL_A32_SUPPORT), true)
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/32/atbm8881_fe_32.ko:$(PRODUCT_OUT)/obj/lib_vendor/atbm8881_fe.ko
else
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/64/atbm8881_fe_64.ko:$(PRODUCT_OUT)/obj/lib_vendor/atbm8881_fe.ko
endif
endif

#avl6762 tuner
ifeq ($(TUNER_MODULE), avl6762)
ifeq ($(KERNEL_A32_SUPPORT), true)
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/32/avl6762_fe_32.ko:$(PRODUCT_OUT)/obj/lib_vendor/avl6762_fe.ko
else
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/64/avl6762_fe_64.ko:$(PRODUCT_OUT)/obj/lib_vendor/avl6762_fe.ko
endif
endif

#atbm2040 tuner
ifeq ($(TUNER_MODULE), atbm2040)
ifeq ($(KERNEL_A32_SUPPORT), true)
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/32/atbm2040_fe_32.ko:$(PRODUCT_OUT)/obj/lib_vendor/atbm2040_fe.ko
else
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/64/atbm2040_fe_64.ko:$(PRODUCT_OUT)/obj/lib_vendor/atbm2040_fe.ko
endif
endif

#sony cxd2856a tuner
ifeq ($(TUNER_MODULE), cxd2856)
ifeq ($(KERNEL_A32_SUPPORT), true)
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/32/cxd2856_fe_32.ko:$(PRODUCT_OUT)/obj/lib_vendor/cxd2856_fe.ko
else
PRODUCT_COPY_FILES += \
    device/bananapi/common/tuner/64/cxd2856_fe_64.ko:$(PRODUCT_OUT)/obj/lib_vendor/cxd2856_fe.ko
endif
endif

#PRODUCT_COPY_FILES += $(foreach tuner, $(TUNER_MODULE),\
#    $(if $(call streq, $($(KERNEL_A32_SUPPORT), true)), \
#        device/bananapi/common/tuner/32/$(tuner)_fe_32.ko:$(PRODUCT_OUT)/obj/lib_vendor/$(tuner)_fe.ko,\
#        device/bananapi/common/tuner/64/$(tuner)_fe_64.ko:$(PRODUCT_OUT)/obj/lib_vendor/$(tuner)_fe.ko))
