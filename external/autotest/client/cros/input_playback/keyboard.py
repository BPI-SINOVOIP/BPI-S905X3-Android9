# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.input_playback import input_playback

_KEYBOARD = 'keyboard'
_KEY_FILENAME = 'keyboard_%s'

class Keyboard(object):
    """An emulated keyboard device used for UI automation."""

    def __init__(self):
        """Prepare an emulated keyboard device."""
        self.dirname = os.path.dirname(__file__)
        # Create an emulated keyboard device.
        self.keyboard = input_playback.InputPlayback()
        self.keyboard.emulate(input_type=_KEYBOARD)
        self.keyboard.find_connected_inputs()

    def press_key(self, key):
        """Replay the recorded key events if exists.

        @param key: the target key name, e.g. f1, f2, tab or combination of keys
                    'alt+shift+i', 'ctrl+f5', etc.

        """
        event_file = os.path.join(self.dirname, _KEY_FILENAME % key)
        if not os.path.exists(event_file):
            raise error.TestError('No such key file keyboard_%s in %s'
                                  % (key, self.dirname))
        self.keyboard.blocking_playback(filepath=event_file,
                                        input_type=_KEYBOARD)

    def playback(self, event_file):
        """Replay the specified key events file.

        @param event_file: the filename of the key events

        """
        self.keyboard.blocking_playback(filepath=event_file,
                                        input_type=_KEYBOARD)

    def close(self):
        """Clean up the files/handles created in the class."""
        if self.keyboard:
            self.keyboard.close()
