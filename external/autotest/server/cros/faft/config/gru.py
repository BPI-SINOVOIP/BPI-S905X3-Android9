# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for gru."""


class Values(object):
    """FAFT config values for gru."""
    software_sync_update = 6
    chrome_ec = True
    ec_capability = ['battery', 'charging', 'keyboard', 'arm', 'lid']
    ec_boot_to_console = 0.2
    firmware_screen = 7
