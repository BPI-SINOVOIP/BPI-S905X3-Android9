#
# Copyright (C) 2016 The Android Open Source Project
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

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := vtsc_test
LOCAL_MODULE_CLASS := FAKE
LOCAL_IS_HOST_MODULE := true

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): PRIVATE_PY_SCRIPT := $(LOCAL_PATH)/test_vtsc.py
$(LOCAL_BUILT_MODULE): PRIVATE_OUT_DIR := $(intermediates)/vts/test_out
$(LOCAL_BUILT_MODULE): PRIVATE_TEMP_DIR := $(intermediates)/vts/temp
$(LOCAL_BUILT_MODULE): PRIVATE_CANONICAL_DIR := $(LOCAL_PATH)/golden
$(LOCAL_BUILT_MODULE): PRIVATE_VTSC_EXEC := $(HOST_OUT_EXECUTABLES)/vtsc
$(LOCAL_BUILT_MODULE): PRIVATE_HIDL_EXEC := $(HOST_OUT_EXECUTABLES)/hidl-gen
$(LOCAL_BUILT_MODULE): $(PRIVATE_PY_SCRIPT) $(HOST_OUT_EXECUTABLES)/vtsc
$(LOCAL_BUILT_MODULE): $(PRIVATE_PY_SCRIPT) $(HOST_OUT_EXECUTABLES)/hidl-gen
	@echo "Regression test (build time): $(PRIVATE_MODULE)"
	$(hide) PYTHONPATH=$$PYTHONPATH:external/python/futures:test \
	python $(PRIVATE_PY_SCRIPT) -h $(PRIVATE_HIDL_EXEC) -p $(PRIVATE_VTSC_EXEC) \
	    -c $(PRIVATE_CANONICAL_DIR) -o $(PRIVATE_OUT_DIR) -t $(PRIVATE_TEMP_DIR)
	$(hide) touch $@

