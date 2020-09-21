# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT configuration overrides for Poppy."""


class Values(object):
    """FAFT config values for Poppy."""

    mode_switcher_type = 'tablet_detachable_switcher'
    fw_bypasser_type = 'tablet_detachable_bypasser'

    confirm_screen = 1

    has_lid = True

    usb_image_boot_timeout = 60

    has_keyboard = False

    chrome_ec = True
    ec_capability = ['x86', 'battery', 'charging', 'lid']
