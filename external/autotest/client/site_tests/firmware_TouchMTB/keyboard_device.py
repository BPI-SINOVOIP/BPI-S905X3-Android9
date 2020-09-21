# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Keyboard device module to capture keyboard events."""

import fcntl
import os
import re
import sys

sys.path.append('../../bin/input')
import input_device

import mtb

from linux_input import EV_KEY


class KeyboardDevice:
    """A class about keyboard device properties."""

    def __init__(self, device_node=None):
        if device_node:
            self.device_node = device_node
        else:
            self.device_node = input_device.get_device_node(
                    input_device.KEYBOARD_TYPES))
        self.system_device = self._non_blocking_open(self.device_node)
        self._input_event = input_device.InputEvent()

    def __del__(self):
        self.system_device.close()

    def exists(self):
        """Indicate whether this device exists or not."""
        return bool(self.device_node)

    def _non_blocking_open(self, filename):
        """Open the system file in the non-blocking mode."""
        fd = open(filename)
        fcntl.fcntl(fd, fcntl.F_SETFL, os.O_NONBLOCK)
        return fd

    def _non_blocking_read(self, fd):
        """Non-blocking read on fd."""
        try:
            self._input_event.read(fd)
            return self._input_event
        except Exception:
            return None

    def get_key_press_event(self, fd):
        """Read the keyboard device node to get the key press events."""
        event = True
        # Read the device node continuously until either a key press event
        # is got or there is no more events to read.
        while event:
            event = self._non_blocking_read(fd)
            if event and event.type == EV_KEY and event.value == 1:
                return event.code
        return None
