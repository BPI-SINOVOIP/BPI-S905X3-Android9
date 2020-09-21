#!/usr/bin/env python
#
#   Copyright 2016 - The Android Open Source Project
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
from acts.controllers.relay_lib.usb_relay_board_base import UsbRelayBoardBase
from pylibftdi import BitBangDevice


class RdlRelayBoard(UsbRelayBoardBase):
    def set(self, relay_position, value):
        """Returns the current status of the passed in relay.

        Args:
            relay_position: Relay position.
            value: Turn_on or Turn_off the relay for the given relay_position.
        """
        with BitBangDevice(self.device) as bb:
            if value == RelayState.NO:
                bb.port |= self.address[relay_position]
            else:
                bb.port &= ~(self.address[relay_position])
        self.status_dict[relay_position] = value
