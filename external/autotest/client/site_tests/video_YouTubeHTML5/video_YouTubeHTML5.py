# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import httpd
from autotest_lib.client.cros.power import power_rapl
from autotest_lib.client.cros.video import youtube_helper
from autotest_lib.client.cros.video import helper_logger


FLASH_PROCESS_NAME = 'chrome/chrome --type=ppapi'
PLAYER_PLAYING_STATE = 'Playing'


class video_YouTubeHTML5(test.test):
    """This test verify the YouTube HTML5 video.

    - verify the video playback.
    - verify the available video resolutions.
    - verify the player functionalities.

    """
    version = 2

    def initialize(self):
        self._testServer = httpd.HTTPListener(8000, docroot=self.bindir)
        self._testServer.run()

    def cleanup(self):
        if self._testServer:
            self._testServer.stop()

        # Report rapl readings to chromeperf/ dashboard.
        if hasattr(self, 'rapl_rate') and self.rapl_rate:
            logging.info(self.rapl_rate)
            # Remove entries that we don't care about, eg. sample count.
            self.rapl_rate = {key: self.rapl_rate[key]
                              for key in self.rapl_rate.keys()
                              if key.endswith('_pwr')}
            for key, values in self.rapl_rate.iteritems():
                self.output_perf_value(
                    description=key,
                    value=values,
                    units='W',
                    higher_is_better=False,
                    graph='rapl_power_consumption'
                )

    def run_youtube_tests(self, browser):
        """Run YouTube HTML5 sanity tests.

        @param browser: The Browser object to run the test with.

        """
        tab = browser.tabs[0]
        tab.Navigate('http://localhost:8000/youtube5.html')
        yh = youtube_helper.YouTubeHelper(tab, power_logging=True)
        # Waiting for test video to load.
        yh.wait_for_player_state(PLAYER_PLAYING_STATE)

        # Set tiny resolution to prevent inadvertently caching a higher
        # bandwidth stream which taints resolution verification.
        yh.set_playback_quality('tiny')
        yh.set_video_duration()

        # Verify that YouTube is running in html5 mode.
        prc = utils.get_process_list('chrome', '--type=ppapi( |$)')
        if prc:
            raise error.TestFail('Running YouTube in Flash mode. '
                                 'Process list: %s.' % prc)

        tab.ExecuteJavaScript('player.mute()')
        yh.verify_video_playback()
        yh.verify_video_resolutions(power_measurement=True)
        yh.verify_player_states()
        self.rapl_rate = yh.get_power_measurement()

    @helper_logger.video_log_wrapper
    def run_once(self):
        with chrome.Chrome(
                extra_browser_args=helper_logger.chrome_vmodule_flag()) as cr:
            self.run_youtube_tests(cr.browser)
