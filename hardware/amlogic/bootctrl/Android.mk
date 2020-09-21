#
# Copyright 2016, The Android Open Source Project
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

LOCAL_PATH := $(my-dir)

avb_common_cflags := \
    -D_FILE_OFFSET_BITS=64 \
    -D_POSIX_C_SOURCE=199309L \
    -Wa,--noexecstack \
    -Werror \
    -Wall \
    -Wextra \
    -Wformat=2 \
    -Wmissing-prototypes \
    -Wno-psabi \
    -Wno-unused-parameter \
    -Wno-format \
    -ffunction-sections \
    -fstack-protector-strong \
    -g \
    -DAVB_ENABLE_DEBUG \
    -DAVB_COMPILATION
avb_common_cppflags := \
    -Wnon-virtual-dtor \
    -fno-strict-aliasing
avb_common_ldflags := \
    -Wl,--gc-sections \
    -rdynamic

avb_common_sources := \
    libavb/avb_chain_partition_descriptor.c \
    libavb/avb_cmdline.c \
    libavb/avb_crc32.c \
    libavb/avb_crypto.c \
    libavb/avb_descriptor.c \
    libavb/avb_footer.c \
    libavb/avb_hash_descriptor.c \
    libavb/avb_hashtree_descriptor.c \
    libavb/avb_kernel_cmdline_descriptor.c \
    libavb/avb_property_descriptor.c \
    libavb/avb_rsa.c \
    libavb/avb_sha256.c \
    libavb/avb_sha512.c \
    libavb/avb_slot_verify.c \
    libavb/avb_util.c \
    libavb/avb_vbmeta_image.c \
    libavb/avb_version.c


# Build libavb for the target (for e.g. fs_mgr usage).
include $(CLEAR_VARS)
LOCAL_MODULE := libavb_amlogic
LOCAL_MODULE_HOST_OS := linux
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_CLANG := true
LOCAL_CFLAGS := $(avb_common_cflags) -DAVB_AB_I_UNDERSTAND_LIBAVB_AB_IS_DEPRECATED
LOCAL_LDFLAGS := $(avb_common_ldflags)
LOCAL_CPPFLAGS := $(avb_common_cppflags)
LOCAL_SRC_FILES := $(avb_common_sources) \
    libavb/avb_sysdeps_posix.c \
    libavb_ab/avb_ab_flow.c \
    libavb_user/avb_ops_user.cpp \
    libavb_user/avb_user_verity.c \
    libavb_user/avb_user_verification.c
LOCAL_SHARED_LIBRARIES := liblog libbase
LOCAL_STATIC_LIBRARIES := libfs_mgr libfstab


ifeq (($(shell test $(PLATFORM_SDK_VERSION) -ge 26 ) && ($(shell test $(PLATFORM_SDK_VERSION) -lt 28)  && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

ifeq ($(PLATFORM_SDK_VERSION),28)
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_PRODUCT_MODULE := true
LOCAL_JAVA_LIBRARIES += org.apache.http.legacy
else
LOCAL_SDK_VERSION := current
endif

include $(BUILD_STATIC_LIBRARY)

# Build avbctl for the target.
include $(CLEAR_VARS)
LOCAL_MODULE := avbctl_amlogic
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_CPP_EXTENSION := .cc
LOCAL_CLANG := true
LOCAL_CFLAGS := $(avb_common_cflags) -DAVB_COMPILATION -DAVB_ENABLE_DEBUG
LOCAL_CPPFLAGS := $(avb_common_cppflags)
LOCAL_LDFLAGS := $(avb_common_ldflags)
LOCAL_STATIC_LIBRARIES := \
    libavb_amlogic \
    libfs_mgr \
    libfstab
LOCAL_SHARED_LIBRARIES := \
    libbase \
    liblog
LOCAL_SRC_FILES := \
    tools/avbctl/avbctl.cc

LOCAL_MODULE_PATH := $(TARGET_OUT_PRODUCT)/bin

ifeq (($(shell test $(PLATFORM_SDK_VERSION) -ge 26 ) && ($(shell test $(PLATFORM_SDK_VERSION) -lt 28)  && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

ifeq ($(PLATFORM_SDK_VERSION),28)
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_PRODUCT_MODULE := true
LOCAL_JAVA_LIBRARIES += org.apache.http.legacy
else
LOCAL_SDK_VERSION := current
endif

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := bootctrl.amlogic
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_REQUIRED_MODULES := libavb_amlogic
LOCAL_SRC_FILES := \
    boot_control/boot_control_avb.c

LOCAL_CLANG := true
LOCAL_CFLAGS := $(avb_common_cflags) -DAVB_AB_I_UNDERSTAND_LIBAVB_AB_IS_DEPRECATED
LOCAL_LDFLAGS := $(avb_common_ldflags)
LOCAL_SHARED_LIBRARIES := libbase libcutils liblog
LOCAL_STATIC_LIBRARIES := libfs_mgr libavb_amlogic libfstab

LOCAL_POST_INSTALL_CMD := \
  $(hide) mkdir -p $(PRODUCT_OUT)/product/lib/hw && \
  mkdir -p $(PRODUCT_OUT)/vendor/lib/hw && \
  cp $(PRODUCT_OUT)/product/lib/hw/bootctrl.amlogic.so $(PRODUCT_OUT)/vendor/lib/hw/bootctrl.default.so


ifeq (($(shell test $(PLATFORM_SDK_VERSION) -ge 26 ) && ($(shell test $(PLATFORM_SDK_VERSION) -lt 28)  && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

ifeq ($(PLATFORM_SDK_VERSION),28)
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_PRODUCT_MODULE := true
LOCAL_JAVA_LIBRARIES += org.apache.http.legacy
else
LOCAL_SDK_VERSION := current
endif

include $(BUILD_SHARED_LIBRARY)