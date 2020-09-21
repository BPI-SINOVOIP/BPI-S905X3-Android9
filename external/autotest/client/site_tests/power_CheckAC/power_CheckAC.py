# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.cros.power import power_status

class power_CheckAC(test.test):
    """Check the line status for AC power

    This is meant for verifying system setups in the lab to make sure that the
    AC line status can be remotely turned on and off.
    """
    version = 1


    def run_once(self, power_on=True):
        utils.poll_for_condition(
            lambda: power_status.get_status().on_ac() == power_on,
            timeout=10, exception=error.TestError('AC power not %d' % power_on))
