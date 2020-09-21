# Copyright 2016 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := vts_archive_test
LOCAL_MODULE_CLASS := FAKE
LOCAL_IS_HOST_MODULE := true

include $(BUILD_SYSTEM)/base_rules.mk

py_scripts := \
  $(LOCAL_PATH)/archive_parser_test.py

$(LOCAL_BUILT_MODULE): PRIVATE_PY_SCRIPTS := $(py_scripts)
$(LOCAL_BUILT_MODULE): $(py_scripts)
	@echo "Regression test (build time): $(PRIVATE_MODULE)"
	$(foreach py, $(PRIVATE_PY_SCRIPTS), (PYTHONPATH=$$PYTHONPATH:test python $(py)) &&) true
	$(hide) touch $@
