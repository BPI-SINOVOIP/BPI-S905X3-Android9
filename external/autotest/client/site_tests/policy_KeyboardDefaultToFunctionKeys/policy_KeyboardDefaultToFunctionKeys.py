# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.enterprise import enterprise_policy_base
from autotest_lib.client.cros.input_playback import input_playback
from autotest_lib.client.cros.touch_playback_test_base import EventsPage


class policy_KeyboardDefaultToFunctionKeys(
        enterprise_policy_base.EnterprisePolicyTest):
    version = 1

    POLICY_NAME = 'KeyboardDefaultToFunctionKeys'
    TEST_CASES = {
        'True': True,
        'False': False,
        'NotSet': None
    }


    def initialize(self, **kwargs):
        """
        Emulate a keyboard and initialize enterprise policy base.

        """
        super(policy_KeyboardDefaultToFunctionKeys, self).initialize(**kwargs)
        self.player = input_playback.InputPlayback()
        self.player.emulate(input_type='keyboard')
        self.player.find_connected_inputs()


    def cleanup(self):
        """
        Close playback and policy base class.

        """
        self.player.close()
        super(policy_KeyboardDefaultToFunctionKeys, self).cleanup()


    def _test_function_keys_default(self, policy_value):
        """
        Test default function keys action.

        Search+function keys should perform the alternate action.

        @param policy_value: policy value for this case.
        @raises error.TestFail if keypress differs from expected value.

        """
        # Get focus of the page
        self.player.blocking_playback_of_default_file(
            input_type='keyboard', filename='keyboard_enter')

        key_actions = ['BrowserForward', 'F2']

        if policy_value:
            key_actions = reversed(key_actions)

        for action, keys in zip(key_actions, ['f2', 'search+f2']):
            self._events.clear_previous_events()

            self.player.blocking_playback_of_default_file(
                input_type='keyboard', filename='keyboard_' + keys)

            events_log = self._events.get_events_log()
            logging.info('Events log: ' + events_log)

            if not re.search('key=' + action, events_log):
                raise error.TestFail(('policy_value: %s - typed: %s, '
                                     'expected: %s') %
                                     (policy_value, keys, action))


    def run_once(self, case):
        """
        Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.

        """
        case_value = self.TEST_CASES[case]
        self.setup_case(user_policies={self.POLICY_NAME: case_value},
                        init_network_controller=True)

        self._events = EventsPage(self.cr, self.bindir)
        self._test_function_keys_default(case_value)
