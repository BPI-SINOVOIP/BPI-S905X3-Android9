# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Facade to access the display-related functionality."""

import logging
import multiprocessing
import numpy
import os
import re
import shutil
import time
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils as common_utils
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.client.cros import constants
from autotest_lib.client.cros.graphics import graphics_utils
from autotest_lib.client.cros.multimedia import facade_resource
from autotest_lib.client.cros.multimedia import image_generator
from autotest_lib.client.cros.power import sys_power
from telemetry.internal.browser import web_contents

class TimeoutException(Exception):
    """Timeout Exception class."""
    pass


_FLAKY_CALL_RETRY_TIMEOUT_SEC = 60
_FLAKY_DISPLAY_CALL_RETRY_DELAY_SEC = 2

_retry_display_call = retry.retry(
        (KeyError, error.CmdError),
        timeout_min=_FLAKY_CALL_RETRY_TIMEOUT_SEC / 60.0,
        delay_sec=_FLAKY_DISPLAY_CALL_RETRY_DELAY_SEC)


class DisplayFacadeNative(object):
    """Facade to access the display-related functionality.

    The methods inside this class only accept Python native types.
    """

    CALIBRATION_IMAGE_PATH = '/tmp/calibration.png'
    MINIMUM_REFRESH_RATE_EXPECTED = 25.0
    DELAY_TIME = 3
    MAX_TYPEC_PORT = 6

    def __init__(self, resource):
        """Initializes a DisplayFacadeNative.

        @param resource: A FacadeResource object.
        """
        self._resource = resource
        self._image_generator = image_generator.ImageGenerator()


    @facade_resource.retry_chrome_call
    def get_display_info(self):
        """Gets the display info from Chrome.system.display API.

        @return array of dict for display info.
        """
        extension = self._resource.get_extension(
                constants.DISPLAY_TEST_EXTENSION)
        extension.ExecuteJavaScript('window.__display_info = null;')
        extension.ExecuteJavaScript(
                "chrome.system.display.getInfo(function(info) {"
                "window.__display_info = info;})")
        utils.wait_for_value(lambda: (
                extension.EvaluateJavaScript("window.__display_info") != None),
                expected_value=True)
        return extension.EvaluateJavaScript("window.__display_info")


    @facade_resource.retry_chrome_call
    def get_window_info(self):
        """Gets the current window info from Chrome.system.window API.

        @return a dict for the information of the current window.
        """
        extension = self._resource.get_extension()
        extension.ExecuteJavaScript('window.__window_info = null;')
        extension.ExecuteJavaScript(
                "chrome.windows.getCurrent(function(info) {"
                "window.__window_info = info;})")
        utils.wait_for_value(lambda: (
                extension.EvaluateJavaScript("window.__window_info") != None),
                expected_value=True)
        return extension.EvaluateJavaScript("window.__window_info")


    def _get_display_by_id(self, display_id):
        """Gets a display by ID.

        @param display_id: id of the display.

        @return: A dict of various display info.
        """
        for display in self.get_display_info():
            if display['id'] == display_id:
                return display
        raise RuntimeError('Cannot find display ' + display_id)


    def get_display_modes(self, display_id):
        """Gets all the display modes for the specified display.

        @param display_id: id of the display to get modes from.

        @return: A list of DisplayMode dicts.
        """
        display = self._get_display_by_id(display_id)
        return display['modes']


    def get_display_rotation(self, display_id):
        """Gets the display rotation for the specified display.

        @param display_id: id of the display to get modes from.

        @return: Degree of rotation.
        """
        display = self._get_display_by_id(display_id)
        return display['rotation']


    def set_display_rotation(self, display_id, rotation,
                             delay_before_rotation=0, delay_after_rotation=0):
        """Sets the display rotation for the specified display.

        @param display_id: id of the display to get modes from.
        @param rotation: degree of rotation
        @param delay_before_rotation: time in second for delay before rotation
        @param delay_after_rotation: time in second for delay after rotation
        """
        time.sleep(delay_before_rotation)
        extension = self._resource.get_extension(
                constants.DISPLAY_TEST_EXTENSION)
        extension.ExecuteJavaScript(
                """
                window.__set_display_rotation_has_error = null;
                chrome.system.display.setDisplayProperties('%(id)s',
                    {"rotation": %(rotation)d}, () => {
                    if (chrome.runtime.lastError) {
                        console.error('Failed to set display rotation',
                            chrome.runtime.lastError);
                        window.__set_display_rotation_has_error = "failure";
                    } else {
                        window.__set_display_rotation_has_error = "success";
                    }
                });
                """
                % {'id': display_id, 'rotation': rotation}
        )
        utils.wait_for_value(lambda: (
                extension.EvaluateJavaScript(
                    'window.__set_display_rotation_has_error') != None),
                expected_value=True)
        time.sleep(delay_after_rotation)
        result = extension.EvaluateJavaScript(
                'window.__set_display_rotation_has_error')
        if result != 'success':
            raise RuntimeError('Failed to set display rotation: %r' % result)


    def get_available_resolutions(self, display_id):
        """Gets the resolutions from the specified display.

        @return a list of (width, height) tuples.
        """
        display = self._get_display_by_id(display_id)
        modes = display['modes']
        if 'widthInNativePixels' not in modes[0]:
            raise RuntimeError('Cannot find widthInNativePixels attribute')
        if display['isInternal']:
            logging.info("Getting resolutions of internal display")
            return list(set([(mode['width'], mode['height']) for mode in
                             modes]))
        return list(set([(mode['widthInNativePixels'],
                          mode['heightInNativePixels']) for mode in modes]))


    def get_internal_display_id(self):
        """Gets the internal display id.

        @return the id of the internal display.
        """
        for display in self.get_display_info():
            if display['isInternal']:
                return display['id']
        raise RuntimeError('Cannot find internal display')


    def get_first_external_display_id(self):
        """Gets the first external display id.

        @return the id of the first external display; -1 if not found.
        """
        # Get the first external and enabled display
        for display in self.get_display_info():
            if display['isEnabled'] and not display['isInternal']:
                return display['id']
        return -1


    def set_resolution(self, display_id, width, height, timeout=3):
        """Sets the resolution of the specified display.

        @param display_id: id of the display to set resolution for.
        @param width: width of the resolution
        @param height: height of the resolution
        @param timeout: maximal time in seconds waiting for the new resolution
                to settle in.
        @raise TimeoutException when the operation is timed out.
        """

        extension = self._resource.get_extension(
                constants.DISPLAY_TEST_EXTENSION)
        extension.ExecuteJavaScript(
                """
                window.__set_resolution_progress = null;
                chrome.system.display.getInfo((info_array) => {
                    var mode;
                    for (var info of info_array) {
                        if (info['id'] == '%(id)s') {
                            for (var m of info['modes']) {
                                if (m['width'] == %(width)d &&
                                    m['height'] == %(height)d) {
                                    mode = m;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    if (mode === undefined) {
                        console.error('Failed to select the resolution ' +
                            '%(width)dx%(height)d');
                        window.__set_resolution_progress = "mode not found";
                        return;
                    }

                    chrome.system.display.setDisplayProperties('%(id)s',
                        {'displayMode': mode}, () => {
                            if (chrome.runtime.lastError) {
                                window.__set_resolution_progress = "failed: " +
                                    chrome.runtime.lastError.message;
                            } else {
                                window.__set_resolution_progress = "succeeded";
                            }
                        }
                    );
                });
                """
                % {'id': display_id, 'width': width, 'height': height}
        )
        utils.wait_for_value(lambda: (
                extension.EvaluateJavaScript(
                    'window.__set_resolution_progress') != None),
                expected_value=True)
        result = extension.EvaluateJavaScript(
                'window.__set_resolution_progress')
        if result != 'succeeded':
            raise RuntimeError('Failed to set resolution: %r' % result)


    @_retry_display_call
    def get_external_resolution(self):
        """Gets the resolution of the external screen.

        @return The resolution tuple (width, height)
        """
        return graphics_utils.get_external_resolution()

    def get_internal_resolution(self):
        """Gets the resolution of the internal screen.

        @return The resolution tuple (width, height) or None if internal screen
                is not available
        """
        for display in self.get_display_info():
            if display['isInternal']:
                bounds = display['bounds']
                return (bounds['width'], bounds['height'])
        return None


    def set_content_protection(self, state):
        """Sets the content protection of the external screen.

        @param state: One of the states 'Undesired', 'Desired', or 'Enabled'
        """
        connector = self.get_external_connector_name()
        graphics_utils.set_content_protection(connector, state)


    def get_content_protection(self):
        """Gets the state of the content protection.

        @param output: The output name as a string.
        @return: A string of the state, like 'Undesired', 'Desired', or 'Enabled'.
                 False if not supported.
        """
        connector = self.get_external_connector_name()
        return graphics_utils.get_content_protection(connector)


    def get_external_crtc(self):
        """Gets the external crtc.

        @return The id of the external crtc."""
        return graphics_utils.get_external_crtc()


    def get_internal_crtc(self):
        """Gets the internal crtc.

        @retrun The id of the internal crtc."""
        return graphics_utils.get_internal_crtc()


    def take_internal_screenshot(self, path):
        """Takes internal screenshot.

        @param path: path to image file.
        """
        self.take_screenshot_crtc(path, self.get_internal_crtc())


    def take_external_screenshot(self, path):
        """Takes external screenshot.

        @param path: path to image file.
        """
        self.take_screenshot_crtc(path, self.get_external_crtc())


    def take_screenshot_crtc(self, path, id):
        """Captures the DUT screenshot, use id for selecting screen.

        @param path: path to image file.
        @param id: The id of the crtc to screenshot.
        """

        graphics_utils.take_screenshot_crop(path, crtc_id=id)
        return True


    def save_calibration_image(self, path):
        """Save the calibration image to the given path.

        @param path: path to image file.
        """
        shutil.copy(self.CALIBRATION_IMAGE_PATH, path)
        return True


    def take_tab_screenshot(self, output_path, url_pattern=None):
        """Takes a screenshot of the tab specified by the given url pattern.

        @param output_path: A path of the output file.
        @param url_pattern: A string of url pattern used to search for tabs.
                            Default is to look for .svg image.
        """
        if url_pattern is None:
            # If no URL pattern is provided, defaults to capture the first
            # tab that shows SVG image.
            url_pattern = '.svg'

        tabs = self._resource.get_tabs()
        for i in xrange(0, len(tabs)):
            if url_pattern in tabs[i].url:
                data = tabs[i].Screenshot(timeout=5)
                # Flip the colors from BGR to RGB.
                data = numpy.fliplr(data.reshape(-1, 3)).reshape(data.shape)
                data.tofile(output_path)
                break
        return True


    def toggle_mirrored(self):
        """Toggles mirrored."""
        graphics_utils.screen_toggle_mirrored()
        return True


    def hide_cursor(self):
        """Hides mouse cursor."""
        graphics_utils.hide_cursor()
        return True


    def hide_typing_cursor(self):
        """Hides typing cursor."""
        graphics_utils.hide_typing_cursor()
        return True


    def is_mirrored_enabled(self):
        """Checks the mirrored state.

        @return True if mirrored mode is enabled.
        """
        return bool(self.get_display_info()[0]['mirroringSourceId'])


    def set_mirrored(self, is_mirrored):
        """Sets mirrored mode.

        @param is_mirrored: True or False to indicate mirrored state.
        @return True if success, False otherwise.
        """
        if self.is_mirrored_enabled() == is_mirrored:
            return True

        retries = 4
        while retries > 0:
            self.toggle_mirrored()
            result = utils.wait_for_value(self.is_mirrored_enabled,
                                          expected_value=is_mirrored,
                                          timeout_sec=3)
            if result == is_mirrored:
                return True
            retries -= 1
        return False


    def is_display_primary(self, internal=True):
        """Checks if internal screen is primary display.

        @param internal: is internal/external screen primary status requested
        @return boolean True if internal display is primary.
        """
        for info in self.get_display_info():
            if info['isInternal'] == internal and info['isPrimary']:
                return True
        return False


    def suspend_resume(self, suspend_time=10):
        """Suspends the DUT for a given time in second.

        @param suspend_time: Suspend time in second.
        """
        sys_power.do_suspend(suspend_time)
        return True


    def suspend_resume_bg(self, suspend_time=10):
        """Suspends the DUT for a given time in second in the background.

        @param suspend_time: Suspend time in second.
        """
        process = multiprocessing.Process(target=self.suspend_resume,
                                          args=(suspend_time,))
        process.start()
        return True


    @_retry_display_call
    def get_external_connector_name(self):
        """Gets the name of the external output connector.

        @return The external output connector name as a string, if any.
                Otherwise, return False.
        """
        return graphics_utils.get_external_connector_name()


    def get_internal_connector_name(self):
        """Gets the name of the internal output connector.

        @return The internal output connector name as a string, if any.
                Otherwise, return False.
        """
        return graphics_utils.get_internal_connector_name()


    def wait_external_display_connected(self, display):
        """Waits for the specified external display to be connected.

        @param display: The display name as a string, like 'HDMI1', or
                        False if no external display is expected.
        @return: True if display is connected; False otherwise.
        """
        result = utils.wait_for_value(self.get_external_connector_name,
                                      expected_value=display)
        return result == display


    @facade_resource.retry_chrome_call
    def move_to_display(self, display_id):
        """Moves the current window to the indicated display.

        @param display_id: The id of the indicated display.
        @return True if success.

        @raise TimeoutException if it fails.
        """
        display_info = self._get_display_by_id(display_id)
        if not display_info['isEnabled']:
            raise RuntimeError('Cannot find the indicated display')
        target_bounds = display_info['bounds']

        extension = self._resource.get_extension()
        # If the area of bounds is empty (here we achieve this by setting
        # width and height to zero), the window_sizer will automatically
        # determine an area which is visible and fits on the screen.
        # For more details, see chrome/browser/ui/window_sizer.cc
        # Without setting state to 'normal', if the current state is
        # 'minimized', 'maximized' or 'fullscreen', the setting of
        # 'left', 'top', 'width' and 'height' will be ignored.
        # For more details, see chrome/browser/extensions/api/tabs/tabs_api.cc
        extension.ExecuteJavaScript(
                """
                var __status = 'Running';
                chrome.windows.update(
                        chrome.windows.WINDOW_ID_CURRENT,
                        {left: %d, top: %d, width: 0, height: 0,
                         state: 'normal'},
                        function(info) {
                            if (info.left == %d && info.top == %d &&
                                info.state == 'normal')
                                __status = 'Done'; });
                """
                % (target_bounds['left'], target_bounds['top'],
                   target_bounds['left'], target_bounds['top'])
        )
        extension.WaitForJavaScriptCondition(
                "__status == 'Done'",
                timeout=web_contents.DEFAULT_WEB_CONTENTS_TIMEOUT)
        return True


    def is_fullscreen_enabled(self):
        """Checks the fullscreen state.

        @return True if fullscreen mode is enabled.
        """
        return self.get_window_info()['state'] == 'fullscreen'


    def set_fullscreen(self, is_fullscreen):
        """Sets the current window to full screen.

        @param is_fullscreen: True or False to indicate fullscreen state.
        @return True if success, False otherwise.
        """
        extension = self._resource.get_extension()
        if not extension:
            raise RuntimeError('Autotest extension not found')

        if is_fullscreen:
            window_state = "fullscreen"
        else:
            window_state = "normal"
        extension.ExecuteJavaScript(
                """
                var __status = 'Running';
                chrome.windows.update(
                        chrome.windows.WINDOW_ID_CURRENT,
                        {state: '%s'},
                        function() { __status = 'Done'; });
                """
                % window_state)
        utils.wait_for_value(lambda: (
                extension.EvaluateJavaScript('__status') == 'Done'),
                expected_value=True)
        return self.is_fullscreen_enabled() == is_fullscreen


    def load_url(self, url):
        """Loads the given url in a new tab. The new tab will be active.

        @param url: The url to load as a string.
        @return a str, the tab descriptor of the opened tab.
        """
        return self._resource.load_url(url)


    def load_calibration_image(self, resolution):
        """Opens a new tab and loads a full screen calibration
           image from the HTTP server.

        @param resolution: A tuple (width, height) of resolution.
        @return a str, the tab descriptor of the opened tab.
        """
        path = self.CALIBRATION_IMAGE_PATH
        self._image_generator.generate_image(resolution[0], resolution[1], path)
        os.chmod(path, 0644)
        tab_descriptor = self.load_url('file://%s' % path)
        return tab_descriptor


    def load_color_sequence(self, tab_descriptor, color_sequence):
        """Displays a series of colors on full screen on the tab.
        tab_descriptor is returned by any open tab API of display facade.
        e.g.,
        tab_descriptor = load_url('about:blank')
        load_color_sequence(tab_descriptor, color)

        @param tab_descriptor: Indicate which tab to test.
        @param color_sequence: An integer list for switching colors.
        @return A list of the timestamp for each switch.
        """
        tab = self._resource.get_tab_by_descriptor(tab_descriptor)
        color_sequence_for_java_script = (
                'var color_sequence = [' +
                ','.join("'#%06X'" % x for x in color_sequence) +
                '];')
        # Paints are synchronized to the fresh rate of the screen by
        # window.requestAnimationFrame.
        tab.ExecuteJavaScript(color_sequence_for_java_script + """
            function render(timestamp) {
                window.timestamp_list.push(timestamp);
                if (window.count < color_sequence.length) {
                    document.body.style.backgroundColor =
                            color_sequence[count];
                    window.count++;
                    window.requestAnimationFrame(render);
                }
            }
            window.count = 0;
            window.timestamp_list = [];
            window.requestAnimationFrame(render);
            """)

        # Waiting time is decided by following concerns:
        # 1. MINIMUM_REFRESH_RATE_EXPECTED: the minimum refresh rate
        #    we expect it to be. Real refresh rate is related to
        #    not only hardware devices but also drivers and browsers.
        #    Most graphics devices support at least 60fps for a single
        #    monitor, and under mirror mode, since the both frames
        #    buffers need to be updated for an input frame, the refresh
        #    rate will decrease by half, so here we set it to be a
        #    little less than 30 (= 60/2) to make it more tolerant.
        # 2. DELAY_TIME: extra wait time for timeout.
        tab.WaitForJavaScriptCondition(
                'window.count == color_sequence.length',
                timeout=(
                    (len(color_sequence) / self.MINIMUM_REFRESH_RATE_EXPECTED)
                    + self.DELAY_TIME))
        return tab.EvaluateJavaScript("window.timestamp_list")


    def close_tab(self, tab_descriptor):
        """Disables fullscreen and closes the tab of the given tab descriptor.
        tab_descriptor is returned by any open tab API of display facade.
        e.g.,
        1.
        tab_descriptor = load_url(url)
        close_tab(tab_descriptor)

        2.
        tab_descriptor = load_calibration_image(resolution)
        close_tab(tab_descriptor)

        @param tab_descriptor: Indicate which tab to be closed.
        """
        if tab_descriptor:
            # set_fullscreen(False) is necessary here because currently there
            # is a bug in tabs.Close(). If the current state is fullscreen and
            # we call close_tab() without setting state back to normal, it will
            # cancel fullscreen mode without changing system configuration, and
            # so that the next time someone calls set_fullscreen(True), the
            # function will find that current state is already 'fullscreen'
            # (though it is not) and do nothing, which will break all the
            # following tests.
            self.set_fullscreen(False)
            self._resource.close_tab(tab_descriptor)
        else:
            logging.error('close_tab: not a valid tab_descriptor')

        return True


    def reset_connector_if_applicable(self, connector_type):
        """Resets Type-C video connector from host end if applicable.

        It's the workaround sequence since sometimes Type-C dongle becomes
        corrupted and needs to be re-plugged.

        @param connector_type: A string, like "VGA", "DVI", "HDMI", or "DP".
        """
        if connector_type != 'HDMI' and connector_type != 'DP':
            return
        # Decide if we need to add --name=cros_pd
        usbpd_command = 'ectool --name=cros_pd usbpd'
        try:
            common_utils.run('%s 0' % usbpd_command)
        except error.CmdError:
            usbpd_command = 'ectool usbpd'

        port = 0
        while port < self.MAX_TYPEC_PORT:
            # We use usbpd to get Role information and then power cycle the
            # SRC one.
            command = '%s %d' % (usbpd_command, port)
            try:
                output = common_utils.run(command).stdout
                if re.compile('Role.*SRC').search(output):
                    logging.info('power-cycle Type-C port %d', port)
                    common_utils.run('%s sink' % command)
                    common_utils.run('%s auto' % command)
                port += 1
            except error.CmdError:
                break
