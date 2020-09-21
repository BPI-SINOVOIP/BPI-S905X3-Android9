# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime, logging, time

from autotest_lib.server.cros.cfm import cfm_base_test


LONG_TIMEOUT = 8
SHORT_TIMEOUT = 5


class enterprise_CFM_SessionStress(cfm_base_test.CfmBaseTest):
    """Stress tests the device in CFM kiosk mode by initiating a new hangout
    session multiple times.
    """
    version = 1


    def _run_hangout_session(self):
        """Start a hangout session and do some checks before ending the session.

        @raises error.TestFail if any of the checks fail.
        """
        current_time = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
        hangout_name = 'auto-hangout-' + current_time
        logging.info('Session name: %s', hangout_name)

        self.cfm_facade.start_new_hangout_session(hangout_name)
        time.sleep(LONG_TIMEOUT)
        self.cfm_facade.end_hangout_session()


    def run_once(self, repeat):
        """Runs the test."""
        self.cfm_facade.wait_for_hangouts_telemetry_commands()
        for i in range(repeat):
            logging.info('Running iteration #%d...', i)
            self._run_hangout_session()
            time.sleep(SHORT_TIMEOUT)

