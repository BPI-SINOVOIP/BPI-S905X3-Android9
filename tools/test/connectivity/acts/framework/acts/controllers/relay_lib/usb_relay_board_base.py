#!/usr/bin/env python
#
#   Copyright 2018 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from acts.controllers.relay_lib.relay import RelayState
from acts.controllers.relay_lib.relay_board import RelayBoard
from pylibftdi import BitBangDevice


class UsbRelayBoardBase(RelayBoard):

    VALID_RELAY_POSITIONS = [1, 2, 3, 4, 5, 6, 7, 8]
    NUM_RELAYS = 8

    def __init__(self, config):
        self.status_dict = dict()
        self.device = config["device"]
        super(UsbRelayBoardBase, self).__init__(config)
        self.address = {
            1: 0x1,
            2: 0x2,
            3: 0x4,
            4: 0x8,
            5: 0x10,
            6: 0x20,
            7: 0x40,
            8: 0x80,
            "select_all": 0xFF
        }

    def get_relay_position_list(self):
        return self.VALID_RELAY_POSITIONS

    def test_bit(self, int_type, offset):
        """Function to get status for the given relay position.

        Args:
            int_type: Port value for given relay.
            offset: offset for given Relay_position.

        Returns:
            returns current status for given relay_position.
        """
        mask = 1 << offset
        return (int_type & mask)

    def _get_relay_state(self, data, relay):
        """Function to get status for the given relay position.

        Args:
            data: Port value for given relay.
            relay: Relay_position.

        Returns:
            returns current status for given relay_position.
        """
        if relay == 1:
            return self.test_bit(data, 1)
        if relay == 2:
            return self.test_bit(data, 3)
        if relay == 3:
            return self.test_bit(data, 5)
        if relay == 4:
            return self.test_bit(data, 7)
        if relay == 5:
            return self.test_bit(data, 2)
        if relay == 6:
            return self.test_bit(data, 4)
        if relay == 7:
            return self.test_bit(data, 6)
        if relay == 8:
            return self.test_bit(data, 8)

    def get_relay_status(self, relay_position):
        """Get relay status for the given relay position.

        Args:
            relay_position: Status for given Relay position.

        Returns:
            returns current status for given relay_position.
        """
        with BitBangDevice(self.device) as bb:
            self.status_dict[relay_position] = self._get_relay_state(
                bb.port, relay_position)
        return self.status_dict[relay_position]

    def set(self, relay_position, value):
        """Returns the current status of the passed in relay.

        Args:
            relay_position: Relay position.
            value: Turn_on or Turn_off the relay for the given relay_position.
        """
        raise NotImplementedError
