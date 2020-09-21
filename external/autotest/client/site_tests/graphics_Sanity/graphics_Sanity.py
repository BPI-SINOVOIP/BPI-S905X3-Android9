# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.graphics import graphics_utils

# to run this test manually on a test target
# ssh root@machine
# cd /usr/local/autotest/deps/glbench
# stop ui
# ./windowmanagertest --screenshot1_sec 2 --screenshot2_sec 1 --cooldown_sec 1 \
#    --screenshot1_cmd \
#        "/usr/local/autotest/bin/screenshot.py screenshot1_generated.png" \
#    --screenshot2_cmd \
#        "/usr/local/autotest/bin/screenshot.py screenshot2_generated.png"
# start ui


class graphics_Sanity(graphics_utils.GraphicsTest):
    """
    This test is meant to be used as a quick sanity check for GL/GLES.
    """
    version = 1

    # None-init vars used by cleanup() here, in case setup() fails
    _services = None

    def setup(self):
        self.job.setup_dep(['glbench'])
        dep = 'glbench'
        dep_dir = os.path.join(self.autodir, 'deps', dep)
        self.job.install_pkg(dep, 'dep', dep_dir)

    def cleanup(self):
        super(graphics_Sanity, self).cleanup()
        if self._services:
            self._services.restore_services()

    def test_something_on_screen(self):
        """Check if something is drawn on screen: i.e. not a black screen.

        @raises TestFail if we cannot determine there was something on screen.
        """

        def can_take_screenshot():
            """Check that taking a screenshot can succeed.

            There are cases when trying to take a screenshot on the device
            fails. e.g. the display has gone to sleep, we have logged out and
            the UI has not come back up yet etc.
            """
            try:
                graphics_utils.take_screenshot(self.resultsdir,
                                               'temp screenshot', '.png')
                return True
            except:
                return False

        utils.poll_for_condition(
            can_take_screenshot,
            sleep_interval=1,
            desc='Failed to take a screenshot. There may be an issue with this '
            'ChromeOS image.')

        w, h = graphics_utils.get_internal_resolution()
        megapixels = (w * h) / 1000000
        filesize_threshold = 25 * megapixels
        screenshot1 = graphics_utils.take_screenshot(self.resultsdir,
                                                     'oobe or signin', '.png')

        with chrome.Chrome() as cr:
            tab = cr.browser.tabs[0]
            tab.Navigate('chrome://settings')
            tab.WaitForDocumentReadyStateToBeComplete()

            screenshot2 = graphics_utils.take_screenshot(
                self.resultsdir, 'settings page', '.png')

        for screenshot in [screenshot1, screenshot2]:
            file_size_kb = os.path.getsize(screenshot) / 1000

            # Use compressed file size to tell if anything is on screen.
            if file_size_kb > filesize_threshold:
                return

        raise error.TestFail(
            'Screenshot filesize is very small. This indicates that '
            'there is nothing on screen. This ChromeOS image could be '
            'unusable. Check the screenshot in the results folder.')

    def test_generated_screenshots_match_expectation(self):
        """Draws a texture with a soft ellipse twice and captures each image.
        Compares the output fuzzily against reference images.
        """
        self._services = service_stopper.ServiceStopper(['ui'])
        self._services.stop_services()

        screenshot1_reference = os.path.join(self.bindir,
                                             'screenshot1_reference.png')
        screenshot1_generated = os.path.join(self.resultsdir,
                                             'screenshot1_generated.png')
        screenshot1_resized = os.path.join(self.resultsdir,
                                           'screenshot1_generated_resized.png')
        screenshot2_reference = os.path.join(self.bindir,
                                             'screenshot2_reference.png')
        screenshot2_generated = os.path.join(self.resultsdir,
                                             'screenshot2_generated.png')
        screenshot2_resized = os.path.join(self.resultsdir,
                                           'screenshot2_generated_resized.png')

        exefile = os.path.join(self.autodir, 'deps/glbench/windowmanagertest')

        # Delay before screenshot: 1 second has caused failures.
        options = ' --screenshot1_sec 2'
        options += ' --screenshot2_sec 1'
        options += ' --cooldown_sec 1'
        # perceptualdiff can handle only 8 bit images.
        screenshot_cmd = ' "/usr/local/autotest/bin/screenshot.py %s"'
        options += ' --screenshot1_cmd' + screenshot_cmd % screenshot1_generated
        options += ' --screenshot2_cmd' + screenshot_cmd % screenshot2_generated

        cmd = exefile + ' ' + options
        utils.run(
            cmd, stdout_tee=utils.TEE_TO_LOGS, stderr_tee=utils.TEE_TO_LOGS)

        convert_cmd = ('convert -channel RGB -colorspace RGB -depth 8'
                       " -resize '100x100!' %s %s")
        utils.system(convert_cmd % (screenshot1_generated, screenshot1_resized))
        utils.system(convert_cmd % (screenshot2_generated, screenshot2_resized))
        os.remove(screenshot1_generated)
        os.remove(screenshot2_generated)

        diff_cmd = 'perceptualdiff -verbose %s %s'
        utils.system(diff_cmd % (screenshot1_reference, screenshot1_resized))
        utils.system(diff_cmd % (screenshot2_reference, screenshot2_resized))

    def run_once(self):
        if graphics_utils.get_display_resolution() is None:
            logging.warning('Skipping test because there is no screen')
            return
        self.add_failures('graphics_Sanity')
        self.wake_screen_with_keyboard()
        self.test_something_on_screen()
        self.test_generated_screenshots_match_expectation()
        self.remove_failures('graphics_Sanity')
