# Mesa 3-D graphics library
#
# Copyright (C) 2015 Emil Velikov <emil.l.velikov@gmail.com>
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

# NOTE: Currently we build only a 'static' pipe-loader
LOCAL_PATH := $(call my-dir)

# get COMMON_SOURCES and DRM_SOURCES
include $(LOCAL_PATH)/Makefile.sources

include $(CLEAR_VARS)

LOCAL_CFLAGS := \
	-DHAVE_PIPE_LOADER_DRI \
	-DDROP_PIPE_LOADER_MISC \
	-DGALLIUM_STATIC_TARGETS

LOCAL_SRC_FILES := $(COMMON_SOURCES)

LOCAL_MODULE := libmesa_pipe_loader

ifneq ($(filter-out swrast,$(MESA_GPU_DRIVERS)),)
LOCAL_SRC_FILES += $(DRM_SOURCES)
LOCAL_STATIC_LIBRARIES := libmesa_loader
endif

include $(GALLIUM_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)
