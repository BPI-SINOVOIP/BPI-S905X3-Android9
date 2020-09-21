# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for Veyron."""


class Values(object):
    """FAFT config values for Veyron."""
    spi_voltage = 'pp3300'
    wp_voltage = 'pp3300'
    confirm_screen = 6
    has_eventlog = False        # for chrome-os-partner:61078
