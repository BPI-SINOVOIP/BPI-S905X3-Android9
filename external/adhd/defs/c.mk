# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file contains definitions which control the C compiler.


COPTIONS =					\
	-g					\
	-O2					\
	-funit-at-a-time

# Compiler is too old to support
#
#	-Wframe-larger-than=256
#	-Wlarger-than=4096
#	-Wsync-nand

# Enable GC on unused functions and data
CGC	=					\
	-ffunction-sections			\
	-fdata-sections

PTHREAD	= 					\
	-pthread

LDGC	=					\
	-Xlinker --gc-sections

CWARN	=					\
	-Waddress				\
	-Waggregate-return			\
	-Wall					\
	-Warray-bounds				\
	-Wbad-function-cast			\
	-Wcast-align				\
	-Wchar-subscripts			\
	-Wclobbered				\
	-Wcomment				\
	-Wconversion				\
	-Wdeclaration-after-statement		\
	-Wdisabled-optimization			\
	-Wempty-body				\
	-Werror					\
	-Wextra					\
	-Wfloat-equal				\
	-Wformat				\
	-Wformat-nonliteral			\
	-Wformat-security			\
	-Wformat-y2k				\
	-Wignored-qualifiers			\
	-Wimplicit				\
	-Winit-self				\
	-Winline				\
	-Wlogical-op				\
	-Wmain					\
	-Wmissing-braces			\
	-Wmissing-declarations			\
	-Wmissing-field-initializers		\
	-Wmissing-format-attribute		\
	-Wmissing-include-dirs			\
	-Wmissing-noreturn			\
	-Wmissing-parameter-type		\
	-Wmissing-prototypes			\
	-Wnested-externs			\
	-Wold-style-declaration			\
	-Wold-style-definition			\
	-Woverlength-strings			\
	-Woverride-init				\
	-Wpacked				\
	-Wparentheses				\
	-Wpointer-arith				\
	-Wpointer-sign				\
	-Wredundant-decls			\
	-Wreturn-type				\
	-Wsequence-point			\
	-Wshadow				\
	-Wsign-compare				\
	-Wsign-conversion			\
	-Wstack-protector			\
	-Wstrict-aliasing			\
	-Wstrict-aliasing=3			\
	-Wstrict-overflow			\
	-Wstrict-overflow=5			\
	-Wstrict-prototypes			\
	-Wswitch				\
	-Wswitch-default			\
	-Wswitch-enum				\
	-Wtrigraphs				\
	-Wtype-limits				\
	-Wundef					\
	-Wuninitialized				\
	-Wunknown-pragmas			\
	-Wunsafe-loop-optimizations		\
	-Wunused-function			\
	-Wunused-label				\
	-Wunused-parameter			\
	-Wunused-value				\
	-Wunused-variable			\
	-Wvariadic-macros			\
	-Wvla					\
	-Wvolatile-register-var			\
	-Wwrite-strings				\
	-pedantic-errors

INCLUDES	=				\
		-I$(ADHD_DIR)/include		\
		-I$(ADHD_SOURCE_DIR)		\
		-I$(ADHD_DIR)/cras/src/common	\
		-I$(ADHD_DIR)/cras/src/libcras

CFLAGS		=				\
	-std=gnu99				\
	-MD 					\
	$(INCLUDES)				\
	$(PTHREADS)				\
	$(CWARN) $(COPTIONS) $(CGC) $(LDGC)
