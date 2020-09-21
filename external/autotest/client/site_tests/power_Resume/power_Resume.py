# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.power import power_suspend


# In cases like crosbug.com/p/26289, we want results, but also want
# to make sure we are suspending quickly enough. Retry with this amount
# of extra suspend time to make sure we can get some results, even
# if we throw a warning.
EXTRA_TIME = 10


class power_Resume(test.test):
    """class for power_Resume test."""
    version = 1
    preserve_srcdir = True

    def initialize(self, suspend_state=''):
        """
        Entry point.

        @param suspend_state: Force to suspend to a specific
                state ("mem" or "freeze"). If the string is empty, suspend
                state is left to the default pref on the system.
        """
        self._suspender = power_suspend.Suspender(
                self.resultsdir,
                throw=True,
                device_times=True,
                suspend_state=suspend_state)


    def run_once(self, max_devs_returned=10, seconds=0,
                 ignore_kernel_warns=False, measure_arc=False):
        try:
            self._suspend_once(max_devs_returned, seconds, ignore_kernel_warns,
                               measure_arc)
        except error.TestWarn:
            self._suspend_once(max_devs_returned, seconds + EXTRA_TIME,
                               ignore_kernel_warns, measure_arc)
            raise


    def _suspend_once(self, max_devs_returned, seconds, ignore_kernel_warns,
                      measure_arc=False):
        (results, device_times) = \
            self._suspender.suspend(seconds,
                                    ignore_kernel_warns=ignore_kernel_warns,
                                    measure_arc=measure_arc)

        # return as keyvals the slowest n devices
        slowest_devs = sorted(
            device_times,
            key=device_times.get,
            reverse=True)[:max_devs_returned]
        for dev in slowest_devs:
            results[dev] = device_times[dev]

        self.output_perf_value(description='system_suspend',
                               value=results['seconds_system_suspend'],
                               units='sec', higher_is_better=False)
        self.output_perf_value(description='system_resume',
                               value=results['seconds_system_resume'],
                               units='sec', higher_is_better=False)
        self.write_perf_keyval(results)
