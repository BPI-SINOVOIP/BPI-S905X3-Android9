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
"""
Device Details:
https://www.logitech.com/en-in/product/bluetooth-audio-adapter#specification-tabular
"""
import enum
from acts.controllers.relay_lib.errors import RelayConfigError
from acts.controllers.relay_lib.generic_relay_device import GenericRelayDevice
from acts.controllers.relay_lib.helpers import validate_key

PAIRING_MODE_WAIT_TIME = 5
WAIT_TIME = 0.1
MISSING_RELAY_MSG = 'Relay config for logitech Headset "%s" missing relay "%s".'


class Buttons(enum.Enum):
    Power = "Power"
    Pair = "Pair"


class LogitechAudioReceiver(GenericRelayDevice):
    def __init__(self, config, relay_rig):
        GenericRelayDevice.__init__(self, config, relay_rig)
        self.mac_address = validate_key('mac_address', config, str,
                                        'LogitechAudioReceiver')
        for button in Buttons:
            self.ensure_config_contains_relay(button.value)

    def setup(self):
        GenericRelayDevice.setup(self)

    def clean_up(self):
        """Sets all relays to their default state (off)."""
        GenericRelayDevice.clean_up(self)

    def ensure_config_contains_relay(self, relay_name):
        """
        Throws an error if the relay does not exist.

        Args:
            relay_name:relay_name to be checked.
        """
        if relay_name not in self.relays:
            raise RelayConfigError(MISSING_RELAY_MSG % (self.name, relay_name))

    def power_on(self):
        """Power on relay."""
        self.relays[Buttons.Power.value].set_nc()

    def pairing_mode(self):
        """Sets relay in paring mode."""
        self.relays[Buttons.Pair.value].set_nc()
