# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""An interface to access the local input facade."""


import json
import logging

from autotest_lib.client.bin.input import input_event_recorder
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.graphics import graphics_utils


class InputFacadeNativeError(Exception):
    """Error in InputFacadeNative."""
    pass


class InputFacadeNative(object):
    """Facade to access the record input events."""

    def __init__(self):
        """Initializes the input facade."""
        self.recorder = None


    def initialize_input_recorder(self, device_name):
        """Initialize an input event recorder object.

        @param device_name: the name of the input device to record.

        """
        self.recorder = input_event_recorder.InputEventRecorder(device_name)
        logging.info('input event device: %s (%s)',
                     self.recorder.device_name, self.recorder.device_node)


    def clear_input_events(self):
        """Clear the event list."""
        if self.recorder is None:
            raise error.TestError('input facade: input device name not given')
        self.recorder.clear_events()


    def start_input_recorder(self):
        """Start the recording thread."""
        if self.recorder is None:
            raise error.TestError('input facade: input device name not given')
        self.recorder.start()


    def stop_input_recorder(self):
        """Stop the recording thread."""
        if self.recorder is None:
            raise error.TestError('input facade: input device name not given')
        self.recorder.stop()


    def get_input_events(self):
        """Get the bluetooth device input events.

        @returns: the recorded input events.

        """
        if self.recorder is None:
            raise error.TestError('input facade: input device name not given')
        events = self.recorder.get_events()
        return json.dumps(events)


    def press_keys(self, key_list):
        """ Simulating key press

        @param key_list: A list of key strings, e.g. ['LEFTCTRL', 'F4']
        """
        graphics_utils.press_keys(key_list)
