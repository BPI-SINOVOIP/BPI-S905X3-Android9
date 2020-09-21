# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import alsa_utils


class audio_Aconnect(test.test):
    """Checks that the MIDI seq kernel interface is accessible."""
    version = 1


    def run_once(self):
        """Ensure we can access at least 1 seq client.

        A properly functioning seq system has at least the System
        client.
        """

        if alsa_utils.get_num_seq_clients() == 0:
            raise error.TestFail('There is no MIDI seq client')
