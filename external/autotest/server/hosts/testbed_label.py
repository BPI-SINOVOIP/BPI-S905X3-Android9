# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This class defines the TestBed Label detection classes."""

import common

from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.hosts import adb_label
from autotest_lib.server.hosts import base_label


class ADBDeviceLabels(base_label.StringLabel):
    """Return a list of the labels gathered from the devices connected."""

    # _NAME is omitted because the labels generated are full labels.
    # The generated labels are from an adb device, so we just want to grab the
    # possible labels from ADBLabels.
    _LABEL_LIST = adb_label.ADB_LABELS

    # pylint: disable=missing-docstring
    def generate_labels(self, testbed):
        labels = []
        for adb_device in testbed.get_adb_devices().values():
            labels.extend(adb_device.get_labels())
        # Currently the board label will need to be modified for each adb
        # device.  We'll get something like 'board:android-shamu' and
        # we'll need to update it to 'board:android-shamu-1'.  Let's store all
        # the labels in a dict and keep track of how many times we encounter
        # it, that way we know what number to append.
        board_label_dict = {}
        updated_labels = set()
        for label in labels:
            # Update the board labels
            if label.startswith(constants.BOARD_PREFIX):
                # Now let's grab the board num and append it to the board_label.
                board_num = board_label_dict.setdefault(label, 0) + 1
                board_label_dict[label] = board_num
                updated_labels.add('%s-%d' % (label, board_num))
            else:
                # We don't need to mess with this.
                updated_labels.add(label)
        return list(updated_labels)


TESTBED_LABELS = [
        ADBDeviceLabels(),
]
