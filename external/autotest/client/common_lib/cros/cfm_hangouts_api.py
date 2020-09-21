# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error

DEFAULT_TIMEOUT = 30
DIAGNOSTIC_RUN_TIMEOUT = 180


class CfmHangoutsAPI(object):
    """Utility class for interacting with Hangouts in CFM."""

    def __init__(self, webview_context):
        self._webview_context = webview_context


    def wait_for_telemetry_commands(self):
        """Wait for hotrod app to load and telemetry commands to be available.
        """
        self._webview_context.WaitForJavaScriptCondition(
                "typeof window.hrOobIsStartPageForTest == 'function'",
                timeout=DEFAULT_TIMEOUT)
        logging.info('Hotrod telemetry commands available for testing.')


    def wait_for_meetings_in_call_page(self):
        """Waits for the in-call page to launch."""
        raise NotImplementedError


    def wait_for_meetings_landing_page(self):
        """Waits for the landing page screen."""
        raise NotImplementedError


    # UI commands/functions
    def wait_for_oobe_start_page(self):
        """Wait for oobe start screen to launch."""
        self._webview_context.WaitForJavaScriptCondition(
                "window.hasOwnProperty('hrOobIsStartPageForTest') "
                "&& window.hrOobIsStartPageForTest() === true;",
                timeout=DEFAULT_TIMEOUT)
        logging.info('Reached oobe start page')


    def skip_oobe_screen(self):
        """Skip Chromebox for Meetings oobe screen."""
        self._webview_context.ExecuteJavaScript("window.hrOobSkipForTest()")
        utils.poll_for_condition(
                lambda: not self._webview_context.EvaluateJavaScript(
                "window.hrOobIsStartPageForTest()"),
                exception=error.TestFail('Not able to skip oobe screen.'),
                timeout=DEFAULT_TIMEOUT,
                sleep_interval=1)
        logging.info('Skipped oobe screen.')


    def is_oobe_start_page(self):
        """Check if device is on CFM oobe start screen."""
        if self._webview_context.EvaluateJavaScript(
                "window.hrOobIsStartPageForTest()"):
            logging.info('Is on oobe start page.')
            return True
        logging.info('Is not on oobe start page.')
        return False


    # Hangouts commands/functions
    def start_new_hangout_session(self, hangout_name):
        """Start a new hangout session.

        @param hangout_name: Name of the hangout session.
        """
        if not self.is_ready_to_start_hangout_session():
            if self.is_in_hangout_session():
                self.end_hangout_session()
            utils.poll_for_condition(
                    lambda: self._webview_context.EvaluateJavaScript(
                    "window.hrIsReadyToStartHangoutForTest()"),
                    exception=error.TestFail(
                            'Not ready to start hangout session.'),
                    timeout=DEFAULT_TIMEOUT,
                    sleep_interval=1)

        self._webview_context.ExecuteJavaScript("window.hrStartCallForTest('" +
                                                hangout_name + "')")
        utils.poll_for_condition(
                lambda: self._webview_context.EvaluateJavaScript(
                "window.hrIsInHangoutForTest()"),
                exception=error.TestFail('Not able to start session.'),
                timeout=DEFAULT_TIMEOUT,
                sleep_interval=1)
        logging.info('Started hangout session: %s', hangout_name)


    def end_hangout_session(self):
        """End current hangout session."""
        self._webview_context.ExecuteJavaScript("window.hrHangupCallForTest()")
        utils.poll_for_condition(
                lambda: not self._webview_context.EvaluateJavaScript(
                "window.hrIsInHangoutForTest()"),
                exception=error.TestFail('Not able to end session.'),
                timeout=DEFAULT_TIMEOUT,
                sleep_interval=1)

        logging.info('Ended hangout session.')


    def is_in_hangout_session(self):
        """Check if device is in hangout session."""
        if self._webview_context.EvaluateJavaScript(
                "window.hrIsInHangoutForTest()"):
            logging.info('Is in hangout session.')
            return True
        logging.info('Is not in hangout session.')
        return False


    def is_ready_to_start_hangout_session(self):
        """Check if device is ready to start a new hangout session."""
        if (self._webview_context.EvaluateJavaScript(
                "window.hrIsReadyToStartHangoutForTest()")):
            logging.info('Is ready to start hangout session.')
            return True
        logging.info('Is not ready to start hangout session.')
        return False


    def join_meeting_session(self, meeting_name):
        """Joins a meeting.

        @param meeting_name: Name of the meeting session.
        """
        raise NotImplementedError


    def end_meeting_session(self):
        """End current meeting session."""
        raise NotImplementedError


    def is_in_meeting_session(self):
        """Check if device is in meeting session."""
        raise NotImplementedError


    def get_participant_count(self):
        """Returns the total number of participants in a hangout."""
        return self._webview_context.EvaluateJavaScript(
                "window.hrGetParticipantsCountInCallForTest()")


    # Diagnostics commands/functions
    def is_diagnostic_run_in_progress(self):
        """Check if hotrod diagnostics is running."""
        if (self._webview_context.EvaluateJavaScript(
                "window.hrIsDiagnosticRunInProgressForTest()")):
            logging.info('Diagnostic run is in progress.')
            return True
        logging.info('Diagnostic run is not in progress.')
        return False


    def wait_for_diagnostic_run_to_complete(self):
        """Wait for hotrod diagnostics to complete."""
        utils.poll_for_condition(
                lambda: not self._webview_context.EvaluateJavaScript(
                "window.hrIsDiagnosticRunInProgressForTest()"),
                exception=error.TestError('Diagnostic run still in progress '
                                          'after 3 minutes.'),
                timeout=DIAGNOSTIC_RUN_TIMEOUT,
                sleep_interval=1)


    def run_diagnostics(self):
        """Run hotrod diagnostics."""
        if self.is_diagnostic_run_in_progress():
            self.wait_for_diagnostic_run_to_complete()
        self._webview_context.ExecuteJavaScript(
                "window.hrRunDiagnosticsForTest()")
        logging.info('Started diagnostics run.')


    def get_last_diagnostics_results(self):
        """Get latest hotrod diagnostics results."""
        if self.is_diagnostic_run_in_progress():
            self.wait_for_diagnostic_run_to_complete()
        return self._webview_context.EvaluateJavaScript(
                "window.hrGetLastDiagnosticsResultForTest()")


    # Mic audio commands/functions
    def is_mic_muted(self):
        """Check if mic is muted."""
        if self._webview_context.EvaluateJavaScript(
                "window.hrGetAudioInMutedForTest()"):
            logging.info('Mic is muted.')
            return True
        logging.info('Mic is not muted.')
        return False


    def mute_mic(self):
        """Local mic mute from toolbar."""
        self._webview_context.ExecuteJavaScript(
                "window.hrSetAudioInMutedForTest(true)")
        logging.info('Locally muted mic.')


    def unmute_mic(self):
        """Local mic unmute from toolbar."""
        self._webview_context.ExecuteJavaScript(
                "window.hrSetAudioInMutedForTest(false)")
        logging.info('Locally unmuted mic.')


    def remote_mute_mic(self):
        """Remote mic mute request from cPanel."""
        self._webview_context.ExecuteJavaScript("window.hrMuteAudioForTest()")
        logging.info('Remotely muted mic.')


    def remote_unmute_mic(self):
        """Remote mic unmute request from cPanel."""
        self._webview_context.ExecuteJavaScript(
                "window.hrUnmuteAudioForTest()")
        logging.info('Remotely unmuted mic.')


    def get_mic_devices(self):
        """Get all mic devices detected by hotrod."""
        return self._webview_context.EvaluateJavaScript(
                "window.hrGetAudioInDevicesForTest()")


    def get_preferred_mic(self):
        """Get mic preferred for hotrod."""
        return self._webview_context.EvaluateJavaScript(
                "window.hrGetAudioInPrefForTest()")


    def set_preferred_mic(self, mic):
        """Set preferred mic for hotrod.

        @param mic: String with mic name.
        """
        self._webview_context.ExecuteJavaScript(
                "window.hrSetAudioInPrefForTest('" + mic + "')")
        logging.info('Setting preferred mic to %s.', mic)


    # Speaker commands/functions
    def get_speaker_devices(self):
        """Get all speaker devices detected by hotrod."""
        return self._webview_context.EvaluateJavaScript(
                "window.hrGetAudioOutDevicesForTest()")


    def get_preferred_speaker(self):
        """Get speaker preferred for hotrod."""
        return self._webview_context.EvaluateJavaScript(
                "window.hrGetAudioOutPrefForTest()")


    def set_preferred_speaker(self, speaker):
        """Set preferred speaker for hotrod.

        @param mic: String with speaker name.
        """
        self._webview_context.ExecuteJavaScript(
                "window.hrSetAudioOutPrefForTest('" + speaker + "')")
        logging.info('Set preferred speaker to %s.', speaker)


    def set_speaker_volume(self, vol_level):
        """Set speaker volume.

        @param vol_level: String value ranging from 0-100 to set volume to.
        """
        self._webview_context.ExecuteJavaScript(
                "window.hrSetAudioOutVolumeLevelForTest('" + vol_level + "')")
        logging.info('Set speaker volume to %s', vol_level)


    def get_speaker_volume(self):
        """Get current speaker volume."""
        return self._webview_context.EvaluateJavaScript(
                "window.hrGetAudioOutVolumeLevelForTest()")


    def play_test_sound(self):
        """Play test sound."""
        self._webview_context.ExecuteJavaScript(
                "window.hrPlayTestSoundForTest()")
        logging.info('Playing test sound.')


    # Camera commands/functions
    def get_camera_devices(self):
        """Get all camera devices detected by hotrod."""
        return self._webview_context.EvaluateJavaScript(
                "window.hrGetVideoCaptureDevicesForTest()")


    def get_preferred_camera(self):
        """Get camera preferred for hotrod."""
        return self._webview_context.EvaluateJavaScript(
                "window.hrGetVideoCapturePrefForTest()")


    def set_preferred_camera(self, camera):
        """Set preferred camera for hotrod.

        @param mic: String with camera name.
        """
        self._webview_context.ExecuteJavaScript(
                "window.hrSetVideoCapturePrefForTest('" + camera + "')")
        logging.info('Set preferred camera to %s.', camera)


    def is_camera_muted(self):
        """Check if camera is muted (turned off)."""
        if self._webview_context.EvaluateJavaScript(
                "window.hrGetVideoCaptureMutedForTest()"):
            logging.info('Camera is muted.')
            return True
        logging.info('Camera is not muted.')
        return False


    def mute_camera(self):
        """Turned camera off."""
        self._webview_context.ExecuteJavaScript(
                "window.hrSetVideoCaptureMutedForTest(true)")
        logging.info('Camera muted.')


    def unmute_camera(self):
        """Turned camera on."""
        self._webview_context.ExecuteJavaScript(
                "window.hrSetVideoCaptureMutedForTest(false)")
        logging.info('Camera unmuted.')
