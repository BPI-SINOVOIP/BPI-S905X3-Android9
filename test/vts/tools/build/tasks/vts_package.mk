#
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
#

LOCAL_PATH := $(call my-dir)

build_utils_dir := $(LOCAL_PATH)/../utils

include $(LOCAL_PATH)/list/vts_adapter_package_list.mk
include $(LOCAL_PATH)/list/vts_apk_package_list.mk
include $(LOCAL_PATH)/list/vts_bin_package_list.mk
include $(LOCAL_PATH)/list/vts_lib_package_list.mk
include $(LOCAL_PATH)/list/vts_spec_file_list.mk
include $(LOCAL_PATH)/list/vts_test_bin_package_list.mk
include $(LOCAL_PATH)/list/vts_test_lib_package_list.mk
include $(LOCAL_PATH)/list/vts_test_lib_hal_package_list.mk
include $(LOCAL_PATH)/list/vts_test_lib_hidl_package_list.mk
include $(LOCAL_PATH)/list/vts_func_fuzzer_package_list.mk
include $(LOCAL_PATH)/list/vts_test_host_lib_package_list.mk
include $(LOCAL_PATH)/list/vts_test_host_bin_package_list.mk
include $(LOCAL_PATH)/list/vts_test_hidl_hal_hash_list.mk
include $(build_utils_dir)/vts_package_utils.mk
-include external/linux-kselftest/android/kselftest_test_list.mk
-include external/ltp/android/ltp_package_list.mk

VTS_OUT_ROOT := $(HOST_OUT)/vts
VTS_TESTCASES_OUT := $(VTS_OUT_ROOT)/android-vts/testcases
VTS_TOOLS_OUT := $(VTS_OUT_ROOT)/android-vts/tools

# Packaging rule for android-vts.zip
test_suite_name := vts
test_suite_tradefed := vts-tradefed
test_suite_readme := test/vts/README.md

include $(BUILD_SYSTEM)/tasks/tools/compatibility.mk

.PHONY: vts
vts: $(compatibility_zip) vtslab adb
$(call dist-for-goals, vts, $(compatibility_zip))

# Packaging rule for android-vts.zip's testcases dir (DATA subdir).
target_native_modules := \
    $(kselftest_modules) \
    ltp \
    $(ltp_packages) \
    $(vts_adapter_package_list) \
    $(vts_apk_packages) \
    $(vts_bin_packages) \
    $(vts_lib_packages) \
    $(vts_test_bin_packages) \
    $(vts_test_lib_packages) \
    $(vts_test_lib_hal_packages) \
    $(vts_test_lib_hidl_packages) \
    $(vts_func_fuzzer_packages) \

target_native_copy_pairs := \
  $(call target-native-copy-pairs,$(target_native_modules),$(VTS_TESTCASES_OUT))

# Packaging rule for android-vts.zip's testcases dir (spec subdir).

target_spec_modules := \
  $(VTS_SPEC_FILE_LIST)

target_spec_copy_pairs :=
$(foreach m,$(target_spec_modules),\
  $(eval my_spec_copy_dir :=\
    spec/$(word 2,$(subst android/frameworks/, frameworks/hardware/interfaces/,\
                    $(subst android/hardware/, hardware/interfaces/,\
                      $(subst android/hidl/, system/libhidl/transport/,\
                        $(subst android/system/, system/hardware/interfaces/,$(dir $(m)))))))/vts)\
  $(eval my_spec_copy_file := $(notdir $(m)))\
  $(eval my_spec_copy_dest := $(my_spec_copy_dir)/$(my_spec_copy_file))\
  $(eval target_spec_copy_pairs += $(m):$(VTS_TESTCASES_OUT)/$(my_spec_copy_dest)))

$(foreach m,$(vts_spec_file_list),\
  $(if $(wildcard $(m)),\
    $(eval target_spec_copy_pairs += $(m):$(VTS_TESTCASES_OUT)/spec/$(m))))

target_trace_files := \
  $(call find-files-in-subdirs,test/vts-testcase/hal-trace,"*.vts.trace" -and -type f,.) \

target_trace_copy_pairs := \
$(foreach f,$(target_trace_files),\
    test/vts-testcase/hal-trace/$(f):$(VTS_TESTCASES_OUT)/hal-hidl-trace/test/vts-testcase/hal-trace/$(f))

target_hal_hash_modules := \
    $(vts_test_hidl_hal_hash_list) \

target_hal_hash_copy_pairs :=
$(foreach m,$(target_hal_hash_modules),\
  $(if $(wildcard $(m)),\
    $(eval target_hal_hash_copy_pairs += $(m):$(VTS_TESTCASES_OUT)/hal-hidl-hash/$(m))))


# Packaging rule for host-side test native packages

target_hostdriven_modules := \
  $(vts_test_host_lib_packages) \
  $(vts_test_host_bin_packages) \

target_hostdriven_copy_pairs := \
  $(call host-native-copy-pairs,$(target_hostdriven_modules),$(VTS_TESTCASES_OUT))

host_additional_deps_copy_pairs := \
  test/vts/tools/vts-tradefed/etc/vts-tradefed_win.bat:$(VTS_TOOLS_OUT)/vts-tradefed_win.bat \
  test/vts/tools/vts-tradefed/DynamicConfig.xml:$(VTS_TESTCASES_OUT)/cts.dynamic

# Packaging rule for host-side Python logic, configs, and data files

host_framework_files := \
  $(call find-files-in-subdirs,test/vts,"*.py" -and -type f,.) \
  $(call find-files-in-subdirs,test/vts,"*.runner_conf" -and -type f,.) \
  $(call find-files-in-subdirs,test/vts,"*.push" -and -type f,.)

host_framework_copy_pairs := \
  $(foreach f,$(host_framework_files),\
    test/vts/$(f):$(VTS_TESTCASES_OUT)/vts/$(f))

host_testcase_files := \
  $(call find-files-in-subdirs,test/vts-testcase,"*.py" -and -type f,.) \
  $(call find-files-in-subdirs,test/vts-testcase,"*.runner_conf" -and -type f,.) \
  $(call find-files-in-subdirs,test/vts-testcase,"*.push" -and -type f,.) \
  $(call find-files-in-subdirs,test/vts-testcase,"*.dump" -and -type f,.)

host_testcase_copy_pairs := \
  $(foreach f,$(host_testcase_files),\
    test/vts-testcase/$(f):$(VTS_TESTCASES_OUT)/vts/testcases/$(f))

host_kernel_config_files :=\
  $(call find-files-in-subdirs,kernel/configs,"android-base*.cfg" -and -type f,.)

host_kernel_config_copy_pairs := \
  $(foreach f,$(host_kernel_config_files),\
    kernel/configs/$(f):$(VTS_TESTCASES_OUT)/vts/testcases/kernel/config/data/$(f))

ifneq ($(TARGET_BUILD_PDK),true)

host_camera_its_files := \
  $(call find-files-in-subdirs,cts/apps/CameraITS,"*.py" -and -type f,.) \
  $(call find-files-in-subdirs,cts/apps/CameraITS,"*.pdf" -and -type f,.) \
  $(call find-files-in-subdirs,cts/apps/CameraITS,"*.png" -and -type f,.)

host_camera_its_copy_pairs := \
  $(foreach f,$(host_camera_its_files),\
    cts/apps/CameraITS/$(f):$(VTS_TESTCASES_OUT)/CameraITS/$(f))

else

host_camera_its_copy_pairs :=

endif  # ifneq ($(TARGET_BUILD_PDK),true)

host_systrace_files := \
  $(filter-out .git/%, \
    $(call find-files-in-subdirs,external/chromium-trace,"*" -and -type f,.))

host_systrace_copy_pairs := \
  $(foreach f,$(host_systrace_files),\
    external/chromium-trace/$(f):$(VTS_OUT_ROOT)/android-vts/tools/external/chromium-trace/$(f))

media_test_res_files := \
  $(call find-files-in-subdirs,hardware/interfaces/media/res,"*.*" -and -type f,.) \

media_test_res_copy_pairs := \
  $(foreach f,$(media_test_res_files),\
    hardware/interfaces/media/res/$(f):$(VTS_TESTCASES_OUT)/DATA/media/res/$(f))

performance_test_res_files := \
  $(call find-files-in-subdirs,test/vts-testcase/performance/res/,"*.*" -and -type f,.) \

performance_test_res_copy_pairs := \
  $(foreach f,$(performance_test_res_files),\
    test/vts-testcase/performance/res/$(f):$(VTS_TESTCASES_OUT)/DATA/performance/res/$(f))

audio_test_res_files := \
  $(call find-files-in-subdirs,hardware/interfaces/audio,"*.xsd" -and -type f,.) \

audio_test_res_copy_pairs := \
  $(foreach f,$(audio_test_res_files),\
    hardware/interfaces/audio/$(f):$(VTS_TESTCASES_OUT)/DATA/hardware/interfaces/audio/$(f))

vndk_test_res_copy_pairs := \
  development/vndk/tools/definition-tool/datasets/eligible-list-28.csv:$(VTS_TESTCASES_OUT)/vts/testcases/vndk/golden/$(PLATFORM_VNDK_VERSION)/eligible-list.csv \

kernel_rootdir_test_rc_files := \
  $(call find-files-in-subdirs,system/core/rootdir,"*.rc" -and -type f,.) \

kernel_rootdir_test_rc_copy_pairs := \
  $(foreach f,$(kernel_rootdir_test_rc_files),\
    system/core/rootdir/$(f):$(VTS_TESTCASES_OUT)/vts/testcases/kernel/api/rootdir/init_rc_files/$(f)) \

acts_framework_files := \
  $(call find-files-in-subdirs,tools/test/connectivity/acts/framework/acts,"*.py" -and -type f,.)

acts_framework_copy_pairs := \
  $(foreach f,$(acts_framework_files),\
    tools/test/connectivity/acts/framework/acts/$(f):$(VTS_TESTCASES_OUT)/acts/$(f))

acts_testcases_files := \
  $(call find-files-in-subdirs,tools/test/connectivity/acts/tests/google,"*.py" -and -type f,.)

acts_testcases_copy_pairs := \
  $(foreach f,$(acts_testcases_files),\
    tools/test/connectivity/acts/tests/google/$(f):$(VTS_TESTCASES_OUT)/vts/testcases/acts/$(f))

target_script_files := \
  $(call find-files-in-subdirs,test/vts/script/target,"*.sh" -and -type f,.)

target_script_copy_pairs := \
  $(foreach f,$(target_script_files),\
    test/vts/script/target/$(f):$(VTS_TESTCASES_OUT)/script/target/$(f))

system_property_compatibility_test_res_copy_pairs := \
  system/sepolicy/public/property_contexts:$(VTS_TESTCASES_OUT)/vts/testcases/security/system_property/data/property_contexts

$(VTS_TESTCASES_OUT)/vts/testcases/vndk/golden/platform_vndk_version.txt:
	@echo -n $(PLATFORM_VNDK_VERSION) > $@

vts_test_core_copy_pairs := \
  $(call copy-many-files,$(host_framework_copy_pairs)) \
  $(call copy-many-files,$(host_testcase_copy_pairs)) \
  $(call copy-many-files,$(host_additional_deps_copy_pairs)) \
  $(call copy-many-files,$(target_spec_copy_pairs)) \
  $(call copy-many-files,$(target_hal_hash_copy_pairs)) \
  $(call copy-many-files,$(acts_framework_copy_pairs)) \

vts_copy_pairs := \
  $(vts_test_core_copy_pairs) \
  $(call copy-many-files,$(target_native_copy_pairs)) \
  $(call copy-many-files,$(target_trace_copy_pairs)) \
  $(call copy-many-files,$(target_hostdriven_copy_pairs)) \
  $(call copy-many-files,$(host_kernel_config_copy_pairs)) \
  $(call copy-many-files,$(host_camera_its_copy_pairs)) \
  $(call copy-many-files,$(host_systrace_copy_pairs)) \
  $(call copy-many-files,$(media_test_res_copy_pairs)) \
  $(call copy-many-files,$(performance_test_res_copy_pairs)) \
  $(call copy-many-files,$(audio_test_res_copy_pairs)) \
  $(call copy-many-files,$(vndk_test_res_copy_pairs)) \
  $(call copy-many-files,$(kernel_rootdir_test_rc_copy_pairs)) \
  $(call copy-many-files,$(acts_testcases_copy_pairs)) \
  $(call copy-many-files,$(target_script_copy_pairs)) \
  $(call copy-many-files,$(system_property_compatibility_test_res_copy_pairs)) \
  $(VTS_TESTCASES_OUT)/vts/testcases/vndk/golden/platform_vndk_version.txt \

.PHONY: vts-test-core
vts-test-core: $(vts_test_core_copy_pairs)

$(compatibility_zip): $(vts_copy_pairs)

-include vendor/google_vts/tools/build/vts_package_vendor.mk
