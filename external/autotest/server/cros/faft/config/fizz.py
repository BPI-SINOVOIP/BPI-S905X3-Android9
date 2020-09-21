# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT configuration overrides for Fizz."""


class Values(object):
    """FAFT config values for Fizz."""
    firmware_screen = 15

    has_lid = False
    has_keyboard = False
    rec_button_dev_switch = True
    spi_voltage = 'pp3300'
    wp_voltage = 'pp3300'
    chrome_ec = True
    ec_capability = ['x86', 'no_reset_in_off']
