# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.power import power_status, power_utils
from autotest_lib.client.cros.video import histogram_verifier
from autotest_lib.client.cros.video import constants
from autotest_lib.client.cros.video import helper_logger


EXTRA_BROWSER_ARGS = ['--use-fake-device-for-media-stream',
                      '--use-fake-ui-for-media-stream']
FAKE_FILE_ARG = '--use-file-for-fake-video-capture="%s"'
DISABLE_ACCELERATED_VIDEO_DECODE_BROWSER_ARGS = [
        '--disable-accelerated-video-decode']

DOWNLOAD_BASE = ('http://commondatastorage.googleapis.com/'
                 'chromiumos-test-assets-public/crowd/')
VIDEO_NAME = 'crowd720_25frames.y4m'

WEBRTC_INTERNALS_URL = 'chrome://webrtc-internals'

WEBRTC_WITH_HW_ACCELERATION = 'webrtc_with_hw_acceleration'
WEBRTC_WITHOUT_HW_ACCELERATION = 'webrtc_without_hw_acceleration'

# Measurement duration in seconds.
MEASUREMENT_DURATION = 30
# Time to exclude from calculation after start the loopback [seconds].
STABILIZATION_DURATION = 10

# List of thermal throttling services that should be disabled.
# - temp_metrics for link.
# - thermal for daisy, snow, pit etc.
THERMAL_SERVICES = ['temp_metrics', 'thermal']

# Time in seconds to wait for cpu idle until giving up.
WAIT_FOR_IDLE_CPU_TIMEOUT = 60.0
# Maximum percent of cpu usage considered as idle.
CPU_IDLE_USAGE = 0.1

MAX_DECODE_TIME_DESCRIPTION = 'decode_time.max'
MEDIAN_DECODE_TIME_DESCRIPTION = 'decode_time.percentile_0.50'
CPU_USAGE_DESCRIPTION = 'video_cpu_usage'
POWER_DESCRIPTION = 'video_mean_energy_rate'

# Minimum battery charge percentage to run the power test.
BATTERY_INITIAL_CHARGED_MIN = 10

# These are the variable names in WebRTC internals.
# Maximum decode time of the frames of the last 10 seconds.
GOOG_MAX_DECODE_MS = 'googMaxDecodeMs'
# The decode time of the last frame.
GOOG_DECODE_MS = 'googDecodeMs'

# Javascript function to get the decode time. addStats is a function called by
# Chrome to pass WebRTC statistics every second.
ADD_STATS_JAVASCRIPT = (
        'var googMaxDecodeMs = new Array();'
        'var googDecodeMs = new Array();'
        'addStats = function(data) {'
        '  reports = data.reports;'
        '  for (var i=0; i < reports.length; i++) {'
        '    if (reports[i].type == "ssrc") {'
        '      values = reports[i].stats.values;'
        '      for (var j=0; j < values.length; j++) {'
        '        if (values[j] == "googMaxDecodeMs")'
        '          googMaxDecodeMs[googMaxDecodeMs.length] = values[j+1];'
        '        else if (values[j] == "googDecodeMs")'
        '          googDecodeMs[googDecodeMs.length] = values[j+1];'
        '      }'
        '    }'
        '  }'
        '}')

# Measure the stats until getting 10 samples or exceeding 30 seconds. addStats
# is called once per second for now.
NUM_DECODE_TIME_SAMPLES = 10
TIMEOUT = 60


class video_WebRtcPerf(test.test):
    """
    The test outputs the decode time, cpu usage and the power consumption for
    WebRTC to performance dashboard.
    """
    version = 1
    arc_mode = None


    def initialize(self):
        self._service_stopper = None
        self._original_governors = None
        self._backlight = None


    def cleanup(self):
        if self._backlight:
            self._backlight.restore()
        if self._service_stopper:
            self._service_stopper.restore_services()
        if self._original_governors:
            utils.restore_scaling_governor_states(self._original_governors)

        super(video_WebRtcPerf, self).cleanup()


    def start_loopback(self, cr):
        """
        Opens WebRTC loopback page.

        @param cr: Autotest Chrome instance.
        """
        cr.browser.platform.SetHTTPServerDirectories(self.bindir)

        tab = cr.browser.tabs[0]
        tab.Navigate(cr.browser.platform.http_server.UrlOf(
                os.path.join(self.bindir, 'loopback.html')))
        tab.WaitForDocumentReadyStateToBeComplete()


    def open_stats_page(self, cr):
        """
        Opens WebRTC internal statistics page.

        @param cr: Autotest Chrome instance.

        @returns the tab of the stats page.
        """
        tab = cr.browser.tabs.New()
        tab.Navigate(WEBRTC_INTERNALS_URL)
        tab.WaitForDocumentReadyStateToBeComplete()
        # Inject stats callback function.
        tab.EvaluateJavaScript(ADD_STATS_JAVASCRIPT)
        return tab


    @helper_logger.video_log_wrapper
    def run_once(self, decode_time_test=False, cpu_test=False,
                 power_test=False, arc_mode=None):
        """
        Runs the video_WebRtcPerf test.

        @param decode_time_test: Pass True to run decode time test.
        @param cpu_test: Pass True to run CPU usage test.
        @param power_test: Pass True to run power consumption test.
        @param arc_mode: if 'enabled', run the test with Android enabled.
        """
        # Download test video.
        url = DOWNLOAD_BASE + VIDEO_NAME
        local_path = os.path.join(self.bindir, VIDEO_NAME)
        file_utils.download_file(url, local_path)
        self.arc_mode = arc_mode

        if decode_time_test:
            keyvals = self.test_decode_time(local_path)
            # The first value is max decode time. The second value is median
            # decode time. Construct a dictionary containing one value to log
            # the result.
            max_decode_time = {
                    key:value[0] for (key, value) in keyvals.items()}
            self.log_result(max_decode_time, MAX_DECODE_TIME_DESCRIPTION,
                            'milliseconds')
            median_decode_time = {
                    key:value[1] for (key, value) in keyvals.items()}
            self.log_result(median_decode_time, MEDIAN_DECODE_TIME_DESCRIPTION,
                            'milliseconds')

        if cpu_test:
            keyvals = self.test_cpu_usage(local_path)
            self.log_result(keyvals, CPU_USAGE_DESCRIPTION, 'percent')

        if power_test:
            keyvals = self.test_power(local_path)
            self.log_result(keyvals, POWER_DESCRIPTION , 'W')


    def test_webrtc(self, local_path, gather_result):
        """
        Runs the webrtc test with and without hardware acceleration.

        @param local_path: the path to the video file.
        @param gather_result: a function to run and return the test result
                after chrome opens. The input parameter of the funciton is
                Autotest chrome instance.

        @return a dictionary that contains test the result.
        """
        keyvals = {}
        EXTRA_BROWSER_ARGS.append(FAKE_FILE_ARG % local_path)

        with chrome.Chrome(extra_browser_args=EXTRA_BROWSER_ARGS +\
                           [helper_logger.chrome_vmodule_flag()],
                           arc_mode=self.arc_mode,
                           init_network_controller=True) as cr:
            # On daisy, Chrome freezes about 30 seconds after login because of
            # TPM error. See http://crbug.com/588579.
            if utils.get_board() == 'daisy':
              logging.warning('Delay 30s for issue 588579 on daisy')
              time.sleep(30)
            # Open WebRTC loopback page and start the loopback.
            self.start_loopback(cr)
            result = gather_result(cr)

            # Check if decode is hardware accelerated.
            if histogram_verifier.is_bucket_present(
                    cr,
                    constants.RTC_INIT_HISTOGRAM,
                    constants.RTC_VIDEO_INIT_BUCKET):
                keyvals[WEBRTC_WITH_HW_ACCELERATION] = result
            else:
                logging.info("Can not use hardware decoding.")
                keyvals[WEBRTC_WITHOUT_HW_ACCELERATION] = result
                return keyvals

        # Start chrome with disabled video hardware decode flag.
        with chrome.Chrome(extra_browser_args=
                DISABLE_ACCELERATED_VIDEO_DECODE_BROWSER_ARGS +
                EXTRA_BROWSER_ARGS, arc_mode=self.arc_mode,
                init_network_controller=True) as cr:

            # crbug/753292 - enforce the idle checks after login
            if not utils.wait_for_idle_cpu(WAIT_FOR_IDLE_CPU_TIMEOUT,
                                           CPU_IDLE_USAGE):
                logging.warning('Could not get idle CPU post login.')
            if not utils.wait_for_cool_machine():
                logging.warning('Could not get cold machine post login.')

            if utils.get_board() == 'daisy':
              logging.warning('Delay 30s for issue 588579 on daisy')
              time.sleep(30)
            # Open the webrtc loopback page and start the loopback.
            self.start_loopback(cr)
            result = gather_result(cr)

            # Make sure decode is not hardware accelerated.
            if histogram_verifier.is_bucket_present(
                    cr,
                    constants.RTC_INIT_HISTOGRAM,
                    constants.RTC_VIDEO_INIT_BUCKET):
                raise error.TestError('HW decode should not be used.')
            keyvals[WEBRTC_WITHOUT_HW_ACCELERATION] = result

        return keyvals


    def test_cpu_usage(self, local_path):
        """
        Runs the video cpu usage test.

        @param local_path: the path to the video file.

        @return a dictionary that contains the test result.
        """
        def get_cpu_usage(cr):
            time.sleep(STABILIZATION_DURATION)
            cpu_usage_start = utils.get_cpu_usage()
            time.sleep(MEASUREMENT_DURATION)
            cpu_usage_end = utils.get_cpu_usage()
            return utils.compute_active_cpu_time(cpu_usage_start,
                                                      cpu_usage_end) * 100

        # crbug/753292 - APNG login pictures increase CPU usage. Move the more
        # strict idle checks after the login phase.
        utils.wait_for_idle_cpu(WAIT_FOR_IDLE_CPU_TIMEOUT, CPU_IDLE_USAGE)
        utils.wait_for_cool_machine()
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
        return self.test_webrtc(local_path, get_cpu_usage)


    def test_power(self, local_path):
        """
        Runs the video power consumption test.

        @param local_path: the path to the video file.

        @return a dictionary that contains the test result.
        """
        self._backlight = power_utils.Backlight()
        self._backlight.set_default()
        self._service_stopper = service_stopper.ServiceStopper(
                service_stopper.ServiceStopper.POWER_DRAW_SERVICES)
        self._service_stopper.stop_services()

        current_power_status = power_status.get_status()
        # We expect the DUT is powered by battery now. But this is not always
        # true due to other bugs. Disable this test temporarily as workaround.
        # TODO(kcwu): remove this workaround after AC control is stable
        #             crbug.com/723968
        if current_power_status.on_ac():
            logging.warning('Still powered by AC. Skip this test')
            return {}
        # Verify that the battery is sufficiently charged.
        current_power_status.assert_battery_state(BATTERY_INITIAL_CHARGED_MIN)

        measurements = [power_status.SystemPower(
                current_power_status.battery_path)]

        def get_power(cr):
            power_logger = power_status.PowerLogger(measurements)
            power_logger.start()
            time.sleep(STABILIZATION_DURATION)
            start_time = time.time()
            time.sleep(MEASUREMENT_DURATION)
            power_logger.checkpoint('result', start_time)
            keyval = power_logger.calc()
            return keyval['result_' + measurements[0].domain + '_pwr']

        return self.test_webrtc(local_path, get_power)


    def test_decode_time(self, local_path):
        """
        Runs the decode time test.

        @param local_path: the path to the video file.

        @return a dictionary that contains the test result.
        """
        def get_decode_time(cr):
            tab = self.open_stats_page(cr)
            # Collect the decode time until there are enough samples.
            start_time = time.time()
            max_decode_time_list = []
            decode_time_list = []
            while (time.time() - start_time < TIMEOUT and
                   len(decode_time_list) < NUM_DECODE_TIME_SAMPLES):
                time.sleep(1)
                max_decode_time_list = []
                decode_time_list = []
                try:
                    max_decode_time_list = [int(x) for x in
                            tab.EvaluateJavaScript(GOOG_MAX_DECODE_MS)]
                    decode_time_list = [int(x) for x in
                            tab.EvaluateJavaScript(GOOG_DECODE_MS)]
                except:
                    pass
            # Output the values if they are valid.
            if len(max_decode_time_list) < NUM_DECODE_TIME_SAMPLES:
                raise error.TestError('Not enough ' + GOOG_MAX_DECODE_MS)
            if len(decode_time_list) < NUM_DECODE_TIME_SAMPLES:
                raise error.TestError('Not enough ' + GOOG_DECODE_MS)
            max_decode_time = max(max_decode_time_list)
            decode_time_median = self.get_median(decode_time_list)
            logging.info("Max decode time list=%s", str(max_decode_time_list))
            logging.info("Decode time list=%s", str(decode_time_list))
            logging.info("Maximum decode time=%d, median=%d", max_decode_time,
                         decode_time_median)
            return (max_decode_time, decode_time_median)

        return self.test_webrtc(local_path, get_decode_time)


    def get_median(self, seq):
        """
        Calculates the median of a sequence of numbers.

        @param seq: a list with numbers.

        @returns the median of the numbers.
        """
        seq.sort()
        size = len(seq)
        if size % 2 != 0:
            return seq[size / 2]
        return (seq[size / 2] + seq[size / 2 - 1]) / 2.0

    def log_result(self, keyvals, description, units):
        """
        Logs the test result output to the performance dashboard.

        @param keyvals: a dictionary that contains results returned by
                test_webrtc.
        @param description: a string that describes the video and test result
                and it will be part of the entry name in the dashboard.
        @param units: the units of test result.
        """
        result_with_hw = keyvals.get(WEBRTC_WITH_HW_ACCELERATION)
        if result_with_hw:
            self.output_perf_value(
                    description= 'hw_' + description, value=result_with_hw,
                    units=units, higher_is_better=False)

        result_without_hw = keyvals.get(WEBRTC_WITHOUT_HW_ACCELERATION)
        if result_without_hw:
            self.output_perf_value(
                    description= 'sw_' + description, value=result_without_hw,
                    units=units, higher_is_better=False)
