# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This class defines the ADBHost Label class."""

import common

from autotest_lib.client.common_lib.brillo import hal_utils
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.hosts import base_label
from autotest_lib.server.hosts import common_label


BOARD_FILE = 'ro.product.device'


class BoardLabel(base_label.StringPrefixLabel):
    """Determine the correct board label for the device."""

    _NAME = constants.BOARD_PREFIX.rstrip(':')

    # pylint: disable=missing-docstring
    def generate_labels(self, host):
        return [host.get_board_name()]


class CameraHalLabel(base_label.BaseLabel):
    """Determine whether a host has a camera HAL in the image."""

    _NAME = 'camera-hal'

    def exists(self, host):
        return hal_utils.has_hal('camera', host=host)


class LoopbackDongleLabel(base_label.BaseLabel):
    """Determines if an audio loopback dongle is connected to the device."""

    _NAME = 'loopback-dongle'

    def exists(self, host):
        results = host.run('cat /sys/class/switch/h2w/state',
                           ignore_status=True)
        return results and '0' not in results.stdout


ADB_LABELS = [
    BoardLabel(),
    CameraHalLabel(),
    LoopbackDongleLabel(),
    common_label.OSLabel(),
]
