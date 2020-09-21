# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
include $(ADHD_DIR)/defs/definitions.mk

CFILES	=					\
	main.c

OFILES	= $(CFILES:.c=.o)
MY_LIBS	= asound

adhdinfo:	$(OFILES) $(GAVD_ARCHIVE)
	$(CC) -o $@ $(OFILES) $(LIBS)

-include *.d
