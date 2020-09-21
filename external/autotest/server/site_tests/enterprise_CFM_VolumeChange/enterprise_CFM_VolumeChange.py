# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import logging
import random
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.cfm import cfm_base_test

_SHORT_TIMEOUT = 2


class enterprise_CFM_VolumeChange(cfm_base_test.CfmBaseTest):
    """
    Volume changes made in the CFM / hotrod app should be accurately reflected
    in CrOS.
    """
    version = 1


    def _start_hangout_session(self):
        """
        Start a hangout session.

        Also unmutes the mic if it's muted after joining.

        @raises error.TestFail if any of the checks fail.
        """
        current_time = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
        hangout_name = 'auto-hangout-' + current_time

        self.cfm_facade.start_new_hangout_session(hangout_name)

        if self.cfm_facade.is_ready_to_start_hangout_session():
            raise error.TestFail('Is already in hangout session and should not '
                                 'be able to start another session.')
        time.sleep(_SHORT_TIMEOUT)

        if self.cfm_facade.is_mic_muted():
            self.cfm_facade.unmute_mic()


    def _change_volume(self, repeat, cmd):
        """
        Change volume using CFM api and cross check with cras_test_client
        output.

        @param repeat: Number of times the volume should be changed.
        @param cmd: cras_test_client command to run.
        @raises error.TestFail if cras volume does not match volume set by CFM.
        """
        # This is used to trigger crbug.com/614885
        for volume in range(55, 85):
            self.cfm_facade.set_speaker_volume(str(volume))
            time.sleep(random.uniform(0.01, 0.05))

        for _ in xrange(repeat):
            # There is a minimal volume threshold so we can't start at 0%.
            # See crbug.com/633809 for more info.
            cfm_volume = str(random.randrange(2, 100, 1))
            self.cfm_facade.set_speaker_volume(cfm_volume)
            time.sleep(_SHORT_TIMEOUT)

            cras_volume = [s.strip() for s in
                           self._host.run_output(cmd).splitlines()]
            for volume in cras_volume:
                if volume != cfm_volume:
                    raise error.TestFail('Cras volume (%s) does not match '
                                         'volume set by CFM (%s).' %
                                         (volume, cfm_volume))
            logging.info('Cras volume (%s) matches volume set by CFM (%s)',
                         cras_volume, cfm_volume)


    def run_once(self, repeat, cmd):
        """Runs the test."""
        self.cfm_facade.wait_for_hangouts_telemetry_commands()
        self._start_hangout_session()
        self._change_volume(repeat, cmd)
        self.cfm_facade.end_hangout_session()

