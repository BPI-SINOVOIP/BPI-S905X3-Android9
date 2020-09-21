# Mesa 3-D graphics library
#
# Copyright (C) 2010-2011 Chia-I Wu <olvaffe@gmail.com>
# Copyright (C) 2010-2011 LunarG Inc.
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

# included by glsl Android.mk for source generation

ifeq ($(LOCAL_MODULE_CLASS),)
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
endif

intermediates := $(call local-generated-sources-dir)
prebuilt_intermediates := $(MESA_TOP)/prebuilt-intermediates

LOCAL_SRC_FILES := $(LOCAL_SRC_FILES)

LOCAL_C_INCLUDES += \
	$(intermediates)/nir \
	$(MESA_TOP)/src/compiler/nir

LOCAL_EXPORT_C_INCLUDE_DIRS += \
	$(intermediates)/nir \
	$(MESA_TOP)/src/compiler/nir

LOCAL_GENERATED_SOURCES += $(addprefix $(intermediates)/, \
	$(NIR_GENERATED_FILES))

# Modules using libmesa_nir must set LOCAL_GENERATED_SOURCES to this
MESA_GEN_NIR_H := $(addprefix $(call local-generated-sources-dir)/, \
	nir/nir_opcodes.h \
	nir/nir_builder_opcodes.h)

nir_builder_opcodes_gen := $(LOCAL_PATH)/nir/nir_builder_opcodes_h.py
nir_builder_opcodes_deps := \
	$(LOCAL_PATH)/nir/nir_opcodes.py \
	$(LOCAL_PATH)/nir/nir_builder_opcodes_h.py

$(intermediates)/nir/nir_builder_opcodes.h: $(prebuilt_intermediates)/nir/nir_builder_opcodes.h
	cp -a $< $@

nir_constant_expressions_gen := $(LOCAL_PATH)/nir/nir_constant_expressions.py
nir_constant_expressions_deps := \
	$(LOCAL_PATH)/nir/nir_opcodes.py \
	$(LOCAL_PATH)/nir/nir_constant_expressions.py

$(intermediates)/nir/nir_constant_expressions.c: $(prebuilt_intermediates)/nir/nir_constant_expressions.c
	cp -a $< $@

nir_opcodes_h_gen := $(LOCAL_PATH)/nir/nir_opcodes_h.py
nir_opcodes_h_deps := \
	$(LOCAL_PATH)/nir/nir_opcodes.py \
	$(LOCAL_PATH)/nir/nir_opcodes_h.py

$(intermediates)/nir/nir_opcodes.h: $(prebuilt_intermediates)/nir/nir_opcodes.h
	cp -a $< $@

$(LOCAL_PATH)/nir/nir.h: $(intermediates)/nir/nir_opcodes.h

nir_opcodes_c_gen := $(LOCAL_PATH)/nir/nir_opcodes_c.py
nir_opcodes_c_deps := \
	$(LOCAL_PATH)/nir/nir_opcodes.py \
	$(LOCAL_PATH)/nir/nir_opcodes_c.py

$(intermediates)/nir/nir_opcodes.c: $(prebuilt_intermediates)/nir/nir_opcodes.c
	cp -a $< $@

nir_opt_algebraic_gen := $(LOCAL_PATH)/nir/nir_opt_algebraic.py
nir_opt_algebraic_deps := \
	$(LOCAL_PATH)/nir/nir_opt_algebraic.py \
	$(LOCAL_PATH)/nir/nir_algebraic.py

$(intermediates)/nir/nir_opt_algebraic.c: $(prebuilt_intermediates)/nir/nir_opt_algebraic.c
	cp -a $< $@
