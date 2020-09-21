# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from autotest_lib.server.cros.cfm import cfm_base_test

LONG_TIMEOUT = 10


class enterprise_CFM_MeetAppSanity(cfm_base_test.CfmBaseTest):
    """
    Basic sanity test for Meet App to be expanded to cover more cases like
    enterprise_CFM_Sanity test.
    """
    version = 1


    def run_once(self):
        """Runs the test."""
        # Following triggers new Thor/Meetings APIs.
        self.cfm_facade.wait_for_meetings_telemetry_commands()
        self.cfm_facade.start_meeting_session()
        time.sleep(LONG_TIMEOUT)
        self.cfm_facade.end_meeting_session()

