# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.input_playback import input_playback


class a11y_test_base(test.test):
    """Base class for a11y tests."""
    version = 1

    # ChromeVox extension id
    _CHROMEVOX_ID = 'mndnfokpggljbaajbnioimlmbfngpief'
    _CVOX_STATE_TIMEOUT = 40
    _CVOX_INDICATOR_TIMEOUT = 40


    def warmup(self):
        """Test setup."""
        # Emulate a keyboard for later ChromeVox toggle (if needed).
        # See input_playback. The keyboard is used to play back shortcuts.
        self._player = input_playback.InputPlayback()
        self._player.emulate(input_type='keyboard')
        self._player.find_connected_inputs()


    def _child_test_cleanup(self):
        """Can be overwritten by child classes and run duing parent cleanup."""
        return


    def cleanup(self):
        self._player.close()
        self._child_test_cleanup()


    def _toggle_chromevox(self):
        """Use keyboard shortcut and emulated keyboard to toggle ChromeVox."""
        self._player.blocking_playback_of_default_file(
                input_type='keyboard', filename='keyboard_ctrl+alt+z')

    def _chromevox_move(self, direction):
        """Use ChromeVox move commands (search + arrow key).

        @param direction:  The direction in which to move, e.g. 'down'.

        """
        self._player.blocking_playback_of_default_file(
                input_type='keyboard',
                filename='keyboard_search+%s' % direction)

    def _set_feature(self, feature, value):
        """Set given feature to given value using a11y API call.

        Presupposes self._extension (with accessibilityFeatures enabled).

        @param feature: string of accessibility feature to change.
        @param value: boolean of expected value.

        @raises: error.TestError if feature cannot be set.

        """
        value_str = 'true' if value else 'false'
        cmd = ('window.__result = null;\n'
               'chrome.accessibilityFeatures.%s.set({value: %s});\n'
               'chrome.accessibilityFeatures.%s.get({}, function(d) {'
               'window.__result = d[\'value\']; });' % (
                       feature, value_str, feature))
        self._extension.ExecuteJavaScript(cmd)

        poll_cmd = 'window.__result == %s;' % value_str
        utils.poll_for_condition(
                lambda: self._extension.EvaluateJavaScript(poll_cmd),
                exception = error.TestError(
                        'Timeout while trying to set %s to %s' %
                        (feature, value_str)))


    def _get_chromevox_state(self):
        """Return whether ChromeVox is enabled or not.

        Presupposes self._extension (with management enabled).

        @returns: value of management.get.enabled.

        @raises: error.TestError if state cannot be determined.

        """
        cmd = ('window.__enabled = null;\n'
               'chrome.management.get(\'%s\', function(r) {'
               'if (r) {window.__enabled = r[\'enabled\'];} '
               'else {window.__enabled = false;}});' % self._CHROMEVOX_ID)
        self._extension.ExecuteJavaScript(cmd)

        poll_cmd = 'window.__enabled != null;'
        utils.poll_for_condition(
                lambda: self._extension.EvaluateJavaScript(poll_cmd),
                exception=error.TestError(
                        'ChromeVox: management.get.enabled was not set!'))
        return self._extension.EvaluateJavaScript('window.__enabled')


    def _confirm_chromevox_state(self, value):
        """Fail test unless ChromeVox state is given value.

        Presupposes self._extension (with management enabled).

        @param value: True or False, whether ChromeVox should be enabled.

        @raises: error.TestFail if actual state doesn't match expected.

        """
        utils.poll_for_condition(
                lambda: self._get_chromevox_state() == value,
                exception=error.TestFail('ChromeVox: enabled state '
                                         'was not %s.' % value),
                timeout=self._CVOX_STATE_TIMEOUT)


    def _get_extension_path(self):
        """Return the path to the default accessibility extension.

        @returns: string of path to default extension.

        """
        this_dir = os.path.dirname(__file__)
        return os.path.join(this_dir, 'a11y_ext')
