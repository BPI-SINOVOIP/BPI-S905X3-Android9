# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, numpy, random, time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import interface
from autotest_lib.client.cros.power import power_suspend, sys_power

class power_SuspendStress(test.test):
    """Class for test."""
    version = 1

    def initialize(self, duration, idle=False, init_delay=0, min_suspend=0,
                   min_resume=5, max_resume_window=3, check_connection=True,
                   iterations=None, suspend_state=''):
        """
        Entry point.

        @param duration: total run time of the test
        @param idle: use sys_power.idle_suspend method.
                (use with dummy_IdleSuspend)
        @param init_delay: wait this many seconds before starting the test to
                give parallel tests time to get started
        @param min_suspend: suspend durations will be chosen randomly out of
                the interval between min_suspend and min_suspend + 3 seconds.
        @param min_resume: minimal time in seconds between suspends.
        @param max_resume_window: maximum range to use between suspends. i.e.,
                we will stay awake between min_resume and min_resume +
                max_resume_window seconds.
        @param check_connection: If true, we check that the network interface
                used for testing is up after resume. Otherwsie we reboot.
        @param iterations: number of times to attempt suspend.  If !=None has
                precedence over duration.
        @param suspend_state: Force to suspend to a specific
                state ("mem" or "freeze"). If the string is empty, suspend
                state is left to the default pref on the system.
        """
        self._endtime = time.time()
        if duration:
            self._endtime += duration
        self._init_delay = init_delay
        self._min_suspend = min_suspend
        self._min_resume = min_resume
        self._max_resume_window = max_resume_window
        self._check_connection = check_connection
        self._iterations = iterations
        self._suspend_state = suspend_state
        self._method = sys_power.idle_suspend if idle else sys_power.do_suspend

    def _done(self):
        if self._iterations != None:
            self._iterations -= 1
            return self._iterations < 0
        return time.time() >= self._endtime

    def run_once(self):
        time.sleep(self._init_delay)
        self._suspender = power_suspend.Suspender(
                self.resultsdir, method=self._method,
                suspend_state=self._suspend_state)
        # Find the interface which is used for most communication.
        # We assume the interface connects to the gateway and has the lowest
        # metric.
        if self._check_connection:
            interface_choices={}
            with open('/proc/net/route') as fh:
                for line in fh:
                    fields = line.strip().split()
                    if fields[1] != '00000000' or not int(fields[3], 16) & 2:
                        continue
                    interface_choices[fields[0]] = fields[6]
            iface = interface.Interface(min(interface_choices))

        while not self._done():
            time.sleep(self._min_resume +
                       random.randint(0, self._max_resume_window))
            # Check the network interface to the caller is still available
            if self._check_connection:
                # Give a 10 second window for the network to come back.
                try:
                    utils.poll_for_condition(iface.is_link_operational,
                                             desc='Link is operational')
                except utils.TimeoutError:
                    logging.error('Link to the server gone, reboot')
                    utils.system('reboot')

            self._suspender.suspend(random.randint(0, 3) + self._min_suspend)


    def postprocess_iteration(self):
        if self._suspender.successes:
            keyvals = {'suspend_iterations': len(self._suspender.successes)}
            for key in self._suspender.successes[0]:
                values = [result[key] for result in self._suspender.successes]
                keyvals[key + '_mean'] = numpy.mean(values)
                keyvals[key + '_stddev'] = numpy.std(values)
                keyvals[key + '_min'] = numpy.amin(values)
                keyvals[key + '_max'] = numpy.amax(values)
            self.write_perf_keyval(keyvals)
        if self._suspender.failures:
            total = len(self._suspender.failures)
            iterations = len(self._suspender.successes) + total
            timeout = kernel = firmware = spurious = 0
            for failure in self._suspender.failures:
                if type(failure) is sys_power.SuspendTimeout: timeout += 1
                if type(failure) is sys_power.KernelError: kernel += 1
                if type(failure) is sys_power.FirmwareError: firmware += 1
                if type(failure) is sys_power.SpuriousWakeupError: spurious += 1
            if total == kernel + timeout:
                raise error.TestWarn('%d non-fatal suspend failures in %d '
                        'iterations (%d timeouts, %d kernel warnings)' %
                        (total, iterations, timeout, kernel))
            if total == 1:
                # just throw it as is, makes aggregation on dashboards easier
                raise self._suspender.failures[0]
            raise error.TestFail('%d suspend failures in %d iterations (%d '
                    'timeouts, %d kernel warnings, %d firmware errors, %d '
                    'spurious wakeups)' %
                    (total, iterations, timeout, kernel, firmware, spurious))


    def cleanup(self):
        """
        Clean this up before we wait ages for all the log copying to finish...
        """
        self._suspender.finalize()
