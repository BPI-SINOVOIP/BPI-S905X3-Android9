# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import threading
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import chrome_binary_test
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.power import power_status
from autotest_lib.client.cros.power import power_utils
from autotest_lib.client.cros.video import helper_logger

DECODE_WITH_HW_ACCELERATION = 'jpeg_decode_with_hw'
DECODE_WITHOUT_HW_ACCELERATION = 'jpeg_decode_with_sw'

# Measurement duration in seconds.
MEASUREMENT_DURATION = 10

# List of thermal throttling services that should be disabled.
# - temp_metrics for link.
# - thermal for daisy, snow, pit etc.
THERMAL_SERVICES = ['temp_metrics', 'thermal']

# Time in seconds to wait for cpu idle until giveup.
WAIT_FOR_IDLE_CPU_TIMEOUT = 60.0
# Maximum percent of cpu usage considered as idle.
CPU_IDLE_USAGE = 0.1

# Minimum battery charge percentage to run the test
BATTERY_INITIAL_CHARGED_MIN = 10


class video_JDAPerf(chrome_binary_test.ChromeBinaryTest):
    """
    The test outputs the cpu usage and the power consumption for jpeg decoding
    to performance dashboard.
    """
    version = 1

    # Decoding times for performance measurement
    perf_jpeg_decode_times = 600

    binary = 'jpeg_decode_accelerator_unittest'
    jda_filter = 'JpegDecodeAcceleratorTest.PerfJDA'
    sw_filter = 'JpegDecodeAcceleratorTest.PerfSW'

    def initialize(self):
        """Initialize this test."""
        super(video_JDAPerf, self).initialize()
        self._service_stopper = None
        self._original_governors = None
        self._backlight = None
        self._use_ec = False

    @chrome_binary_test.nuke_chrome
    def run_once(self, power_test=False):
        """
        Runs the video_JDAPerf test.

        @param power_test: True for power consumption test.
                           False for cpu usage test.
        """
        if power_test:
            keyvals = self.test_power()
            self.log_result(keyvals, 'jpeg_decode_energy', 'W')
        else:
            keyvals = self.test_cpu_usage()
            self.log_result(keyvals, 'jpeg_decode_cpu', 'percent')

    def test_cpu_usage(self):
        """
        Runs the video cpu usage test.

        @return a dictionary that contains the test result.
        """
        def get_cpu_usage():
            cpu_usage_start = utils.get_cpu_usage()
            time.sleep(MEASUREMENT_DURATION)
            cpu_usage_end = utils.get_cpu_usage()
            return utils.compute_active_cpu_time(cpu_usage_start,
                                                 cpu_usage_end) * 100

        # crbug/753292 - APNG login pictures increase CPU usage. Move the more
        # strict idle checks after the login phase.
        if not utils.wait_for_idle_cpu(WAIT_FOR_IDLE_CPU_TIMEOUT,
                                       CPU_IDLE_USAGE):
            logging.warning('Could not get idle CPU pre login.')
        if not utils.wait_for_cool_machine():
            logging.warning('Could not get cold machine pre login.')

        # Stop the thermal service that may change the cpu frequency.
        self._service_stopper = service_stopper.ServiceStopper(THERMAL_SERVICES)
        self._service_stopper.stop_services()
        # Set the scaling governor to performance mode to set the cpu to the
        # highest frequency available.
        self._original_governors = utils.set_high_performance_mode()
        return self.test_decode(get_cpu_usage)

    def test_power(self):
        """
        Runs the video power consumption test.

        @return a dictionary that contains the test result.
        """

        self._backlight = power_utils.Backlight()
        self._backlight.set_level(0)

        self._power_status = power_status.get_status()

        self._use_ec = True
        if not power_utils.charge_control_by_ectool(is_charge=False):
            logging.warning('Can\'t stop charging')
            return {}

        if not self._power_status.battery:
            raise error.TestFail('No valid battery')

        # Verify that the battery is sufficiently charged.
        percent_initial_charge = self._power_status.percent_current_charge()
        if percent_initial_charge < BATTERY_INITIAL_CHARGED_MIN:
            logging.warning('Initial charge (%f) less than min (%f)',
                            (percent_initial_charge,
                             BATTERY_INITIAL_CHARGED_MIN))
            return {}

        measurements = [power_status.SystemPower(
                self._power_status.battery_path)]

        def get_power():
            power_logger = power_status.PowerLogger(measurements)
            power_logger.start()
            start_time = time.time()
            time.sleep(MEASUREMENT_DURATION)
            power_logger.checkpoint('result', start_time)
            keyval = power_logger.calc()
            return keyval['result_' + measurements[0].domain + '_pwr_avg']

        return self.test_decode(get_power)

    def start_decode(self, gtest_filter):
        """
        Start jpeg decode process.

        @param gtest_filter: gtest_filter argument.
        """
        logging.debug('Starting video_JpegDecodeAccelerator %s', gtest_filter)
        cmd_line_list = [helper_logger.chrome_vmodule_flag()]
        cmd_line_list.append('--gtest_filter="%s"' % gtest_filter)
        cmd_line_list.append('--perf_decode_times=%d' %
                             self.perf_jpeg_decode_times)

        cmd_line = ' '.join(cmd_line_list)
        self.run_chrome_test_binary(self.binary, cmd_line)

    def test_decode(self, gather_result):
        """
        Runs the jpeg decode test with and without hardware acceleration.

        @param gather_result: a function to run and return the test result

        @return a dictionary that contains test the result.
        """
        keyvals = {}

        thread = threading.Thread(target=self.start_decode,
                                  kwargs={'gtest_filter': self.jda_filter})
        thread.start()
        result = gather_result()
        thread.join()
        keyvals[DECODE_WITH_HW_ACCELERATION] = result

        thread = threading.Thread(target=self.start_decode,
                                  kwargs={'gtest_filter': self.sw_filter})
        thread.start()
        result = gather_result()
        thread.join()
        keyvals[DECODE_WITHOUT_HW_ACCELERATION] = result

        return keyvals

    def log_result(self, keyvals, description, units):
        """
        Logs the test result output to the performance dashboard.

        @param keyvals: a dictionary that contains results returned by
                test_playback.
        @param description: a string that describes the video and test result
                and it will be part of the entry name in the dashboard.
        @param units: the units of test result.
        """
        result_with_hw = keyvals.get(DECODE_WITH_HW_ACCELERATION)
        if result_with_hw is not None:
            self.output_perf_value(description='hw_' + description,
                                   value=result_with_hw,
                                   units=units, higher_is_better=False)

        result_without_hw = keyvals.get(DECODE_WITHOUT_HW_ACCELERATION)
        if result_without_hw is not None:
            self.output_perf_value(
                    description='sw_' + description, value=result_without_hw,
                    units=units, higher_is_better=False)

    def cleanup(self):
        """Autotest cleanup function

        It is run by common_lib/test.py.
        """
        if self._backlight:
            self._backlight.restore()
        if self._service_stopper:
            self._service_stopper.restore_services()
        if self._original_governors:
            utils.restore_scaling_governor_states(self._original_governors)
        if self._use_ec:
            power_utils.charge_control_by_ectool(is_charge=True)
        super(video_JDAPerf, self).cleanup()
