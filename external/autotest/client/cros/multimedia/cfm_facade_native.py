# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Facade to access the CFM functionality."""

import glob
import logging
import os
import time
import urlparse

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import cfm_hangouts_api
from autotest_lib.client.common_lib.cros import cfm_meetings_api
from autotest_lib.client.common_lib.cros import enrollment
from autotest_lib.client.common_lib.cros import kiosk_utils
from autotest_lib.client.cros.graphics import graphics_utils


class TimeoutException(Exception):
    """Timeout Exception class."""
    pass


class CFMFacadeNative(object):
    """Facade to access the CFM functionality.

    The methods inside this class only accept Python native types.
    """
    _USER_ID = 'cr0s-cfm-la6-aut0t3st-us3r@croste.tv'
    _PWD = 'test0000'
    _EXT_ID = 'ikfcpmgefdpheiiomgmhlmmkihchmdlj'
    _ENROLLMENT_DELAY = 15
    _DEFAULT_TIMEOUT = 30

    # Log file locations
    _BASE_DIR = '/home/chronos/user/Storage/ext/'
    _CALLGROK_LOGS_PATTERN = _BASE_DIR + _EXT_ID + '/0*/File System/000/t/00/0*'
    _PA_LOGS_PATTERN = _BASE_DIR + _EXT_ID + '/def/File System/primary/p/00/0*'


    def __init__(self, resource, screen):
        """Initializes a CFMFacadeNative.

        @param resource: A FacadeResource object.
        """
        self._resource = resource
        self._screen = screen


    def enroll_device(self):
        """Enroll device into CFM."""
        self._resource.start_custom_chrome({"auto_login": False,
                                            "disable_gaia_services": False})
        enrollment.RemoraEnrollment(self._resource._browser, self._USER_ID,
                self._PWD)
        # Timeout to allow for the device to stablize and go back to the
        # login screen before proceeding.
        time.sleep(self._ENROLLMENT_DELAY)


    def restart_chrome_for_cfm(self):
        """Restart chrome with custom values for CFM."""
        custom_chrome_setup = {"clear_enterprise_policy": False,
                               "dont_override_profile": True,
                               "disable_gaia_services": False,
                               "disable_default_apps": False,
                               "auto_login": False}
        self._resource.start_custom_chrome(custom_chrome_setup)


    def check_hangout_extension_context(self):
        """Check to make sure hangout app launched.

        @raises error.TestFail if the URL checks fails.
        """
        ext_contexts = kiosk_utils.wait_for_kiosk_ext(
                self._resource._browser, self._EXT_ID)
        ext_urls = [context.EvaluateJavaScript('location.href;')
                        for context in ext_contexts]
        expected_urls = ['chrome-extension://' + self._EXT_ID + '/' + path
                         for path in ['hangoutswindow.html?windowid=0',
                                      'hangoutswindow.html?windowid=1',
                                      'hangoutswindow.html?windowid=2',
                                      '_generated_background_page.html']]
        for url in ext_urls:
            logging.info('Extension URL %s', url)
            if url not in expected_urls:
                raise error.TestFail(
                    'Unexpected extension context urls, expected one of %s, '
                    'got %s' % (expected_urls, url))


    def take_screenshot(self, screenshot_name):
        """
        Takes a screenshot of what is currently displayed in png format.

        The screenshot is stored in /tmp. Uses the low level graphics_utils API.

        @param screenshot_name: Name of the screenshot file.
        @returns The path to the screenshot or None.
        """
        try:
            return graphics_utils.take_screenshot('/tmp', screenshot_name)
        except Exception as e:
            logging.warning('Taking screenshot failed', exc_info = e)
            return None


    def get_latest_callgrok_file_path(self):
        """
        @return The path to the lastest callgrok log file, if any.
        """
        try:
            return max(glob.iglob(self._CALLGROK_LOGS_PATTERN),
                       key=os.path.getctime)
        except ValueError as e:
            logging.exception('Error while searching for callgrok logs.')
            return None


    def get_latest_pa_logs_file_path(self):
        """
        @return The path to the lastest packaged app log file, if any.
        """
        try:
            return max(glob.iglob(self._PA_LOGS_PATTERN), key=os.path.getctime)
        except ValueError as e:
            logging.exception('Error while searching for packaged app logs.')
            return None


    def reboot_device_with_chrome_api(self):
        """Reboot device using chrome runtime API."""
        ext_contexts = kiosk_utils.wait_for_kiosk_ext(
                self._resource._browser, self._EXT_ID)
        for context in ext_contexts:
            context.WaitForDocumentReadyStateToBeInteractiveOrBetter()
            ext_url = context.EvaluateJavaScript('document.URL')
            background_url = ('chrome-extension://' + self._EXT_ID +
                              '/_generated_background_page.html')
            if ext_url in background_url:
                context.ExecuteJavaScript('chrome.runtime.restart();')


    def _get_webview_context_by_screen(self, screen):
        """Get webview context that matches the screen param in the url.

        @param screen: Value of the screen param, e.g. 'hotrod' or 'control'.
        """
        def _get_context():
            try:
                ctxs = kiosk_utils.get_webview_contexts(self._resource._browser,
                                                        self._EXT_ID)
                for ctx in ctxs:
                    url_query = urlparse.urlparse(ctx.GetUrl()).query
                    logging.info('Webview query: "%s"', url_query)
                    params = urlparse.parse_qs(url_query,
                                               keep_blank_values = True)
                    is_oobe_slave_screen = ('nooobestatesync' in params and
                                            'oobedone' in params)
                    if is_oobe_slave_screen:
                        # Skip the oobe slave screen. Not doing this can cause
                        # the wrong webview context to be returned.
                        continue
                    if 'screen' in params and params['screen'][0] == screen:
                        return ctx
            except Exception as e:
                # Having a MIMO attached to the DUT causes a couple of webview
                # destruction/construction operations during OOBE. If we query a
                # destructed webview it will throw an exception. Instead of
                # failing the test, we just swallow the exception.
                logging.exception(
                    "Exception occured while querying the webview contexts.")
            return None

        return utils.poll_for_condition(
                    _get_context,
                    exception=error.TestFail(
                        'Webview with screen param "%s" not found.' % screen),
                    timeout=self._DEFAULT_TIMEOUT,
                    sleep_interval = 1)


    def skip_oobe_after_enrollment(self):
        """Skips oobe and goes to the app landing page after enrollment."""
        self.restart_chrome_for_cfm()
        self.check_hangout_extension_context()
        self.wait_for_hangouts_telemetry_commands()
        self.wait_for_oobe_start_page()
        self.skip_oobe_screen()


    @property
    def _webview_context(self):
        """Get webview context object."""
        return self._get_webview_context_by_screen(self._screen)


    @property
    def _cfmApi(self):
        """Instantiate appropriate cfm api wrapper"""
        if self._webview_context.EvaluateJavaScript(
                "typeof window.hrRunDiagnosticsForTest == 'function'"):
            return cfm_hangouts_api.CfmHangoutsAPI(self._webview_context)
        if self._webview_context.EvaluateJavaScript(
                "typeof window.hrTelemetryApi != 'undefined'"):
            return cfm_meetings_api.CfmMeetingsAPI(self._webview_context)
        raise error.TestFail('No hangouts or meet telemetry API available. '
                             'Current url is "%s"' %
                             self._webview_context.GetUrl())


    #TODO: This is a legacy api. Deprecate this api and update existing hotrod
    #      tests to use the new wait_for_hangouts_telemetry_commands api.
    def wait_for_telemetry_commands(self):
        """Wait for telemetry commands."""
        self.wait_for_hangouts_telemetry_commands()


    def wait_for_hangouts_telemetry_commands(self):
        """Wait for Hangouts App telemetry commands."""
        self._webview_context.WaitForJavaScriptCondition(
                "typeof window.hrOobIsStartPageForTest == 'function'",
                timeout=self._DEFAULT_TIMEOUT)


    def wait_for_meetings_telemetry_commands(self):
        """Wait for Meet App telemetry commands """
        self._webview_context.WaitForJavaScriptCondition(
                'window.hasOwnProperty("hrTelemetryApi")',
                timeout=self._DEFAULT_TIMEOUT)


    def wait_for_meetings_in_call_page(self):
        """Waits for the in-call page to launch."""
        self.wait_for_meetings_telemetry_commands()
        self._cfmApi.wait_for_meetings_in_call_page()


    def wait_for_meetings_landing_page(self):
        """Waits for the landing page screen."""
        self.wait_for_meetings_telemetry_commands()
        self._cfmApi.wait_for_meetings_landing_page()


    # UI commands/functions
    def wait_for_oobe_start_page(self):
        """Wait for oobe start screen to launch."""
        self._cfmApi.wait_for_oobe_start_page()


    def skip_oobe_screen(self):
        """Skip Chromebox for Meetings oobe screen."""
        self._cfmApi.skip_oobe_screen()


    def is_oobe_start_page(self):
        """Check if device is on CFM oobe start screen.

        @return a boolean, based on oobe start page status.
        """
        return self._cfmApi.is_oobe_start_page()


    # Hangouts commands/functions
    def start_new_hangout_session(self, session_name):
        """Start a new hangout session.

        @param session_name: Name of the hangout session.
        """
        self._cfmApi.start_new_hangout_session(session_name)


    def end_hangout_session(self):
        """End current hangout session."""
        self._cfmApi.end_hangout_session()


    def is_in_hangout_session(self):
        """Check if device is in hangout session.

        @return a boolean, for hangout session state.
        """
        return self._cfmApi.is_in_hangout_session()


    def is_ready_to_start_hangout_session(self):
        """Check if device is ready to start a new hangout session.

        @return a boolean for hangout session ready state.
        """
        return self._cfmApi.is_ready_to_start_hangout_session()


    def join_meeting_session(self, session_name):
        """Joins a meeting.

        @param session_name: Name of the meeting session.
        """
        self._cfmApi.join_meeting_session(session_name)


    def start_meeting_session(self):
        """Start a meeting."""
        self._cfmApi.start_meeting_session()


    def end_meeting_session(self):
        """End current meeting session."""
        self._cfmApi.end_meeting_session()


    def get_participant_count(self):
        """Gets the total participant count in a call."""
        return self._cfmApi.get_participant_count()


    # Diagnostics commands/functions
    def is_diagnostic_run_in_progress(self):
        """Check if hotrod diagnostics is running.

        @return a boolean for diagnostic run state.
        """
        return self._cfmApi.is_diagnostic_run_in_progress()


    def wait_for_diagnostic_run_to_complete(self):
        """Wait for hotrod diagnostics to complete."""
        self._cfmApi.wait_for_diagnostic_run_to_complete()


    def run_diagnostics(self):
        """Run hotrod diagnostics."""
        self._cfmApi.run_diagnostics()


    def get_last_diagnostics_results(self):
        """Get latest hotrod diagnostics results.

        @return a dict with diagnostic test results.
        """
        return self._cfmApi.get_last_diagnostics_results()


    # Mic audio commands/functions
    def is_mic_muted(self):
        """Check if mic is muted.

        @return a boolean for mic mute state.
        """
        return self._cfmApi.is_mic_muted()


    def mute_mic(self):
        """Local mic mute from toolbar."""
        self._cfmApi.mute_mic()


    def unmute_mic(self):
        """Local mic unmute from toolbar."""
        self._cfmApi.unmute_mic()


    def remote_mute_mic(self):
        """Remote mic mute request from cPanel."""
        self._cfmApi.remote_mute_mic()


    def remote_unmute_mic(self):
        """Remote mic unmute request from cPanel."""
        self._cfmApi.remote_unmute_mic()


    def get_mic_devices(self):
        """Get all mic devices detected by hotrod.

        @return a list of mic devices.
        """
        return self._cfmApi.get_mic_devices()


    def get_preferred_mic(self):
        """Get mic preferred for hotrod.

        @return a str with preferred mic name.
        """
        return self._cfmApi.get_preferred_mic()


    def set_preferred_mic(self, mic):
        """Set preferred mic for hotrod.

        @param mic: String with mic name.
        """
        self._cfmApi.set_preferred_mic(mic)


    # Speaker commands/functions
    def get_speaker_devices(self):
        """Get all speaker devices detected by hotrod.

        @return a list of speaker devices.
        """
        return self._cfmApi.get_speaker_devices()


    def get_preferred_speaker(self):
        """Get speaker preferred for hotrod.

        @return a str with preferred speaker name.
        """
        return self._cfmApi.get_preferred_speaker()


    def set_preferred_speaker(self, speaker):
        """Set preferred speaker for hotrod.

        @param speaker: String with speaker name.
        """
        self._cfmApi.set_preferred_speaker(speaker)


    def set_speaker_volume(self, volume_level):
        """Set speaker volume.

        @param volume_level: String value ranging from 0-100 to set volume to.
        """
        self._cfmApi.set_speaker_volume(volume_level)


    def get_speaker_volume(self):
        """Get current speaker volume.

        @return a str value with speaker volume level 0-100.
        """
        return self._cfmApi.get_speaker_volume()


    def play_test_sound(self):
        """Play test sound."""
        self._cfmApi.play_test_sound()


    # Camera commands/functions
    def get_camera_devices(self):
        """Get all camera devices detected by hotrod.

        @return a list of camera devices.
        """
        return self._cfmApi.get_camera_devices()


    def get_preferred_camera(self):
        """Get camera preferred for hotrod.

        @return a str with preferred camera name.
        """
        return self._cfmApi.get_preferred_camera()


    def set_preferred_camera(self, camera):
        """Set preferred camera for hotrod.

        @param camera: String with camera name.
        """
        self._cfmApi.set_preferred_camera(camera)


    def is_camera_muted(self):
        """Check if camera is muted (turned off).

        @return a boolean for camera muted state.
        """
        return self._cfmApi.is_camera_muted()


    def mute_camera(self):
        """Turned camera off."""
        self._cfmApi.mute_camera()


    def unmute_camera(self):
        """Turned camera on."""
        self._cfmApi.unmute_camera()

    def move_camera(self, camera_motion):
        """Move camera(PTZ commands).

        @param camera_motion: Set of allowed commands
            defined in cfmApi.move_camera.
        """
        self._cfmApi.move_camera(camera_motion)

    def get_media_info_data_points(self):
        """
        Gets media info data points containing media stats.

        These are exported on the window object when the
        ExportMediaInfo mod is enabled.

        @returns A list with dictionaries of media info data points.
        @raises RuntimeError if the data point API is not available.
        """
        is_api_available_script = (
                '"realtime" in window '
                '&& "media" in realtime '
                '&& "getMediaInfoDataPoints" in realtime.media')
        if not self._webview_context.EvaluateJavaScript(
                is_api_available_script):
            raise RuntimeError(
                    'realtime.media.getMediaInfoDataPoints not available. '
                    'Is the ExportMediaInfo mod active? '
                    'The mod is only available for Meet.')

        data_points = self._webview_context.EvaluateJavaScript(
                'window.realtime.media.getMediaInfoDataPoints()')
        for data_point in data_points:
            # XML RCP gives overflow errors when trying to send too large
            # integers or longs. Convert timestamps to float seconds and media
            # stats to floats. We do not care if we lose some precision.
            # When we are at it, convert the timestamp to seconds as
            # expected in Python.
            data_point['timestamp'] = data_point['timestamp'] / 1000.0
            for media in data_point['media']:
                for k, v in media.iteritems():
                    if type(v) == int:
                        media[k] = float(v)
        return data_points

