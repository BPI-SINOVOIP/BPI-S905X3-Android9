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

# for platform/bionic/tests/libs
vts_test_lib_packages += \
    cfi_test_helper \
    cfi_test_helper2 \
    ld_config_test_helper \
    ld_config_test_helper_lib1 \
    ld_config_test_helper_lib2 \
    ld_config_test_helper_lib3 \
    ld_preload_test_helper \
    ld_preload_test_helper_lib1 \
    ld_preload_test_helper_lib2 \
    libatest_simple_zip \
    libcfi-test \
    libcfi-test-bad \
    libdl_preempt_test_1 \
    libdl_preempt_test_2 \
    libdl_test_df_1_global \
    libdlext_test \
    libdlext_test_different_soname \
    libdlext_test_fd \
    libdlext_test_norelro \
    libdlext_test_runpath_zip_zipaligned \
    libdlext_test_zip \
    libdlext_test_zip_zipaligned \
    libfortify1-tests-clang \
    libfortify2-tests-clang \
    libgnu-hash-table-library \
    libnstest_dlopened \
    libnstest_ns_a_public1 \
    libnstest_ns_a_public1_internal \
    libnstest_ns_b_public2 \
    libnstest_ns_b_public3 \
    libnstest_private \
    libnstest_private_external \
    libnstest_public \
    libnstest_public_internal \
    libnstest_root \
    libnstest_root_not_isolated \
    libsysv-hash-table-library \
    libtest_atexit \
    libtest_check_order_dlsym \
    libtest_check_order_dlsym_1_left \
    libtest_check_order_dlsym_2_right \
    libtest_check_order_dlsym_3_c \
    libtest_check_order_dlsym_a \
    libtest_check_order_dlsym_b \
    libtest_check_order_dlsym_d \
    libtest_check_order_reloc_root \
    libtest_check_order_reloc_root_1 \
    libtest_check_order_reloc_root_2 \
    libtest_check_order_reloc_siblings \
    libtest_check_order_reloc_siblings_1 \
    libtest_check_order_reloc_siblings_2 \
    libtest_check_order_reloc_siblings_3 \
    libtest_check_order_reloc_siblings_a \
    libtest_check_order_reloc_siblings_b \
    libtest_check_order_reloc_siblings_c \
    libtest_check_order_reloc_siblings_c_1 \
    libtest_check_order_reloc_siblings_c_2 \
    libtest_check_order_reloc_siblings_d \
    libtest_check_order_reloc_siblings_e \
    libtest_check_order_reloc_siblings_f \
    libtest_check_rtld_next_from_library \
    libtest_dlopen_df_1_global \
    libtest_dlopen_from_ctor \
    libtest_dlopen_from_ctor_main \
    libtest_dlopen_weak_undefined_func \
    libtest_dlsym_df_1_global \
    libtest_dlsym_from_this \
    libtest_dlsym_from_this_child \
    libtest_dlsym_from_this_grandchild \
    libtest_dlsym_weak_func \
    libtest_dt_runpath_a \
    libtest_dt_runpath_b \
    libtest_dt_runpath_c \
    libtest_dt_runpath_d \
    libtest_dt_runpath_x \
    libtest_empty \
    libtest_ifunc \
    libtest_ifunc_variable \
    libtest_ifunc_variable_impl \
    libtest_init_fini_order_child \
    libtest_init_fini_order_grand_child \
    libtest_init_fini_order_root \
    libtest_init_fini_order_root2 \
    libtest_invalid-empty_shdr_table.so \
    libtest_invalid-rw_load_segment.so \
    libtest_invalid-textrels.so \
    libtest_invalid-textrels2.so \
    libtest_invalid-unaligned_shdr_offset.so \
    libtest_invalid-zero_shdr_table_content.so \
    libtest_invalid-zero_shdr_table_offset.so \
    libtest_invalid-zero_shentsize.so \
    libtest_invalid-zero_shstrndx.so \
    libtest_nodelete_1 \
    libtest_nodelete_2 \
    libtest_nodelete_dt_flags_1 \
    libtest_pthread_atfork \
    libtest_relo_check_dt_needed_order \
    libtest_relo_check_dt_needed_order_1 \
    libtest_relo_check_dt_needed_order_2 \
    libtest_simple \
    libtest_two_parents_child \
    libtest_two_parents_parent1 \
    libtest_two_parents_parent2 \
    libtest_versioned_lib \
    libtest_versioned_libv1 \
    libtest_versioned_libv2 \
    libtest_versioned_otherlib \
    libtest_versioned_otherlib_empty \
    libtest_versioned_uselibv1 \
    libtest_versioned_uselibv2 \
    libtest_versioned_uselibv2_other \
    libtest_versioned_uselibv3_other \
    libtest_with_dependency \
    libtest_with_dependency_loop \
    libtest_with_dependency_loop_a \
    libtest_with_dependency_loop_b \
    libtest_with_dependency_loop_b_tmp \
    libtest_with_dependency_loop_c \
    libtestshared \
    preinit_getauxval_test_helper \
    preinit_syscall_test_helper \

# for drm tests
vts_test_lib_packages += \
    libvtswidevine \

# for fuzz tests
vts_test_lib_packages += \
    libclang_rt.asan-arm-android \
    libclang_rt.asan-aarch64-android \
    libclang_rt.asan-i686-android \
    libvts_func_fuzzer_utils \
    libvts_proto_fuzzer \
    libvts_proto_fuzzer_proto \

# for HAL interface hash test
vts_test_lib_packages += \
    libhidl-gen-hash \
