# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export ADHD_DIR = $(shell pwd)
include $(ADHD_DIR)/defs/definitions.mk

all:	cras

cras:
	@$(call remake,Building,$@,cras.mk,$@)

cras_install:
	@$(call remake,Building,cras,cras.mk,$@)

cras-scripts:
	$(ECHO) "Installing cras scripts"
	$(INSTALL) --mode 755 -d $(DESTDIR)usr/bin/
	$(INSTALL) --mode 755 -D $(ADHD_DIR)/scripts/audio_diagnostics \
		$(DESTDIR)usr/bin/

cras_init_upstart:	$(ADHD_DIR)/init/cras.conf
	$(ECHO) "Installing upstart file"
	$(INSTALL) --mode 644 -D $< $(DESTDIR)/etc/init/cras.conf

cras_init_scripts:	$(ADHD_DIR)/init/cras.sh
	$(INSTALL) --mode 644 -D $< $(DESTDIR)/usr/share/cros/init/cras.sh

SYSTEMD_UNIT_DIR := /usr/lib/systemd/system/
SYSTEMD_TMPFILESD_DIR := /usr/lib/tmpfiles.d/

cras_init_systemd:	$(ADHD_DIR)/init/cras.service \
	$(ADHD_DIR)/init/cras-directories.conf
	$(ECHO) "Installing systemd files"
	$(INSTALL) --mode 644 -D $(ADHD_DIR)/init/cras.service \
		$(DESTDIR)/$(SYSTEMD_UNIT_DIR)/cras.service
	$(INSTALL) --mode 755 -d $(DESTDIR)/$(SYSTEMD_UNIT_DIR)/system-services.target.wants
	$(LINK) -s ../cras.service \
		$(DESTDIR)/$(SYSTEMD_UNIT_DIR)/system-services.target.wants/cras.service
	$(INSTALL) --mode 644 -D $(ADHD_DIR)/init/cras-directories.conf \
		$(DESTDIR)/$(SYSTEMD_TMPFILESD_DIR)/cras-directories.conf

ifeq ($(strip $(SYSTEMD)), yes)

cras_init: cras_init_systemd cras_init_scripts

else

cras_init: cras_init_upstart cras_init_scripts

endif

$(DESTDIR)/etc/cras/device_blacklist:	$(ADHD_DIR)/cras-config/device_blacklist
	$(ECHO) "Installing '$<' to '$@'"
	$(INSTALL) --mode 644 -D $< $@

optional_alsa_conf := $(wildcard $(ADHD_DIR)/alsa-module-config/alsa-$(BOARD).conf)

ifneq ($(strip $(optional_alsa_conf)),)

$(DESTDIR)/etc/modprobe.d/alsa-$(BOARD).conf:	$(optional_alsa_conf)
	$(ECHO) "Installing '$<' to '$@'"
	$(INSTALL) --mode 644 -D $< $@

install:	$(DESTDIR)/etc/modprobe.d/alsa-$(BOARD).conf

endif

optional_alsa_patch := $(wildcard $(ADHD_DIR)/alsa-module-config/$(BOARD)_alsa.fw)

ifneq ($(strip $(optional_alsa_patch)),)

$(DESTDIR)/lib/firmware/$(BOARD)_alsa.fw:	$(optional_alsa_patch)
	$(ECHO) "Installing '$<' to '$@'"
	$(INSTALL) --mode 644 -D $< $@

install:	$(DESTDIR)/lib/firmware/$(BOARD)_alsa.fw

endif

install:	$(DESTDIR)/etc/cras/device_blacklist \
		cras-scripts \
		cras_install \
		cras_init

clean:
	@rm -rf $(ADHD_BUILD_DIR)

.PHONY:	clean cras cras_install cras-script
