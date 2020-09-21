# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

DEFAULT_TIMEOUT = 30
TELEMETRY_API = 'hrTelemetryApi'


class CfmMeetingsAPI(object):
    """Utility class for interacting with CfMs."""

    def __init__(self, webview_context):
        self._webview_context = webview_context

    def _execute_telemetry_command(self, command):
        self._webview_context.ExecuteJavaScript(
            'window.%s.%s' % (TELEMETRY_API, command))

    def _evaluate_telemetry_command(self, command):
        return self._webview_context.EvaluateJavaScript(
            'window.%s.%s' % (TELEMETRY_API, command))

    # UI commands/functions
    def wait_for_meetings_landing_page(self):
        """Waits for the landing page screen."""
        self._webview_context.WaitForJavaScriptCondition(
            'window.hasOwnProperty("%s") '
            '&& !window.%s.isInMeeting()' % (TELEMETRY_API, TELEMETRY_API),
            timeout=DEFAULT_TIMEOUT)
        logging.info('Reached meetings landing page.')

    def wait_for_meetings_in_call_page(self):
        """Waits for the in-call page to launch."""
        self._webview_context.WaitForJavaScriptCondition(
            'window.hasOwnProperty("%s") '
            '&& window.%s.isInMeeting()' % (TELEMETRY_API, TELEMETRY_API),
            timeout=DEFAULT_TIMEOUT)
        logging.info('Reached meetings in-call page.')

    def wait_for_telemetry_commands(self):
        """Wait for hotrod app to load and telemetry commands to be available.
        """
        raise NotImplementedError

    def wait_for_oobe_start_page(self):
        """Wait for oobe start screen to launch."""
        raise NotImplementedError

    def skip_oobe_screen(self):
        """Skip Chromebox for Meetings oobe screen."""
        raise NotImplementedError

    def is_oobe_start_page(self):
        """Check if device is on CFM oobe start screen."""
        raise NotImplementedError

    # Hangouts commands/functions
    def start_meeting_session(self):
        """Start a meeting."""
        if self.is_in_meeting_session():
            self.end_meeting_session()

        self._execute_telemetry_command('startMeeting()')
        self.wait_for_meetings_in_call_page()
        logging.info('Started meeting session.')

    def join_meeting_session(self, meeting_name):
        """Joins a meeting.

        @param meeting_name: Name of the meeting session.
        """
        if self.is_in_meeting_session():
            self.end_meeting_session()

        self._execute_telemetry_command('joinMeeting("%s")' % meeting_name)
        self.wait_for_meetings_in_call_page()
        logging.info('Started meeting session: %s', meeting_name)

    def end_meeting_session(self):
        """End current meeting session."""
        self._execute_telemetry_command('endCall()')
        self.wait_for_meetings_landing_page()
        logging.info('Ended meeting session.')

    def is_in_meeting_session(self):
        """Check if device is in meeting session."""
        if self._evaluate_telemetry_command('isInMeeting()'):
            logging.info('Is in meeting session.')
            return True
        logging.info('Is not in meeting session.')
        return False

    def start_new_hangout_session(self, hangout_name):
        """Start a new hangout session.

        @param hangout_name: Name of the hangout session.
        """
        raise NotImplementedError

    def end_hangout_session(self):
        """End current hangout session."""
        raise NotImplementedError

    def is_in_hangout_session(self):
        """Check if device is in hangout session."""
        raise NotImplementedError

    def is_ready_to_start_hangout_session(self):
        """Check if device is ready to start a new hangout session."""
        raise NotImplementedError

    def get_participant_count(self):
        """Returns the total number of participants in a meeting."""
        return self._evaluate_telemetry_command('getParticipantCount()')

    # Diagnostics commands/functions
    def is_diagnostic_run_in_progress(self):
        """Check if hotrod diagnostics is running."""
        raise NotImplementedError

    def wait_for_diagnostic_run_to_complete(self):
        """Wait for hotrod diagnostics to complete."""
        raise NotImplementedError

    def run_diagnostics(self):
        """Run hotrod diagnostics."""
        raise NotImplementedError

    def get_last_diagnostics_results(self):
        """Get latest hotrod diagnostics results."""
        raise NotImplementedError

    # Mic audio commands/functions
    def is_mic_muted(self):
        """Check if mic is muted."""
        if self._evaluate_telemetry_command('isMicMuted()'):
            logging.info('Mic is muted.')
            return True
        logging.info('Mic is not muted.')
        return False

    def mute_mic(self):
        """Local mic mute from toolbar."""
        self._execute_telemetry_command('setMicMuted(true)')
        logging.info('Locally muted mic.')

    def unmute_mic(self):
        """Local mic unmute from toolbar."""
        self._execute_telemetry_command('setMicMuted(false)')
        logging.info('Locally unmuted mic.')

    def get_mic_devices(self):
        """Get all mic devices detected by hotrod."""
        return self._evaluate_telemetry_command('getAudioInDevices()')

    def get_preferred_mic(self):
        """Get preferred microphone for hotrod."""
        return self._evaluate_telemetry_command('getPreferredAudioInDevice()')

    def set_preferred_mic(self, mic_name):
        """Set preferred mic for hotrod.

        @param mic_name: String with mic name.
        """
        self._execute_telemetry_command('setPreferredAudioInDevice(%s)'
                                        % mic_name)
        logging.info('Setting preferred mic to %s.', mic_name)

    def remote_mute_mic(self):
        """Remote mic mute request from cPanel."""
        raise NotImplementedError

    def remote_unmute_mic(self):
        """Remote mic unmute request from cPanel."""
        raise NotImplementedError

    # Speaker commands/functions
    def get_speaker_devices(self):
        """Get all speaker devices detected by hotrod."""
        return self._evaluate_telemetry_command('getAudioOutDevices()')

    def get_preferred_speaker(self):
        """Get speaker preferred for hotrod."""
        return self._evaluate_telemetry_command('getPreferredAudioOutDevice()')

    def set_preferred_speaker(self, speaker_name):
        """Set preferred speaker for hotrod.

        @param speaker_name: String with speaker name.
        """
        self._execute_telemetry_command('setPreferredAudioOutDevice(%s)'
                                        % speaker_name)
        logging.info('Set preferred speaker to %s.', speaker_name)

    def set_speaker_volume(self, volume_level):
        """Set speaker volume.

        @param volume_level: Number value ranging from 0-100 to set volume to.
        """
        self._execute_telemetry_command('setAudioOutVolume(%d)' % volume_level)
        logging.info('Set speaker volume to %d', volume_level)

    def get_speaker_volume(self):
        """Get current speaker volume."""
        return self._evaluate_telemetry_command('getAudioOutVolume()')

    def play_test_sound(self):
        """Play test sound."""
        raise NotImplementedError

    # Camera commands/functions
    def get_camera_devices(self):
        """Get all camera devices detected by hotrod.

        @return List of camera devices.
        """
        return self._evaluate_telemetry_command('getVideoInDevices()')

    def get_preferred_camera(self):
        """Get camera preferred for hotrod."""
        return self._evaluate_telemetry_command('getPreferredVideoInDevice()')

    def set_preferred_camera(self, camera_name):
        """Set preferred camera for hotrod.

        @param camera_name: String with camera name.
        """
        self._execute_telemetry_command('setPreferredVideoInDevice(%s)'
                                        % camera_name)
        logging.info('Set preferred camera to %s.', camera_name)

    def is_camera_muted(self):
        """Check if camera is muted (turned off)."""
        if self._evaluate_telemetry_command('isCameraMuted()'):
            logging.info('Camera is muted.')
            return True
        logging.info('Camera is not muted.')
        return False

    def mute_camera(self):
        """Mute (turn off) camera."""
        self._execute_telemetry_command('setCameraMuted(true)')
        logging.info('Camera muted.')

    def unmute_camera(self):
        """Unmute (turn on) camera."""
        self._execute_telemetry_command('setCameraMuted(false)')
        logging.info('Camera unmuted.')

    def move_camera(self, camera_motion):
        """Move camera(PTZ functions).

        @param camera_motion: String of the desired camera motion.
        """
        ptz_motions = ['panLeft','panRight','panStop',
                       'tiltUp','tiltDown','tiltStop',
                       'zoomIn','zoomOut','resetPosition']

        if camera_motion in ptz_motions:
            self._execute_telemetry_command('ptz.%s()' % camera_motion)
        else:
            raise ValueError('Unsupported PTZ camera action: "%s"'
                             % camera_motion)
