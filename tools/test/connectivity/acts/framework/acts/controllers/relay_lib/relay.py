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

from enum import Enum
from time import sleep

from acts.controllers.relay_lib.errors import RelayConfigError


class RelayState(Enum):
    """Enum for possible Relay States."""
    # Pretend this means 'OFF'
    NO = 'NORMALLY_OPEN'
    # Pretend this means 'ON'
    NC = 'NORMALLY_CLOSED'


class SynchronizeRelays:
    """A class that allows for relays to change state nearly simultaneously.

    Can be used with the 'with' statement in Python:

    with SynchronizeRelays():
        relay1.set_no()
        relay2.set_nc()

    Note that the thread will still wait for RELAY_TRANSITION_WAIT_TIME
    after execution leaves the 'with' statement.
    """
    _sync_sleep_flag = False

    def __enter__(self):
        self.prev_toggle_time = Relay.transition_wait_time
        self.prev_sync_flag = SynchronizeRelays._sync_sleep_flag
        Relay.transition_wait_time = 0
        SynchronizeRelays._sync_sleep_flag = False

    def __exit__(self, type, value, traceback):
        if SynchronizeRelays._sync_sleep_flag:
            sleep(Relay.transition_wait_time)

        Relay.transition_wait_time = self.prev_toggle_time
        SynchronizeRelays._sync_sleep_flag = self.prev_sync_flag


class Relay(object):
    """A class representing a single relay switch on a RelayBoard.

    References to these relays are stored in both the RelayBoard and the
    RelayDevice classes under the variable "relays". GenericRelayDevice can also
    access these relays through the subscript ([]) operator.

    At the moment, relays only have a valid state of 'ON' or 'OFF'. This may be
    extended in a subclass if needed. Keep in mind that if this is done, changes
    will also need to be made in the RelayRigParser class to initialize the
    relays.

    """
    """How long to wait for relays to transition state."""
    transition_wait_time = .2
    button_press_time = .25

    def __init__(self, relay_board, position):
        self.relay_board = relay_board
        self.position = position
        self._original_state = None
        self.relay_id = "%s/%s" % (self.relay_board.name, self.position)

    def set_no(self):
        """Sets the relay to the 'NO' state. Shorthand for set(RelayState.NO).

        Blocks the thread for Relay.transition_wait_time.
        """
        self.set(RelayState.NO)

    def set_nc(self):
        """Sets the relay to the 'NC' state. Shorthand for set(RelayState.NC).

        Blocks the thread for Relay.transition_wait_time.

        """
        self.set(RelayState.NC)

    def toggle(self):
        """Swaps the state from 'NO' to 'NC' or 'NC' to 'NO'.
        Blocks the thread for Relay.transition_wait_time.
        """
        if self.get_status() == RelayState.NO:
            self.set(RelayState.NC)
        else:
            self.set(RelayState.NO)

    def set(self, state):
        """Sets the relay to the 'NO' or 'NC' state.

        Blocks the thread for Relay.transition_wait_time.

        Args:
            state: either 'NO' or 'NC'.

        Raises:
            ValueError if state is not 'NO' or 'NC'.

        """
        if self._original_state is None:
            self._original_state = self.relay_board.get_relay_status(
                self.position)

        if state is not RelayState.NO and state is not RelayState.NC:
            raise ValueError(
                'Invalid state. Received "%s". Expected any of %s.' %
                (state, [state for state in RelayState]))
        if self.get_status() != state:
            self.relay_board.set(self.position, state)
            SynchronizeRelays._sync_sleep_flag = True
            sleep(Relay.transition_wait_time)

    def set_no_for(self, seconds=button_press_time):
        """Sets the relay to 'NORMALLY_OPEN' for seconds. Blocks the thread.

        Args:
            seconds: The number of seconds to sleep for.
        """
        self.set_no()
        sleep(seconds)
        self.set_nc()

    def set_nc_for(self, seconds=button_press_time):
        """Sets the relay to 'NORMALLY_CLOSED' for seconds. Blocks the thread.

        Respects Relay.transition_wait_time for toggling state.

        Args:
            seconds: The number of seconds to sleep for.
        """
        self.set_nc()
        sleep(seconds)
        self.set_no()

    def get_status(self):
        return self.relay_board.get_relay_status(self.position)

    def clean_up(self):
        """Does any clean up needed to allow the next series of tests to run.

        For now, all this does is switches to its previous state. Inheriting
        from this class and overriding this method would be the best course of
        action to allow a more complex clean up to occur. If you do this, be
        sure to make the necessary modifications in RelayRig.initialize_relay
        and RelayRigParser.parse_json_relays.
        """
        if self._original_state is not None:
            self.set(self._original_state)

    def is_dirty(self):
        return self._original_state is not None


class RelayDict(object):
    """A wrapped dictionary that gives config errors upon failure.

    Has the same interface as a dictionary, but when getting the key fails, the
    dictionary returns a RelayConfigError, letting the user know that the reason
    the dict failed to return a relay is because the relay was not found in the
    config.

    Also prevents modification of elements, because changing the relays here
    does not change what they are in hardware.
    """
    ERROR_MESSAGE = ('Error: Attempted to get relay "%s" in %s "%s" but the '
                     'relay does not exist.\nExisting relays are: %s.\nMake '
                     'sure the missing relay is added to the config file, and '
                     'is properly setup.')

    def __init__(self, relay_device, input_dict):
        self.relay_device = relay_device
        self._store = input_dict

    def __getitem__(self, key):
        try:
            return self._store[key]
        except KeyError:
            raise RelayConfigError(self.ERROR_MESSAGE %
                                   (key, type(self.relay_device),
                                    self.relay_device.name, self._store))

    def __iter__(self):
        return iter(self._store)

    def __len__(self):
        return len(self._store)

    def __repr__(self):
        return repr(self._store)
