# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""An adapter to remotely access the input facade on DUT."""

import json


class InputFacadeRemoteAdapter(object):
    """This is an adapter to remotely capture the DUT's input events.

    The Autotest host object representing the remote DUT, passed to this
    class on initialization, can be accessed from its _client property.

    """
    def __init__(self, remote_facade_proxy):
        """Constructs an InputFacadeRemoteAdapter.

        @param remote_facade_proxy: RemoteFacadeProxy object.

        """
        self._proxy = remote_facade_proxy


    @property
    def _input_proxy(self):
        """Gets the proxy to DUT input facade.

        @return XML RPC proxy to DUT input facade.

        """
        return self._proxy.input


    def initialize_input_recorder(self, device_name):
        """Initialize an input event recorder object.

        @param device_name: the name of the input device to record.

        """
        self._input_proxy.initialize_input_recorder(device_name)


    def clear_input_events(self):
        """Clear the event list."""
        self._input_proxy.clear_input_events()


    def start_input_recorder(self):
        """Start the recording thread."""
        self._input_proxy.start_input_recorder()


    def stop_input_recorder(self):
        """Stop the recording thread."""
        self._input_proxy.stop_input_recorder()


    def get_input_events(self):
        """Get the bluetooth device events.

        @returns: the recorded input events.

        """
        return json.loads(self._input_proxy.get_input_events())


    def press_keys(self, key_list):
        """ Simulating key press

        @param key_list: A list of key strings, e.g. ['LEFTCTRL', 'F4']
        """
        self._input_proxy.press_keys(key_list)
