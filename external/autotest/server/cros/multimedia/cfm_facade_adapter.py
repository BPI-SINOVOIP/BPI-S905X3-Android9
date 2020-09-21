# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""An adapter to remotely access the CFM facade on DUT."""

import os
import tempfile
import time


class CFMFacadeRemoteAdapter(object):
    """CFMFacadeRemoteAdapter is an adapter to remotely control CFM on DUT.

    The Autotest host object representing the remote DUT, passed to this
    class on initialization, can be accessed from its _client property.
    """
    _RESTART_UI_DELAY = 10

    def __init__(self, host, remote_facade_proxy):
        """Construct a CFMFacadeRemoteAdapter.

        @param host: Host object representing a remote host.
        @param remote_facade_proxy: RemoteFacadeProxy object.
        """
        self._client = host
        self._proxy = remote_facade_proxy


    @property
    def _cfm_proxy(self):
        return self._proxy.cfm_main_screen


    @property
    def main_screen(self):
        """CFM main screen API."""
        return self._proxy.cfm_main_screen


    @property
    def mimo_screen(self):
        """CFM mimo screen API."""
        return self._proxy.cfm_mimo_screen


    def enroll_device(self):
        """Enroll device into CFM."""
        self._cfm_proxy.enroll_device()


    def restart_chrome_for_cfm(self):
        """Restart chrome for CFM."""
        self._cfm_proxy.restart_chrome_for_cfm()

    def reboot_device_with_chrome_api(self):
        """Reboot device using Chrome runtime API."""
        self._cfm_proxy.reboot_device_with_chrome_api()

    def skip_oobe_after_enrollment(self):
        """Skips oobe and goes to the app landing page after enrollment."""
        self._client.run('restart ui', ignore_status=True)
        time.sleep(self._RESTART_UI_DELAY)
        self._cfm_proxy.skip_oobe_after_enrollment()


    def wait_for_telemetry_commands(self):
        """Wait for telemetry commands."""
        self._cfm_proxy.wait_for_telemetry_commands()


    def wait_for_hangouts_telemetry_commands(self):
        """Wait for Hangouts App telemetry commands."""
        self._cfm_proxy.wait_for_hangouts_telemetry_commands()


    def wait_for_meetings_telemetry_commands(self):
        """Waits for Meet App telemetry commands."""
        self._cfm_proxy.wait_for_meetings_telemetry_commands()


    def wait_for_meetings_in_call_page(self):
        """Waits for the in-call page to launch."""
        self._cfm_proxy.wait_for_meetings_in_call_page()


    def wait_for_meetings_landing_page(self):
        """Waits for the landing page screen."""
        self._cfm_proxy.wait_for_meetings_landing_page()


    # UI commands/functions
    def wait_for_oobe_start_page(self):
        """Wait for oobe start screen to launch."""
        self._cfm_proxy.wait_for_oobe_start_page()


    def skip_oobe_screen(self):
        """Skip Chromebox for Meetings oobe screen."""
        self._cfm_proxy.skip_oobe_screen()


    def is_oobe_start_page(self):
        """Check if device is on CFM oobe start screen.

        @return a boolean, based on oobe start page status.
        """
        return self._cfm_proxy.is_oobe_start_page()


    # Hangouts commands/functions
    def start_new_hangout_session(self, session_name):
        """Start a new hangout session.

        @param session_name: Name of the hangout session.
        """
        self._cfm_proxy.start_new_hangout_session(session_name)


    def end_hangout_session(self):
        """End current hangout session."""
        self._cfm_proxy.end_hangout_session()


    def take_screenshot(self):
        """
        Takes a screenshot on the DUT.

        @return The file path to the screenshot on the DUT or None.
        """
        # No suffix since cfm_proxy.take_screenshot() automactially appends one.
        with tempfile.NamedTemporaryFile() as f:
            basename = os.path.basename(f.name)
            return self._cfm_proxy.take_screenshot(basename)

    def get_latest_callgrok_file_path(self):
        """
        @return The path to the lastest callgrok log file, if any.
        """
        return self._cfm_proxy.get_latest_callgrok_file_path()


    def get_latest_pa_logs_file_path(self):
        """
        @return The path to the lastest packaged app log file, if any.
        """
        return self._cfm_proxy.get_latest_pa_logs_file_path()


    def is_in_hangout_session(self):
        """Check if device is in hangout session.

        @return a boolean, for hangout session state.
        """
        return self._cfm_proxy.is_in_hangout_session()


    def is_ready_to_start_hangout_session(self):
        """Check if device is ready to start a new hangout session.

        @return a boolean for hangout session ready state.
        """
        return self._cfm_proxy.is_ready_to_start_hangout_session()


    def join_meeting_session(self, session_name):
        """Join a meeting.

        @param session_name: Name of the meeting session.
        """
        self._cfm_proxy.join_meeting_session(session_name)


    def start_meeting_session(self):
        """Start a meeting."""
        self._cfm_proxy.start_meeting_session()


    def end_meeting_session(self):
        """End current meeting session."""
        self._cfm_proxy.end_meeting_session()


    def get_participant_count(self):
        """Gets the total participant count in a call."""
        return self._cfm_proxy.get_participant_count()


    # Diagnostics commands/functions
    def is_diagnostic_run_in_progress(self):
        """Check if hotrod diagnostics is running.

        @return a boolean for diagnostic run state.
        """
        return self._cfm_proxy.is_diagnostic_run_in_progress()


    def wait_for_diagnostic_run_to_complete(self):
        """Wait for hotrod diagnostics to complete."""
        self._cfm_proxy.wait_for_diagnostic_run_to_complete()


    def run_diagnostics(self):
        """Run hotrod diagnostics."""
        self._cfm_proxy.run_diagnostics()


    def get_last_diagnostics_results(self):
        """Get latest hotrod diagnostics results.

        @return a dict with diagnostic test results.
        """
        return self._cfm_proxy.get_last_diagnostics_results()


    # Mic audio commands/functions
    def is_mic_muted(self):
        """Check if mic is muted.

        @return a boolean for mic mute state.
        """
        return self._cfm_proxy.is_mic_muted()


    def mute_mic(self):
        """Local mic mute from toolbar."""
        self._cfm_proxy.mute_mic()


    def unmute_mic(self):
        """Local mic unmute from toolbar."""
        self._cfm_proxy.unmute_mic()


    def remote_mute_mic(self):
        """Remote mic mute request from cPanel."""
        self._cfm_proxy.remote_mute_mic()


    def remote_unmute_mic(self):
        """Remote mic unmute request from cPanel."""
        self._cfm_proxy.remote_unmute_mic()


    def get_mic_devices(self):
        """Get all mic devices detected by hotrod."""
        return self._cfm_proxy.get_mic_devices()


    def get_preferred_mic(self):
        """Get mic preferred for hotrod.

        @return a str with preferred mic name.
        """
        return self._cfm_proxy.get_preferred_mic()


    def set_preferred_mic(self, mic):
        """Set preferred mic for hotrod.

        @param mic: String with mic name.
        """
        self._cfm_proxy.set_preferred_mic(mic)


    # Speaker commands/functions
    def get_speaker_devices(self):
        """Get all speaker devices detected by hotrod.

        @return a list of speaker devices.
        """
        return self._cfm_proxy.get_speaker_devices()


    def get_preferred_speaker(self):
        """Get speaker preferred for hotrod.

        @return a str with preferred speaker name.
        """
        return self._cfm_proxy.get_preferred_speaker()


    def set_preferred_speaker(self, speaker):
        """Set preferred speaker for hotrod.

        @param speaker: String with speaker name.
        """
        self._cfm_proxy.set_preferred_speaker(speaker)


    def set_speaker_volume(self, volume_level):
        """Set speaker volume.

        @param volume_level: String value ranging from 0-100 to set volume to.
        """
        self._cfm_proxy.set_speaker_volume(volume_level)


    def get_speaker_volume(self):
        """Get current speaker volume.

        @return a str value with speaker volume level 0-100.
        """
        return self._cfm_proxy.get_speaker_volume()


    def play_test_sound(self):
        """Play test sound."""
        self._cfm_proxy.play_test_sound()


    # Camera commands/functions
    def get_camera_devices(self):
        """Get all camera devices detected by hotrod.

        @return a list of camera devices.
        """
        return self._cfm_proxy.get_camera_devices()


    def get_preferred_camera(self):
        """Get camera preferred for hotrod.

        @return a str with preferred camera name.
        """
        return self._cfm_proxy.get_preferred_camera()


    def set_preferred_camera(self, camera):
        """Set preferred camera for hotrod.

        @param camera: String with camera name.
        """
        self._cfm_proxy.set_preferred_camera(camera)


    def is_camera_muted(self):
        """Check if camera is muted (turned off).

        @return a boolean for camera muted state.
        """
        return self._cfm_proxy.is_camera_muted()


    def mute_camera(self):
        """Turned camera off."""
        self._cfm_proxy.mute_camera()


    def unmute_camera(self):
        """Turned camera on."""
        self._cfm_proxy.unmute_camera()


    def move_camera(self, camera_motion):
        """
        Move camera(PTZ commands).

        @param camera_motion: Set of allowed commands
            defined in cfmApi.move_camera.
        """
        self._cfm_proxy.move_camera(camera_motion)

    def get_media_info_data_points(self):
        """
        Gets media info data points containing media stats.

        These are exported on the window object when the
        ExportMediaInfo mod is enabled.

        @returns A list with dictionaries of media info data points.
        @raises RuntimeError if the data point API is not available.
        """
        return self._cfm_proxy.get_media_info_data_points()

