# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.bluetooth import bluetooth_semiauto_helper


class bluetooth_IDCheck(bluetooth_semiauto_helper.BluetoothSemiAutoHelper):
    """Checks whether the Bluetooth ID and Alias are in the correct format."""
    version = 1

    # Boards which only support bluetooth version 3 and below
    _BLUETOOTH_3_BOARDS = ['x86-mario', 'x86-zgb']

    # Boards which are not shipped to customers and don't need an id.
    _REFERENCE_ONLY = ['rambi', 'nyan', 'oak', 'reef']

    def warmup(self):
        """Overwrite parent warmup; no need to log in."""
        pass

    def _check_id(self, adapter_info):
        """Fail if the Bluetooth ID is not in the correct format.

        @param adapter_info: a dict of information about this device's adapter
        @raises: error.TestFail if incorrect format is found.

        """
        modalias = adapter_info['Modalias']
        logging.info('Saw Bluetooth ID of: %s', modalias)

        if self._device in self._BLUETOOTH_3_BOARDS:
            bt_format = 'bluetooth:v00E0p24..d0300'
        else:
            bt_format = 'bluetooth:v00E0p24..d0400'

        if not re.match(bt_format, modalias):
            raise error.TestFail('%s does not match expected format: %s'
                                 % (modalias, bt_format))

    def _check_name(self, adapter_info):
        """Fail if the Bluetooth name is not in the correct format.

        @param adapter_info: a dict of information about this device's adapter
        @raises: error.TestFail if incorrect format is found.

        """
        alias = adapter_info['Alias']
        logging.info('Saw Bluetooth Alias of: %s', alias)

        device_type = utils.get_device_type().lower()
        alias_format = '%s_[a-z0-9]{4}' % device_type
        if not re.match(alias_format, alias.lower()):
            raise error.TestFail('%s does not match expected format: %s'
                                 % (alias, alias_format))

    def run_once(self):
        """Entry point of this test."""
        if not self.supports_bluetooth():
            return

        self._device = utils.get_board()
        if self._device in self._REFERENCE_ONLY:
            return

        self.poll_adapter_presence()
        adapter_info = self._get_adapter_info()
        self._check_id(adapter_info)
        self._check_name(adapter_info)
