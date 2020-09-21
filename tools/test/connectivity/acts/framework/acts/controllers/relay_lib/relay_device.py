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

from acts.controllers.relay_lib.errors import RelayConfigError
from acts.controllers.relay_lib.helpers import validate_key


class RelayDevice(object):
    """The base class for all relay devices.

    RelayDevice has access to both its relays as well as the relay rig it is
    a part of. Note that you can receive references to the relay_boards
    through relays[0...n].board. The relays are not guaranteed to be on
    the same relay board.
    """

    def __init__(self, config, relay_rig):
        """Creates a RelayDevice.

        Args:
            config: The dictionary found in the config file for this device.
            You can add your own params to the config file if needed, and they
            will be found in this dictionary.
            relay_rig: The RelayRig the device is attached to. This won't be
            useful for classes that inherit from RelayDevice, so just pass it
            down to this __init__.
        """
        self.rig = relay_rig
        self.relays = dict()

        validate_key('name', config, str, '"devices" element')
        self.name = config['name']

        relays = validate_key('relays', config, dict, '"devices" list element')
        if len(relays) < 1:
            raise RelayConfigError(
                'Key "relays" must have at least 1 element.')

        for name, relay_id in relays.items():
            self.relays[name] = relay_rig.relays[relay_id]

    def setup(self):
        """Sets up the relay device to be ready for commands."""
        pass

    def clean_up(self):
        """Sets the relay device back to its inert state."""
        pass
