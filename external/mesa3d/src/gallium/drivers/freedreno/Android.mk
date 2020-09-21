# Copyright (C) 2014 Emil Velikov <emil.l.velikov@gmail.com>
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

LOCAL_PATH := $(call my-dir)

# get C_SOURCES
include $(LOCAL_PATH)/Makefile.sources

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(C_SOURCES) \
	$(a2xx_SOURCES) \
	$(a3xx_SOURCES)	\
	$(a4xx_SOURCES) \
	$(a5xx_SOURCES) \
	$(ir3_SOURCES)

#LOCAL_CFLAGS := \
#	-Wno-packed-bitfield-compat

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/ir3

LOCAL_GENERATED_SOURCES := $(MESA_GEN_NIR_H)

LOCAL_SHARED_LIBRARIES := libdrm_freedreno
LOCAL_STATIC_LIBRARIES := libmesa_glsl libmesa_nir
LOCAL_MODULE := libmesa_pipe_freedreno

include $(LOCAL_PATH)/Android.gen.mk
include $(GALLIUM_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)
