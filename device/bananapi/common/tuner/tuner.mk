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

#======================================================================================
# 1.for tuner ko file copy and insmod
# 2.define and config $(TUNER_MODULE) in $(PRODUCT_DIR).mk
# 3.support multiple tuner config, e.g. TUNER_MODULE := mxl661 si2151
# 4.need to add insmod for each tuner to the file $(PRODUCT_DIR)/init.amlogic.board.rc
#======================================================================================

ifneq ($(strip $(TUNER_MODULE)),)
$(warning TUNER_MODULE is $(TUNER_MODULE))
PRODUCT_COPY_FILES += $(foreach tuner, $(TUNER_MODULE),\
    $(if $(findstring true, $(KERNEL_A32_SUPPORT)),\
        device/bananapi/common/tuner/32/$(tuner)_fe_32.ko:$(PRODUCT_OUT)/obj/lib_vendor/$(tuner)_fe.ko,\
        device/bananapi/common/tuner/64/$(tuner)_fe_64.ko:$(PRODUCT_OUT)/obj/lib_vendor/$(tuner)_fe.ko))
endif
