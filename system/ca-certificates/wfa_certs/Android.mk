# -*- mode: makefile -*-
# Copyright (C) 2018 The Android Open Source Project
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

cacerts_wfa := $(call all-files-under,files)

#This target is reference by frameworks/opt/net/wifi/service/Android.mk and used by WfaKeyStore
cacerts_wfa_target_directory := $(TARGET_OUT)/etc/security/cacerts_wfa
$(foreach cacert, $(cacerts_wfa), $(eval $(call include-prebuilt-with-destination-directory,target-cacert-wifi-$(notdir $(cacert)),$(cacert),$(cacerts_wfa_target_directory))))
cacerts_wfa_target := $(addprefix $(cacerts_wfa_target_directory)/,$(foreach cacert,$(cacerts_wfa),$(notdir $(cacert))))
.PHONY: cacerts_wfa_target
cacerts_wfa: $(cacerts_wfa_target)

# This is so that build/target/product/core.mk can use cacerts_wfa in PRODUCT_PACKAGES
ALL_MODULES.cacerts_wfa.INSTALLED := $(cacerts_wfa_target)

cacerts_wfa_host_directory := $(HOST_OUT)/etc/security/cacerts_wfa
$(foreach cacert, $(cacerts_wfa), $(eval $(call include-prebuilt-with-destination-directory,host-cacert-wifi-$(notdir $(cacert)),$(cacert),$(cacerts_wfa_host_directory))))

cacerts_wfa_host := $(addprefix $(cacerts_wfa_host_directory)/,$(foreach cacert,$(cacerts_wfa),$(notdir $(cacert))))
.PHONY: cacerts_wfa-host
cacerts_wfa-host: $(cacerts_wfa_host)
