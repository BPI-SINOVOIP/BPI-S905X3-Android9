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
from acts.controllers.relay_lib.usb_relay_board_base import UsbRelayBoardBase
from pylibftdi import BitBangDevice
""" This library is to control the sainsmart board.

Device:
    https://www.sainsmart.com/products/8-channel-12v-usb-relay-module

Additional setup steps:
Change out pip/pip3 and python2.7/3.4 based on python version
1. pip install pylibftdi
2. pip install usblib1
3. sudo apt-get install libftdi-dev
4. Make this file /etc/udev/rules.d/99-libftdi.rules with root and add the lines below:
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", GROUP="dialout", MODE="0660"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6014", GROUP="dialout", MODE="0660"
5. Connect USB relay to computer and power board with necessary connectors
6. Verify device is found by: python -m pylibftdi.examples.list_devices
6a. Example output: FTDI:FT245R USB FIFO:A9079L5D
7. The FIFO value is going to be your device name in the config
8. Your config should look something like this (note FIFO name is used here):

{
    "_description": "This is an example skeleton of a ficticious relay.",
    "testbed": [{
        "_description": "A testbed with one relay",
        "name": "relay_test",
        "RelayDevice": {
            "boards": [{
                "type": "SainSmart8ChannelUsbRelayBoard",
                "name": "ttyUSB0",
                "device": "A9079L5D"
            }],
            "devices": [{
                "type": "SingleButtonDongle",
                "name": "aukey",
                "mac_address": "e9:08:ef:2b:47:a1",
                "relays": {
                    "Action": "ttyUSB0/1"
                }

            }]
        }
    }],
    "logpath": "/tmp/logs",
    "testpaths": ["../tests"]
}
"""


class SainSmart8ChannelUsbRelayBoard(UsbRelayBoardBase):
    def set(self, relay_position, value):
        """Returns the current status of the passed in relay.

        Note that this board acts in reverse of normal relays.
        EG: NO = NC and NC = NO

        Args:
            relay_position: Relay position.
            value: Turn_on or Turn_off the relay for the given relay_position.
        """
        with BitBangDevice(self.device) as bb:
            if value == RelayState.NO:
                bb.port &= ~(self.address[relay_position])
            else:
                bb.port |= self.address[relay_position]
        self.status_dict[relay_position] = value
