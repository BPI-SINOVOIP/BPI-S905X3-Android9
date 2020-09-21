# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for Pit."""


class Values(object):
    """FAFT config values for Pit."""
    chrome_ec = True
    ec_capability = (['battery', 'keyboard', 'arm', 'lid'])
    ec_has_powerbtn_cmd = False
    has_eventlog = False        # No RTC support in firmware
