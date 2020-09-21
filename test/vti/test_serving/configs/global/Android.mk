# Copyright (C) 2017 The Android Open Source Project
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

.PHONY: vti-global-config

vti-global-config-prod-zip := $(HOST_OUT)/vti-global-config/vti-global-config-prod.zip
vti-global-config-test-zip := $(HOST_OUT)/vti-global-config/vti-global-config-test.zip

.PHONY: $(vti-global-config-prod-zip)
$(vti-global-config-prod-zip): $(SOONG_ZIP)
	@echo "build vti config package: $@"
	$(hide) mkdir -p $(dir $@)
	$(hide) rm -f $@
	$(hide) find test/vti/test_serving/configs/global/prod/ -name '*.*_config' | sort > $@.list
	$(hide) find vendor/google_vts/configs/global/prod/ -name '*.*_config' | sort > $@.list.vendor
	$(hide) $(SOONG_ZIP) -d -o $@ -C test/vti/test_serving/configs/global -l $@.list \
	    -C vendor/google_vts/configs/global -l $@.list.vendor
	$(hide) rm -f $@.list $@.list.vendor

.PHONY: $(vti-global-config-test-zip)
$(vti-global-config-test-zip): $(SOONG_ZIP)
	@echo "build vti config package: $@"
	$(hide) mkdir -p $(dir $@)
	$(hide) rm -f $@
	$(hide) find test/vti/test_serving/configs/global/test/ -name '*.*_config' | sort > $@.list
	$(hide) find vendor/google_vts/configs/global/test/ -name '*.*_config' | sort > $@.list.vendor
	$(hide) $(SOONG_ZIP) -d -o $@ -C test/vti/test_serving/configs/global -l $@.list \
	    -C vendor/google_vts/configs/global -l $@.list.vendor
	$(hide) rm -f $@.list $@.list.vendor

vti-global-config: $(vti-global-config-prod-zip) $(vti-global-config-test-zip)
$(call dist-for-goals, vti-global-config, $(vti-global-config-prod-zip) $(vti-global-config-test-zip))

vti-config: vti-global-config

vti: vti-config

vts: vti-config
