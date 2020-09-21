#!/usr/bin/env python
#
#   Copyright 2017 - The Android Open Source Project
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
import time
import enum
import logging
from acts.controllers.relay_lib.generic_relay_device import GenericRelayDevice
from acts.controllers.relay_lib.errors import RelayConfigError
from acts.controllers.relay_lib.helpers import validate_key

PAIRING_MODE_WAIT_TIME = 6
POWER_TOGGLE_WAIT_TIME = 1
MISSING_RELAY_MSG = 'Relay config for Sony XB20 "%s" missing relay "%s".'

log = logging

class Buttons(enum.Enum):
    POWER = 'Power'

class SonyXB20Speaker(GenericRelayDevice):
    """Sony XB20 Bluetooth Speaker model

    Wraps the button presses, as well as the special features like pairing.
    """

    def __init__(self, config, relay_rig):
        GenericRelayDevice.__init__(self, config, relay_rig)

        self.mac_address = validate_key('mac_address', config, str, 'sony_xb20')

        for button in Buttons:
            self.ensure_config_contains_relay(button.value)

    def ensure_config_contains_relay(self, relay_name):
        """Throws an error if the relay does not exist."""
        if relay_name not in self.relays:
            raise RelayConfigError(MISSING_RELAY_MSG % (self.name, relay_name))

    def _hold_button(self, button, seconds):
        self.hold_down(button.value)
        time.sleep(seconds)
        self.release(button.value)

    def power_on(self):
        self._hold_button(Buttons.POWER, POWER_TOGGLE_WAIT_TIME)

    def power_off(self):
        self._hold_button(Buttons.POWER, POWER_TOGGLE_WAIT_TIME)

    def enter_pairing_mode(self):
        self._hold_button(Buttons.POWER, PAIRING_MODE_WAIT_TIME)

    def setup(self):
        """Sets all relays to their default state (off)."""
        GenericRelayDevice.setup(self)

    def clean_up(self):
        """Sets all relays to their default state (off)."""
        GenericRelayDevice.clean_up(self)
