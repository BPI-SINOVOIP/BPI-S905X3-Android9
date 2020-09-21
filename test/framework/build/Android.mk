#
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
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

VTSLAB_OUT_ROOT := $(HOST_OUT)/vtslab
VTSLAB_TESTCASES_OUT := $(VTSLAB_OUT_ROOT)/android-vtslab/testcases
VTSLAB_TOOLS_OUT := $(VTSLAB_OUT_ROOT)/android-vtslab/tools

# Packaging rule for android-vtslab.zip
test_suite_name := vtslab
test_suite_readme := test/framework/README.md

include $(LOCAL_PATH)/package.mk
include test/vts/tools/build/utils/vts_package_utils.mk

vts_lab_required_packages: run adb fastboot
	echo "radio $(ALL_MODULES.WifiUtil.BUILT_INSTALLED)"

.PHONY: vtslab
vtslab: $(compatibility_zip) $(vts_lab_required_packages)
$(call dist-for-goals, vtslab, $(compatibility_zip))


host_hc_files := \
  $(call find-files-in-subdirs,test/framework/harnesses/host_controller,"*.py" -and -type f,.)

host_hc_copy_pairs := \
  $(foreach f,$(host_hc_files),\
      test/framework/harnesses/host_controller/$(f):$(VTSLAB_TESTCASES_OUT)/host_controller/$(f))

host_hc_extra_copy_pairs := \
  test/framework/tools/host_controller/run:$(VTSLAB_TOOLS_OUT)/run \

host_acloud_files := \
  $(call find-files-in-subdirs,tools/acloud,"*.py" -and -type f,.) \
  $(call find-files-in-subdirs,tools/acloud,"*.config" -and -type f,.)

host_acloud_copy_pairs := \
  $(foreach f,$(host_acloud_files),\
    tools/acloud/$(f):$(VTSLAB_TESTCASES_OUT)/acloud/$(f))

host_vti_proto_files := \
  $(call find-files-in-subdirs,test/vti/test_serving/proto,"*.py" -and -type f,.)

host_vti_proto_copy_pairs := \
  $(foreach f,$(host_vti_proto_files),\
    test/vti/test_serving/proto/$(f):$(VTSLAB_TESTCASES_OUT)/vti/test_serving/proto/$(f))

$(VTSLAB_TESTCASES_OUT)/vti/test_serving/__init__.py:
	@mkdir -p $(VTSLAB_TESTCASES_OUT)/vti/test_serving
	@touch $(VTSLAB_TESTCASES_OUT)/vti/test_serving/__init__.py

$(VTSLAB_TESTCASES_OUT)/vti/__init__.py:
	@mkdir -p $(VTSLAB_TESTCASES_OUT)/vti
	@touch $(VTSLAB_TESTCASES_OUT)/vti/__init__.py

host_vti_extra_copy_pairs := \
  $(VTSLAB_TESTCASES_OUT)/vti/__init__.py \
  $(VTSLAB_TESTCASES_OUT)/vti/test_serving/__init__.py \

vts_host_python_files := \
  $(call find-files-in-subdirs,test/vts,"*.py" -and -type f,.)

vts_host_python_copy_pairs := \
  $(foreach f,$(vts_host_python_files),\
    test/vts/$(f):$(VTSLAB_TESTCASES_OUT)/vts/$(f))

vtslab_apk_modules := \
  WifiUtil \
  CtsVerifier \

vtslab_copy_pairs := \
  $(call copy-many-files,$(vtslab_apk_modules)) \
  $(call copy-many-files,$(host_hc_copy_pairs)) \
  $(call copy-many-files,$(host_hc_extra_copy_pairs)) \
  $(call copy-many-files,$(host_acloud_copy_pairs)) \
  $(call copy-many-files,$(host_vti_proto_copy_pairs)) \
  $(call copy-many-files,$(vts_host_python_copy_pairs)) \
  $(host_vti_extra_copy_pairs) \

$(compatibility_zip): $(vtslab_copy_pairs)

