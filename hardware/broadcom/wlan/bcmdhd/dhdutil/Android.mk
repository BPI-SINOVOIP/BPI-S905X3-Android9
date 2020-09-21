#
# Copyright (C) 2008-2011 Broadcom Corporation
#
# $Id: Android.mk,v 2.6 2009-05-07 18:25:15 $
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
# OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	dhdu.c \
	dhdu_linux.c \
	bcmutils.c \
	miniopt.c

LOCAL_MODULE := dhdutil
LOCAL_CFLAGS := -DSDTEST -DTARGETENV_android -Dlinux -DLINUX
ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -mabi=aapcs-linux
endif
LOCAL_CFLAGS += -Wall -Werror -Wno-unused-parameter
LOCAL_C_INCLUDES +=$(LOCAL_PATH)/include

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := debug

include $(BUILD_EXECUTABLE)
