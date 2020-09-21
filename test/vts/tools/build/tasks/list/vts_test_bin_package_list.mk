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

vts_test_bin_packages := \
    android.hardware.tests.msgq@1.0-service-benchmark \
    android.hardware.tests.msgq@1.0-service-test \
    fmq_test \
    hidl_test \
    hidl_test_client \
    hidl_test_helper \
    hidl_test_servers \
    mq_benchmark_client \
    mq_test_client \
    libhwbinder_benchmark \
    libhwbinder_latency \
    libbinder_benchmark \
    vts_codelab_target_binary \
    vts_selftest_flaky_test \
    vts_selftest_zero_testcase_binary_test \
    vts_test_binary_crash_app \
    vts_test_binary_seg_fault \
    vts_test_binary_syscall_exists \
    simpleperf_cpu_hotplug_test \
    binderThroughputTest \
    hwbinderThroughputTest \
    bionic-unit-tests \
    bionic-unit-tests-gcc \
    bionic-unit-tests-static \
    stressapptest \
    libcutils_test \
    vts_test_binary_qtaguid_module \
    vts_test_binary_bpf_module \

# Proto fuzzer executable
vts_test_bin_packages += \
    vts_proto_fuzzer \

# VTS Treble VINTF Test
vts_test_bin_packages += \
    vts_ibase_test \
    vts_treble_vintf_test_o_mr1 \
    vts_treble_vintf_framework_test \
    vts_treble_vintf_vendor_test  \

# Netd tests
vts_test_bin_packages += \
    netd_integration_test \

# Kernel tests.
vts_test_bin_packages += \
    dt_early_mount_test \
    kernel_net_tests \
    vts_kernel_tun_test \

# Binder tests.
vts_test_bin_packages += \
    binderDriverInterfaceTest \
    binderDriverInterfaceTest_IPC_32 \
    binderValueTypeTest \
    binderLibTest \
    binderLibTest_IPC_32 \
    binderTextOutputTest \
    binderSafeInterfaceTest \
    memunreachable_binder_test \

# VTS security PoC tests
vts_test_bin_packages += \
    30149612 \
    28838221 \
    32219453 \
    31707909 \
    32402310 \

# VTS DTBO verification tests
vts_test_bin_packages += \
    ufdt_verify_overlay
