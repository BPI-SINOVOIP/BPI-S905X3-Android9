# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test

class brillo_PowerMgmtInterfaces(test.test):
    """Verify presence of required power management kernel interfaces."""
    version = 1

    def check_cpuidle(self, host):
        cpuidle_driver = host.run_output('cat /sys/devices/system/cpu/cpuidle/current_driver')

        if cpuidle_driver == 'none' or cpuidle_driver == '':
            raise error.testFail('no cpuidle driver registered')

    def check_cpufreq(self, host):
        cpufreq_governors = host.run_output('cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors')

        if cpufreq_governors == '':
            raise error.testFail('no cpufreq governors registered')

    def check_wakelocks(self, host):
        wakelock_ls = host.run_output('ls /sys/power/wake_lock')

    def check_suspend(self, host):
        pm_states = host.run_output('cat /sys/power/state')
        if pm_states.find("mem") == -1:
            raise error.testFail('suspend-to-mem not supported')

    def run_once(self, host):
        """Run the Brillo power management kernel interfaces presence test.

        @param host: a host object representing the DUT.

        """
        self.check_cpuidle(host)
        self.check_cpufreq(host)
        self.check_wakelocks(host)
        self.check_suspend(host)
