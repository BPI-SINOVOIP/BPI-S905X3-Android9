# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file contains definitions that are specific to the invocation
# and usage of Gnu Make.

ifndef VERBOSE
# Be silent unless 'VERBOSE' is set on the make command line.
SILENT	= --silent
endif

ifndef ADHD_BUILD_DIR
export ADHD_BUILD_DIR	= $(ADHD_DIR)/build/$(BOARD)
endif

GAVD_ARCHIVE	= $(ADHD_BUILD_DIR)/lib/gavd.a

LIBS		=							\
		-L$(ADHD_DIR)/cras/src/.libs				\
		$(GAVD_ARCHIVE)						\
		$(foreach lib,$(MY_LIBS),-l$(lib))

# mkdir: Creates a directory, and all its parents, if it does not exist.
#
mkdir	= [ ! -d $(1) ] &&			\
	    $(MKDIR) --parents $(1) || true

# remake: Gnu Make function which will create the build directory,
#         then build the first argument by recursively invoking make.
#         The recursive make is performed in the build directory.
#
#         $(call remake,<label>,<subdirectory>,<makefile>,<target>)
#
#         ex: @$(call remake,Building,gavd,Makefile,gavd)
#                             $(1)    $(2) $(3)     $(4)
#
#  REL_DIR:
#
#    Directory relative from the root of the source tree.  REL_DIR is
#    built up using the previous value plus the current target
#    directory.
#
#  ADHD_SOURCE_DIR:
#
#    The directory containing the sources for the target directory
#    being built.  This is used by Makefiles to access files in the
#    source directory.  It has the same value as VPATH.
#
#  THIS_BUILD_DIR:
#
#    The build directory which is currently being built.  This is the
#    same 'pwd', and the directory in which Make is building.
#
#  The build is performed in the build directory and VPATH is used to
#  allow Make to find the source files in the source directory.
#
remake	=							\
	+($(if $(REL_DIR),					\
		export REL_DIR=$${REL_DIR}/$(2),		\
		export REL_DIR=$(2)) &&				\
	$(call mkdir,$(ADHD_BUILD_DIR)/$${REL_DIR}) &&		\
	    $(MESSAGE) "$(1) $${REL_DIR}";			\
	    $(MAKE) $(SILENT)					\
		-f $(ADHD_DIR)/$${REL_DIR}/$(3)			\
		-C $(ADHD_BUILD_DIR)/$${REL_DIR}		\
		VPATH=$(ADHD_DIR)/$${REL_DIR}			\
		ADHD_SOURCE_DIR=$(ADHD_DIR)/$${REL_DIR}		\
		THIS_BUILD_DIR=$(ADHD_BUILD_DIR)/$${REL_DIR}	\
		$(4))
