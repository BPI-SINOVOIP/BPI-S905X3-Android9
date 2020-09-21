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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from acts.controllers.relay_lib.errors import RelayConfigError
from acts.controllers.relay_lib.helpers import validate_key
from acts.controllers.relay_lib.relay import Relay


class RelayBoard(object):
    """Handles interfacing with the Relays and RelayDevices.

    This is the base class for all RelayBoards.
    """

    def __init__(self, config):
        """Creates a RelayBoard instance. Handles naming and relay creation.

        Args:
            config: A configuration dictionary, usually pulled from an element
            under in "boards" list in the relay rig config file.
        """
        self.name = validate_key('name', config, str, 'config')
        if '/' in self.name:
            raise RelayConfigError('RelayBoard name cannot contain a "/".')
        self.relays = dict()
        for pos in self.get_relay_position_list():
            self.relays[pos] = Relay(self, pos)

    def set(self, relay_position, state):
        """Sets the relay to the given state.

        Args:
            relay_position: the relay having its state modified.
            state: the state to set the relay to. Currently only states NO and
                   NC are supported.
        """
        raise NotImplementedError()

    def get_relay_position_list(self):
        """Returns a list of all possible relay positions."""
        raise NotImplementedError()

    def get_relay_status(self, relay):
        """Returns the state of the given relay."""
        raise NotImplementedError()
