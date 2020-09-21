#
# Copyright (C) 2011 Intel Corporation
# Copyright (C) 2010-2011 Chia-I Wu <olvaffe@gmail.com>
# Copyright (C) 2010-2011 LunarG
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#

LOCAL_PATH := $(call my-dir)

# Import variables i965_FILES.
include $(LOCAL_PATH)/Makefile.sources

I965_PERGEN_COMMON_INCLUDES := \
	$(MESA_DRI_C_INCLUDES) \
	$(MESA_TOP)/src/intel

I965_PERGEN_SHARED_LIBRARIES := \
	$(MESA_DRI_SHARED_LIBRARIES) \
	libdrm_intel

I965_PERGEN_STATIC_LIBRARIES := \
	libmesa_genxml \
	libmesa_nir

I965_PERGEN_LIBS := \
	libmesa_i965_gen6 \
	libmesa_i965_gen7 \
	libmesa_i965_gen75 \
	libmesa_i965_gen8 \
	libmesa_i965_gen9

# ---------------------------------------
# Build libmesa_i965_gen6
# ---------------------------------------

include $(CLEAR_VARS)

LOCAL_MODULE := libmesa_i965_gen6

LOCAL_C_INCLUDES := $(I965_PERGEN_COMMON_INCLUDES)

LOCAL_SRC_FILES := $(i965_gen6_FILES)

LOCAL_SHARED_LIBRARIES := $(I965_PERGEN_SHARED_LIBRARIES)

LOCAL_STATIC_LIBRARIES := $(I965_PERGEN_STATIC_LIBRARIES)

LOCAL_CFLAGS := -DGEN_VERSIONx10=60

include $(MESA_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)

# ---------------------------------------
# Build libmesa_i965_gen7
# ---------------------------------------

include $(CLEAR_VARS)

LOCAL_MODULE := libmesa_i965_gen7

LOCAL_C_INCLUDES := $(I965_PERGEN_COMMON_INCLUDES)

LOCAL_SRC_FILES := $(i965_gen7_FILES)

LOCAL_SHARED_LIBRARIES := $(I965_PERGEN_SHARED_LIBRARIES)

LOCAL_STATIC_LIBRARIES := $(I965_PERGEN_STATIC_LIBRARIES)

LOCAL_CFLAGS := -DGEN_VERSIONx10=70

include $(MESA_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)

# ---------------------------------------
# Build libmesa_i965_gen75
# ---------------------------------------

include $(CLEAR_VARS)

LOCAL_MODULE := libmesa_i965_gen75

LOCAL_C_INCLUDES := $(I965_PERGEN_COMMON_INCLUDES)

LOCAL_SRC_FILES := $(i965_gen75_FILES)

LOCAL_SHARED_LIBRARIES := $(I965_PERGEN_SHARED_LIBRARIES)

LOCAL_STATIC_LIBRARIES := $(I965_PERGEN_STATIC_LIBRARIES)

LOCAL_CFLAGS := -DGEN_VERSIONx10=75

include $(MESA_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)

# ---------------------------------------
# Build libmesa_i965_gen8
# ---------------------------------------

include $(CLEAR_VARS)

LOCAL_MODULE := libmesa_i965_gen8

LOCAL_C_INCLUDES := $(I965_PERGEN_COMMON_INCLUDES)

LOCAL_SRC_FILES := $(i965_gen8_FILES)

LOCAL_SHARED_LIBRARIES := $(I965_PERGEN_SHARED_LIBRARIES)

LOCAL_STATIC_LIBRARIES := $(I965_PERGEN_STATIC_LIBRARIES)

LOCAL_CFLAGS := -DGEN_VERSIONx10=80

include $(MESA_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)

# ---------------------------------------
# Build libmesa_i965_gen9
# ---------------------------------------

include $(CLEAR_VARS)

LOCAL_MODULE := libmesa_i965_gen9

LOCAL_C_INCLUDES := $(I965_PERGEN_COMMON_INCLUDES)

LOCAL_SRC_FILES := $(i965_gen9_FILES)

LOCAL_SHARED_LIBRARIES := $(I965_PERGEN_SHARED_LIBRARIES)

LOCAL_STATIC_LIBRARIES := $(I965_PERGEN_STATIC_LIBRARIES)

LOCAL_CFLAGS := -DGEN_VERSIONx10=90

include $(MESA_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)

# ---------------------------------------
# Build libmesa_i965_compiler
# ---------------------------------------

include $(CLEAR_VARS)

LOCAL_MODULE := libmesa_i965_compiler
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_SRC_FILES := \
	$(i965_compiler_FILES)

LOCAL_C_INCLUDES := \
	$(MESA_DRI_C_INCLUDES) \
	$(MESA_TOP)/src/intel \
	$(MESA_TOP)/src/compiler/nir \
	$(call generated-sources-dir-for,STATIC_LIBRARIES,libmesa_nir,,)/nir \
	$(call generated-sources-dir-for,STATIC_LIBRARIES,libmesa_glsl,,)/glsl

LOCAL_SHARED_LIBRARIES := \
	libdrm_intel

include $(LOCAL_PATH)/Android.gen.mk
include $(MESA_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)

# ---------------------------------------
# Build i965_dri
# ---------------------------------------

include $(CLEAR_VARS)

LOCAL_MODULE := i965_dri
LOCAL_MODULE_RELATIVE_PATH := $(MESA_DRI_MODULE_REL_PATH)

LOCAL_CFLAGS := \
	$(MESA_DRI_CFLAGS)

ifeq ($(ARCH_X86_HAVE_SSE4_1),true)
LOCAL_CFLAGS += \
	-DUSE_SSE41
endif

LOCAL_C_INCLUDES := \
	$(MESA_DRI_C_INCLUDES)

LOCAL_SRC_FILES := \
	$(i965_FILES)

LOCAL_WHOLE_STATIC_LIBRARIES := \
	$(MESA_DRI_WHOLE_STATIC_LIBRARIES) \
	$(I965_PERGEN_LIBS) \
	libmesa_intel_common \
	libmesa_blorp \
	libmesa_isl \
	libmesa_i965_compiler

LOCAL_SHARED_LIBRARIES := \
	$(MESA_DRI_SHARED_LIBRARIES) \
	libdrm_intel

LOCAL_GENERATED_SOURCES := \
	$(MESA_DRI_OPTIONS_H) \
	$(MESA_GEN_NIR_H)

include $(MESA_COMMON_MK)
include $(BUILD_SHARED_LIBRARY)
