# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server.cros.update_engine import update_engine_test

class autoupdate_NonBlockingOOBEUpdate(update_engine_test.UpdateEngineTest):
    """Try a non-forced autoupdate during OOBE."""
    version = 1

    # We override the default lsb-release file.
    _CUSTOM_LSB_RELEASE = '/mnt/stateful_partition/etc/lsb-release'


    def cleanup(self):
        self._host.run('rm %s' % self._CUSTOM_LSB_RELEASE, ignore_status=True)
        self._host.get_file('/var/log/update_engine.log', self.resultsdir)
        super(autoupdate_NonBlockingOOBEUpdate, self).cleanup()


    def run_once(self, host, full_payload=True, job_repo_url=None):
        """
        Trys an autoupdate during ChromeOS OOBE without a deadline.

        @param host: The DUT that we are running on.
        @param full_payload: True for a full payload. False for delta.
        @param job_repo_url: Used for debugging locally. This is used to figure
                             out the current build and the devserver to use.
                             The test will read this from a host argument
                             when run in the lab.

        """
        self._host = host

        # veyron_rialto is a medical device with a different OOBE that auto
        # completes so this test is not valid on that device.
        if 'veyron_rialto' in self._host.get_board():
            raise error.TestNAError('Rialto has a custom OOBE. Skipping test.')

        update_url = self.get_update_url_for_test(job_repo_url,
                                                  full_payload=full_payload,
                                                  critical_update=False)
        logging.info('Update url: %s', update_url)

        # Call client test to start the OOBE update.
        client_at = autotest.Autotest(self._host)
        client_at.run_test('autoupdate_StartOOBEUpdate', image_url=update_url,
                           full_payload=full_payload, critical_update=False)

        # Ensure that the update failed as expected.
        err_msg = 'finished OmahaRequestAction with code ' \
                  'ErrorCode::kNonCriticalUpdateInOOBE'
        output = self._host.run('cat /var/log/update_engine.log | grep "%s"'
                                % err_msg, ignore_status=True).exit_status
        if output != 0:
            raise error.TestFail('Update did not fail with '
                                 'kNonCriticalUpdateInOOBE')