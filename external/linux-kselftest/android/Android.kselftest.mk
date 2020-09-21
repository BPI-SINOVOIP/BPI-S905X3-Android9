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

# Cpu-hotplug test
module_prebuilt := cpu-hotplug/cpu-on-off-test
module_src_files := cpu-hotplug/cpu-on-off-test.sh
include $(build_kselftest_prebuilt)

# Efivarfs test
module_prebuilt := efivarfs/efivarfs
module_src_files := efivarfs/efivarfs.sh
include $(build_kselftest_prebuilt)

# Exec test
module_prebuilt := exec/execveat.sh
module_src_files := exec/execveat.sh
include $(build_kselftest_prebuilt)

# Firmware test

module_prebuilt := firmware/fw_fallback
module_src_files := firmware/fw_fallback.sh
include $(build_kselftest_prebuilt)

module_prebuilt := firmware/fw_filesystem
module_src_files := firmware/fw_filesystem.sh
include $(build_kselftest_prebuilt)

# Ftrace test
module_prebuilt := ftrace/ftracetest
module_src_files := ftrace/ftracetest
include $(build_kselftest_prebuilt)

module_prebuilt := ftrace/test.d/functions
module_src_files := ftrace/test.d/functions
include $(build_kselftest_prebuilt)

module_prebuilt := ftrace/test.d/00basic/basic2
module_src_files := ftrace/test.d/00basic/basic2.tc
include $(build_kselftest_prebuilt)

module_prebuilt := ftrace/test.d/00basic/basic4
module_src_files := ftrace/test.d/00basic/basic4.tc
include $(build_kselftest_prebuilt)

module_prebuilt := ftrace/test.d/00basic/basic1
module_src_files := ftrace/test.d/00basic/basic1.tc
include $(build_kselftest_prebuilt)

module_prebuilt := ftrace/test.d/00basic/basic3
module_src_files := ftrace/test.d/00basic/basic3.tc
include $(build_kselftest_prebuilt)

module_prebuilt := ftrace/test.d/template
module_src_files := ftrace/test.d/template
include $(build_kselftest_prebuilt)

module_prebuilt := ftrace/test.d/instances/instance
module_src_files := ftrace/test.d/instances/instance.tc
include $(build_kselftest_prebuilt)

module_prebuilt := ftrace/test.d/instances/instance-event
module_src_files := ftrace/test.d/instances/instance-event.tc
include $(build_kselftest_prebuilt)

# futex test
module_prebuilt := futex/functional/run.sh
module_src_files := futex/functional/run.sh
include $(build_kselftest_prebuilt)

# intel_pstate test
module_prebuilt := intel_pstate/run.sh
module_src_files := intel_pstate/run.sh
include $(build_kselftest_prebuilt)

# Lib test
module_prebuilt := lib/printf
module_src_files := lib/printf.sh
include $(build_kselftest_prebuilt)

module_prebuilt := lib/bitmap
module_src_files := lib/bitmap.sh
include $(build_kselftest_prebuilt)

# Memory-hotplug test
module_prebuilt := memory-hotplug/mem-on-off-test
module_src_files := memory-hotplug/mem-on-off-test.sh
include $(build_kselftest_prebuilt)

# Net test
module_prebuilt := net/test_bpf
module_src_files := net/test_bpf.sh
include $(build_kselftest_prebuilt)

# Pstore test
module_prebuilt := pstore/pstore_tests
module_src_files := pstore/pstore_tests
include $(build_kselftest_prebuilt)

module_prebuilt := pstore/pstore_post_reboot_tests
module_src_files := pstore/pstore_post_reboot_tests
include $(build_kselftest_prebuilt)

module_prebuilt := pstore/common_tests
module_src_files := pstore/common_tests
include $(build_kselftest_prebuilt)

module_prebuilt := pstore/pstore_crash_test
module_src_files := pstore/pstore_crash_test
include $(build_kselftest_prebuilt)


# splice test
module_prebuilt := splice/default_file_splice_read.sh
module_src_files := splice/default_file_splice_read.sh
include $(build_kselftest_prebuilt)

# Static keys test
module_prebuilt := static_keys/test_static_keys
module_src_files := static_keys/test_static_keys.sh
include $(build_kselftest_prebuilt)

# User test
module_prebuilt := user/test_user_copy
module_src_files := user/test_user_copy.sh
include $(build_kselftest_prebuilt)

# vm test
module_prebuilt := vm/run_vmtests
module_src_files := vm/run_vmtests
include $(build_kselftest_prebuilt)

# zram tests
module_prebuilt := zram/zram.sh
module_src_files := zram/zram.sh
include $(build_kselftest_prebuilt)

module_prebuilt := zram/zram01.sh
module_src_files := zram/zram01.sh
include $(build_kselftest_prebuilt)

module_prebuilt := zram/zram02.sh
module_src_files := zram/zram02.sh
include $(build_kselftest_prebuilt)

module_prebuilt := zram/zram_lib.sh
module_src_files := zram/zram_lib.sh
include $(build_kselftest_prebuilt)

