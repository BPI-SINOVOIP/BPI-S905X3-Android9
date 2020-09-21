# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT configuration overrides for Ninja."""

from autotest_lib.server.cros.faft.config import rambi


class Values(rambi.Values):
    """Inherit overrides from rambi."""
    ec_capability = ['x86', 'usb', 'smart_usb_charge']
    has_lid = False
    has_keyboard = False
    rec_button_dev_switch = True
    wp_voltage = 'pp3300'
    spi_voltage = 'pp3300'
