# Copyright 2005 The Android Open Source Project
#
# Android.mk for TinyXml.
#
# Add -DTIXML_USE_STL to CFLAGS to use STL.
#

LOCAL_PATH := $(call my-dir)

commonSources:= \
	tinyxml.cpp \
	tinyxmlparser.cpp \
	tinyxmlerror.cpp \
	tinystr.cpp

commonFlags := \
	-Wno-undefined-bool-conversion \
	-Wno-missing-braces \
        -Wno-logical-op-parentheses \
        -Werror


# For the device
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	$(commonSources)

LOCAL_MODULE:= libtinyxml

LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS+= $(commonFlags)

include $(BUILD_SHARED_LIBRARY)


