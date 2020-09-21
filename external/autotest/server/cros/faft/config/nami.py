# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for Nami."""

class Values(object):
    """FAFT config values for Nami."""
    chrome_ec = True
    ec_capability = ['battery', 'charging',
                     'keyboard', 'lid', 'x86' ]
    firmware_screen = 15
    wp_voltage = 'pp3300'
    spi_voltage = 'pp3300'
    servo_prog_state_delay = 10
    dark_resume_capable = True
