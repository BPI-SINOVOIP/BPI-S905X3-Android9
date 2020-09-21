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
LOCAL_PATH := $(call my-dir)

simpleperf_version :=  $(shell git -C $(LOCAL_PATH) rev-parse --short=12 HEAD 2>/dev/null)

simpleperf_common_cflags := -Wall -Werror -Wextra -Wunused -Wno-unknown-pragmas \
                              -DSIMPLEPERF_REVISION='"$(simpleperf_version)"'

simpleperf_cflags_target := $(simpleperf_common_cflags)

simpleperf_cflags_host := $(simpleperf_common_cflags) \
                            -DUSE_BIONIC_UAPI_HEADERS -I bionic/libc/kernel \
                            -fvisibility=hidden \

simpleperf_cflags_host_darwin := -I $(LOCAL_PATH)/nonlinux_support/include
simpleperf_cflags_host_windows := -I $(LOCAL_PATH)/nonlinux_support/include


LLVM_ROOT_PATH := external/llvm
include $(LLVM_ROOT_PATH)/llvm.mk

simpleperf_static_libraries_target := \
  libbacktrace \
  libunwindstack \
  libdexfile \
  libziparchive \
  libz \
  libbase \
  libcutils \
  liblog \
  libprocinfo \
  libutils \
  liblzma \
  libLLVMObject \
  libLLVMBitReader \
  libLLVMMC \
  libLLVMMCParser \
  libLLVMCore \
  libLLVMSupport \
  libprotobuf-cpp-lite \
  libevent \

simpleperf_static_libraries_with_libc_target := \
  $(simpleperf_static_libraries_target) \
  libc \

simpleperf_static_libraries_host := \
  libziparchive \
  libbase \
  liblog \
  liblzma \
  libz \
  libutils \
  libLLVMObject \
  libLLVMBitReader \
  libLLVMMC \
  libLLVMMCParser \
  libLLVMCore \
  libLLVMSupport \
  libprotobuf-cpp-lite \

simpleperf_static_libraries_host_linux := \
  libprocinfo \
  libbacktrace \
  libunwindstack \
  libdexfile \
  libcutils \
  libevent \

simpleperf_ldlibs_host_linux := -lrt

# libsimpleperf
# =========================================================
libsimpleperf_src_files := \
  cmd_dumprecord.cpp \
  cmd_help.cpp \
  cmd_kmem.cpp \
  cmd_report.cpp \
  cmd_report_sample.cpp \
  command.cpp \
  dso.cpp \
  event_attr.cpp \
  event_type.cpp \
  perf_regs.cpp \
  read_apk.cpp \
  read_elf.cpp \
  record.cpp \
  record_file_reader.cpp \
  report_sample.proto \
  thread_tree.cpp \
  tracing.cpp \
  utils.cpp \

libsimpleperf_src_files_linux := \
  CallChainJoiner.cpp \
  cmd_debug_unwind.cpp \
  cmd_list.cpp \
  cmd_record.cpp \
  cmd_stat.cpp \
  environment.cpp \
  event_fd.cpp \
  event_selection_set.cpp \
  InplaceSamplerClient.cpp \
  IOEventLoop.cpp \
  OfflineUnwinder.cpp \
  perf_clock.cpp \
  record_file_writer.cpp \
  UnixSocket.cpp \
  workload.cpp \

libsimpleperf_src_files_darwin := \
  nonlinux_support/nonlinux_support.cpp \

libsimpleperf_src_files_windows := \
  nonlinux_support/nonlinux_support.cpp \

# libsimpleperf target
include $(CLEAR_VARS)
LOCAL_MODULE := libsimpleperf
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_CFLAGS := $(simpleperf_cflags_target)
LOCAL_SRC_FILES := \
  $(libsimpleperf_src_files) \
  $(libsimpleperf_src_files_linux) \

LOCAL_STATIC_LIBRARIES := $(simpleperf_static_libraries_target)
LOCAL_MULTILIB := both
LOCAL_PROTOC_OPTIMIZE_TYPE := lite-static
include $(LLVM_DEVICE_BUILD_MK)
include $(BUILD_STATIC_LIBRARY)

# libsimpleperf host
include $(CLEAR_VARS)
LOCAL_MODULE := libsimpleperf
LOCAL_MODULE_HOST_OS := darwin linux windows
LOCAL_CFLAGS := $(simpleperf_cflags_host)
LOCAL_CFLAGS_darwin := $(simpleperf_cflags_host_darwin)
LOCAL_CFLAGS_linux := $(simpleperf_cflags_host_linux)
LOCAL_CFLAGS_windows := $(simpleperf_cflags_host_windows)
LOCAL_SRC_FILES := $(libsimpleperf_src_files)
LOCAL_SRC_FILES_darwin := $(libsimpleperf_src_files_darwin)
LOCAL_SRC_FILES_linux := $(libsimpleperf_src_files_linux)
LOCAL_SRC_FILES_windows := $(libsimpleperf_src_files_windows)
LOCAL_STATIC_LIBRARIES := $(simpleperf_static_libraries_host)
LOCAL_STATIC_LIBRARIES_linux := $(simpleperf_static_libraries_host_linux)
LOCAL_LDLIBS_linux := $(simpleperf_ldlibs_host_linux)
LOCAL_MULTILIB := both
LOCAL_PROTOC_OPTIMIZE_TYPE := lite-static
LOCAL_CXX_STL := libc++_static
include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_STATIC_LIBRARY)


# simpleperf
# =========================================================

# simpleperf target
include $(CLEAR_VARS)
LOCAL_MODULE := simpleperf
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_CFLAGS := $(simpleperf_cflags_target)
LOCAL_SRC_FILES := main.cpp
LOCAL_STATIC_LIBRARIES := libsimpleperf $(simpleperf_static_libraries_with_libc_target)
ifdef TARGET_2ND_ARCH
ifneq ($(TARGET_TRANSLATE_2ND_ARCH),true)
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := simpleperf32
LOCAL_MODULE_STEM_64 := simpleperf
endif
endif
LOCAL_FORCE_STATIC_EXECUTABLE := true
include $(LLVM_DEVICE_BUILD_MK)
include $(BUILD_EXECUTABLE)

$(call dist-for-goals,sdk,$(ALL_MODULES.simpleperf.BUILT))
ifdef TARGET_2ND_ARCH
$(call dist-for-goals,sdk,$(ALL_MODULES.simpleperf$(TARGET_2ND_ARCH_MODULE_SUFFIX).BUILT))
endif

# simpleperf host
include $(CLEAR_VARS)
LOCAL_MODULE := simpleperf_host
LOCAL_MODULE_HOST_OS := darwin linux windows
LOCAL_CFLAGS := $(simpleperf_cflags_host)
LOCAL_CFLAGS_darwin := $(simpleperf_cflags_host_darwin)
LOCAL_CFLAGS_linux := $(simpleperf_cflags_host_linux)
LOCAL_CFLAGS_windows := $(simpleperf_cflags_host_windows)
LOCAL_SRC_FILES := main.cpp
LOCAL_STATIC_LIBRARIES := libsimpleperf $(simpleperf_static_libraries_host)
LOCAL_STATIC_LIBRARIES_linux := $(simpleperf_static_libraries_host_linux)
LOCAL_LDLIBS_linux := $(simpleperf_ldlibs_host_linux)
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := simpleperf32
LOCAL_MODULE_STEM_64 := simpleperf
LOCAL_CXX_STL := libc++_static
include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_EXECUTABLE)

$(call dist-for-goals,sdk,$(ALL_MODULES.simpleperf_host.BUILT):simpleperf_host)
ifdef HOST_2ND_ARCH
$(call dist-for-goals,sdk,$(ALL_MODULES.simpleperf_host$(HOST_2ND_ARCH_MODULE_SUFFIX).BUILT):simpleperf_host32)
endif
$(call dist-for-goals,win_sdk,$(ALL_MODULES.host_cross_simpleperf_host.BUILT))
ifdef HOST_CROSS_2ND_ARCH
$(call dist-for-goals,win_sdk,$(ALL_MODULES.host_cross_simpleperf_host$(HOST_CROSS_2ND_ARCH_MODULE_SUFFIX).BUILT))
endif

# libsimpleperf_record.a and libsimpleperf_record.so
# They are linked to user's program, to get profile
# counters and samples for specified code ranges.
# =========================================================

# libsimpleperf_record.a on target
include $(CLEAR_VARS)
LOCAL_MODULE := libsimpleperf_record
LOCAL_CFLAGS := $(simpleperf_cflags_target)
LOCAL_SRC_FILES := record_lib_interface.cpp
LOCAL_STATIC_LIBRARIES := libsimpleperf $(simpleperf_static_libraries_target)
LOCAL_MULTILIB := both
LOCAL_CXX_STL := libc++_static
LOCAL_LDLIBS := -Wl,--exclude-libs,ALL
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
include $(LLVM_DEVICE_BUILD_MK)
include $(BUILD_STATIC_LIBRARY)

# libsimpleperf_record.so on target
include $(CLEAR_VARS)
LOCAL_MODULE := libsimpleperf_record
LOCAL_CFLAGS := $(simpleperf_cflags_target)
LOCAL_SRC_FILES := record_lib_interface.cpp
LOCAL_STATIC_LIBRARIES := libsimpleperf $(simpleperf_static_libraries_target)
LOCAL_MULTILIB := both
LOCAL_CXX_STL := libc++_static
LOCAL_LDLIBS := -Wl,--exclude-libs,ALL
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
include $(LLVM_DEVICE_BUILD_MK)
include $(BUILD_SHARED_LIBRARY)

# libsimpleperf_record.a on host
include $(CLEAR_VARS)
LOCAL_MODULE := libsimpleperf_record
LOCAL_MODULE_HOST_OS := linux
LOCAL_CFLAGS := $(simpleperf_cflags_host)
LOCAL_CFLAGS_linux := $(simpleperf_cflags_host_linux)
LOCAL_SRC_FILES := record_lib_interface.cpp
LOCAL_STATIC_LIBRARIES := libsimpleperf $(simpleperf_static_libraries_host)
LOCAL_STATIC_LIBRARIES_linux := $(simpleperf_static_libraries_host_linux)
LOCAL_LDLIBS_linux := $(simpleperf_ldlibs_host_linux) -Wl,--exclude-libs,ALL
LOCAL_MULTILIB := both
LOCAL_CXX_STL := libc++_static
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# libsimpleperf_record.so on host
include $(CLEAR_VARS)
LOCAL_MODULE := libsimpleperf_record
LOCAL_MODULE_HOST_OS := linux
LOCAL_CFLAGS := $(simpleperf_cflags_host)
LOCAL_CFLAGS_linux := $(simpleperf_cflags_host_linux)
LOCAL_SRC_FILES := record_lib_interface.cpp
LOCAL_STATIC_LIBRARIES := libsimpleperf $(simpleperf_static_libraries_host)
LOCAL_STATIC_LIBRARIES_linux := $(simpleperf_static_libraries_host_linux)
LOCAL_LDLIBS_linux := $(simpleperf_ldlibs_host_linux) -Wl,--exclude-libs,ALL
LOCAL_MULTILIB := both
LOCAL_CXX_STL := libc++_static
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_SHARED_LIBRARY)


# libsimpleperf_report.so
# It is the shared library used on host by python scripts
# to report samples in different ways.
# =========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libsimpleperf_report
LOCAL_MODULE_HOST_OS := darwin linux windows
LOCAL_CFLAGS := $(simpleperf_cflags_host)
LOCAL_CFLAGS_darwin := $(simpleperf_cflags_host_darwin)
LOCAL_CFLAGS_linux := $(simpleperf_cflags_host_linux)
LOCAL_CFLAGS_windows := $(simpleperf_cflags_host_windows)
LOCAL_SRC_FILES := report_lib_interface.cpp
LOCAL_STATIC_LIBRARIES := libsimpleperf $(simpleperf_static_libraries_host)
LOCAL_STATIC_LIBRARIES_linux := $(simpleperf_static_libraries_host_linux)
LOCAL_LDLIBS_linux := $(simpleperf_ldlibs_host_linux) -Wl,--exclude-libs,ALL
LOCAL_MULTILIB := both
LOCAL_CXX_STL := libc++_static
include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_SHARED_LIBRARY)

$(call dist-for-goals,sdk,$(ALL_MODULES.libsimpleperf_report.BUILT))
ifdef HOST_2ND_ARCH
$(call dist-for-goals,sdk,$(ALL_MODULES.libsimpleperf_report$(HOST_2ND_ARCH_MODULE_SUFFIX).BUILT):libsimpleperf_report32.so)
endif
$(call dist-for-goals,win_sdk,$(ALL_MODULES.host_cross_libsimpleperf_report.BUILT):libsimpleperf_report32.dll)
ifdef HOST_CROSS_2ND_ARCH
$(call dist-for-goals,win_sdk,$(ALL_MODULES.host_cross_libsimpleperf_report$(HOST_CROSS_2ND_ARCH_MODULE_SUFFIX).BUILT))
endif


# libsimpleperf_inplace_sampler.so
# It is the shared library linked with user's app and get samples from
# signal handlers in each thread.
# =========================================================

# libsimpleperf_inplace_sampler.so on target
include $(CLEAR_VARS)
LOCAL_MODULE := libsimpleperf_inplace_sampler
LOCAL_CFLAGS := $(simpleperf_cflags_target)
LOCAL_SRC_FILES := inplace_sampler_lib.cpp
LOCAL_STATIC_LIBRARIES := libsimpleperf $(simpleperf_static_libraries_target)
LOCAL_MULTILIB := both
LOCAL_CXX_STL := libc++_static
LOCAL_LDLIBS := -Wl,--exclude-libs,ALL
include $(LLVM_DEVICE_BUILD_MK)
include $(BUILD_SHARED_LIBRARY)

# libsimpleperf_inplace_sampler.so on host
include $(CLEAR_VARS)
LOCAL_MODULE := libsimpleperf_inplace_sampler
LOCAL_MODULE_HOST_OS := linux
LOCAL_CFLAGS := $(simpleperf_cflags_host)
LOCAL_CFLAGS_linux := $(simpleperf_cflags_host_linux)
LOCAL_SRC_FILES := inplace_sampler_lib.cpp
LOCAL_STATIC_LIBRARIES := libsimpleperf $(simpleperf_static_libraries_host)
LOCAL_STATIC_LIBRARIES_linux := $(simpleperf_static_libraries_host_linux)
LOCAL_LDLIBS_linux := $(simpleperf_ldlibs_host_linux) -Wl,--exclude-libs,ALL
LOCAL_MULTILIB := both
LOCAL_CXX_STL := libc++_static
include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_SHARED_LIBRARY)


# simpleperf_unit_test
# =========================================================
simpleperf_unit_test_src_files := \
  cmd_kmem_test.cpp \
  cmd_report_test.cpp \
  cmd_report_sample_test.cpp \
  command_test.cpp \
  gtest_main.cpp \
  read_apk_test.cpp \
  read_elf_test.cpp \
  record_test.cpp \
  sample_tree_test.cpp \
  utils_test.cpp \

simpleperf_unit_test_src_files_linux := \
  CallChainJoiner_test.cpp \
  cmd_debug_unwind_test.cpp \
  cmd_dumprecord_test.cpp \
  cmd_list_test.cpp \
  cmd_record_test.cpp \
  cmd_stat_test.cpp \
  environment_test.cpp \
  IOEventLoop_test.cpp \
  record_file_test.cpp \
  UnixSocket_test.cpp \
  workload_test.cpp \

# simpleperf_unit_test target
include $(CLEAR_VARS)
LOCAL_MODULE := simpleperf_unit_test
LOCAL_COMPATIBILITY_SUITE := device-tests
LOCAL_CFLAGS := $(simpleperf_cflags_target)
LOCAL_SRC_FILES := \
  $(simpleperf_unit_test_src_files) \
  $(simpleperf_unit_test_src_files_linux) \

LOCAL_STATIC_LIBRARIES += libsimpleperf $(simpleperf_static_libraries_with_libc_target)
LOCAL_TEST_DATA := $(call find-test-data-in-subdirs,$(LOCAL_PATH),"*",testdata)
LOCAL_MULTILIB := both
LOCAL_FORCE_STATIC_EXECUTABLE := true
include $(LLVM_DEVICE_BUILD_MK)
include $(BUILD_NATIVE_TEST)

# simpleperf_unit_test host
include $(CLEAR_VARS)
LOCAL_MODULE := simpleperf_unit_test
LOCAL_MODULE_HOST_OS := darwin linux windows
LOCAL_CFLAGS := $(simpleperf_cflags_host)
LOCAL_CFLAGS_darwin := $(simpleperf_cflags_host_darwin)
LOCAL_CFLAGS_linux := $(simpleperf_cflags_host_linux)
LOCAL_CFLAGS_windows := $(simpleperf_cflags_host_windows)
LOCAL_SRC_FILES := $(simpleperf_unit_test_src_files)
LOCAL_SRC_FILES_linux := $(simpleperf_unit_test_src_files_linux)
LOCAL_STATIC_LIBRARIES := libsimpleperf $(simpleperf_static_libraries_host)
LOCAL_STATIC_LIBRARIES_linux := $(simpleperf_static_libraries_host_linux)
LOCAL_LDLIBS_linux := $(simpleperf_ldlibs_host_linux)
LOCAL_MULTILIB := both
include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_NATIVE_TEST)


# simpleperf_cpu_hotplug_test
# =========================================================
simpleperf_cpu_hotplug_test_src_files := \
  cpu_hotplug_test.cpp \

# simpleperf_cpu_hotplug_test target
include $(CLEAR_VARS)
LOCAL_MODULE := simpleperf_cpu_hotplug_test
LOCAL_COMPATIBILITY_SUITE := device-tests
LOCAL_CFLAGS := $(simpleperf_cflags_target)
LOCAL_SRC_FILES := $(simpleperf_cpu_hotplug_test_src_files)
LOCAL_STATIC_LIBRARIES := libsimpleperf $(simpleperf_static_libraries_with_libc_target)
LOCAL_MULTILIB := both
LOCAL_FORCE_STATIC_EXECUTABLE := true
include $(LLVM_DEVICE_BUILD_MK)
include $(BUILD_NATIVE_TEST)

# simpleperf_cpu_hotplug_test linux host
include $(CLEAR_VARS)
LOCAL_MODULE := simpleperf_cpu_hotplug_test
LOCAL_MODULE_HOST_OS := linux
LOCAL_CFLAGS := $(simpleperf_cflags_host)
LOCAL_CFLAGS_linux := $(simpleperf_cflags_host_linux)
LOCAL_SRC_FILES := $(simpleperf_cpu_hotplug_test_src_files)
LOCAL_STATIC_LIBRARIES := libsimpleperf $(simpleperf_static_libraries_host)
LOCAL_STATIC_LIBRARIES_linux := $(simpleperf_static_libraries_host_linux)
LOCAL_LDLIBS_linux := $(simpleperf_ldlibs_host_linux)
LOCAL_MULTILIB := first
include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_NATIVE_TEST)


# libsimpleperf_cts_test
# =========================================================
libsimpleperf_cts_test_src_files := \
  $(libsimpleperf_src_files) \
  $(libsimpleperf_src_files_linux) \
  $(simpleperf_unit_test_src_files) \
  $(simpleperf_unit_test_src_files_linux) \

# libsimpleperf_cts_test target
include $(CLEAR_VARS)
LOCAL_MODULE := libsimpleperf_cts_test
LOCAL_CFLAGS := $(simpleperf_cflags_target) -DRUN_IN_APP_CONTEXT="\"com.android.simpleperf\""
LOCAL_SRC_FILES := $(libsimpleperf_cts_test_src_files)
LOCAL_STATIC_LIBRARIES := $(simpleperf_static_libraries_target)
LOCAL_MULTILIB := both
LOCAL_FORCE_STATIC_EXECUTABLE := true
include $(LLVM_DEVICE_BUILD_MK)
include $(BUILD_STATIC_TEST_LIBRARY)

# libsimpleperf_cts_test linux host
include $(CLEAR_VARS)
LOCAL_MODULE := libsimpleperf_cts_test
LOCAL_MODULE_HOST_OS := linux
LOCAL_CFLAGS := $(simpleperf_cflags_host)
LOCAL_CFLAGS_linux := $(simpleperf_cflags_host_linux)
LOCAL_SRC_FILES := $(libsimpleperf_cts_test_src_files)
LOCAL_STATIC_LIBRARIES := $(simpleperf_static_libraries_host)
LOCAL_STATIC_LIBRARIES_linux := $(simpleperf_static_libraries_host_linux)
LOCAL_LDLIBS_linux := $(simpleperf_ldlibs_host_linux)
LOCAL_MULTILIB := both
include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_STATIC_TEST_LIBRARY)

# simpleperf_record_test
# ============================================================

# simpleperf_record_test target
include $(CLEAR_VARS)
LOCAL_MODULE := simpleperf_record_test
LOCAL_CFLAGS := $(simpleperf_cflags_target)
LOCAL_SRC_FILES := record_lib_test.cpp
LOCAL_SHARED_LIBRARIES := libsimpleperf_record
LOCAL_MULTILIB := both
include $(BUILD_NATIVE_TEST)

# simpleperf_record_test linux host
include $(CLEAR_VARS)
LOCAL_MODULE := simpleperf_record_test
LOCAL_MODULE_HOST_OS := linux
LOCAL_CFLAGS := $(simpleperf_cflags_host)
LOCAL_CFLAGS_linux := $(simpleperf_cflags_host_linux)
LOCAL_SRC_FILES := record_lib_test.cpp
LOCAL_SHARED_LIBRARIES := libsimpleperf_record
LOCAL_LDLIBS_linux := $(simpleperf_ldlibs_host_linux)
LOCAL_MULTILIB := both
include $(BUILD_HOST_NATIVE_TEST)


# simpleperf_script.zip (for release in ndk)
# ============================================================
SIMPLEPERF_SCRIPT_LIST := \
    $(filter-out scripts/update.py,$(call all-named-files-under,*.py,scripts)) \
    scripts/inferno.sh \
    scripts/inferno.bat \
    scripts/inferno/inferno.b64 \
    $(call all-named-files-under,*,scripts/script_testdata) \
    $(call all-named-files-under,*.js,scripts) \
    $(call all-named-files-under,*.css,scripts) \
    $(call all-named-files-under,*,doc) \
    $(call all-named-files-under,app-profiling.apk,demo) \
    $(call all-named-files-under,*.so,demo) \
    $(call all-cpp-files-under,demo) \
    $(call all-java-files-under,demo) \
    $(call all-named-files-under,*.kt,demo) \
    testdata/perf_with_symbols.data \
    testdata/perf_with_trace_offcpu.data \
    testdata/perf_with_tracepoint_event.data

SIMPLEPERF_SCRIPT_LIST := $(addprefix -f $(LOCAL_PATH)/,$(SIMPLEPERF_SCRIPT_LIST))

SIMPLEPERF_SCRIPT_PATH := \
    $(call intermediates-dir-for,PACKAGING,simplerperf_script,HOST)/simpleperf_script.zip

$(SIMPLEPERF_SCRIPT_PATH) : $(SOONG_ZIP)
	$(hide) $(SOONG_ZIP) -d -o $@ -C system/extras/simpleperf $(SIMPLEPERF_SCRIPT_LIST)

sdk: $(SIMPLEPERF_SCRIPT_PATH)

$(call dist-for-goals,sdk,$(SIMPLEPERF_SCRIPT_PATH))

include $(call first-makefiles-under,$(LOCAL_PATH))
