# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Resource manager to access the ARC-related functionality."""

import logging
import os
import pipes
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import arc
from autotest_lib.client.cros.multimedia import arc_resource_common
from autotest_lib.client.cros.input_playback import input_playback


def set_tag(tag):
    """Sets a tag file.

    @param tag: Path to the tag file.

    """
    open(tag, 'w').close()


def tag_exists(tag):
    """Checks if a tag exists.

    @param tag: Path to the tag file.

    """
    return os.path.exists(tag)


class ArcMicrophoneResourceException(Exception):
    """Exceptions in ArcResource."""
    pass


class ArcMicrophoneResource(object):
    """Class to manage microphone app in container."""
    _MICROPHONE_ACTIVITY = 'org.chromium.arc.testapp.microphone/.MainActivity'
    _MICROPHONE_PACKAGE = 'org.chromium.arc.testapp.microphone'
    _MICROPHONE_RECORD_PATH = '/storage/emulated/0/recorded.amr-nb'
    _MICROPHONE_PERMISSIONS = ['RECORD_AUDIO', 'WRITE_EXTERNAL_STORAGE',
                               'READ_EXTERNAL_STORAGE']

    def __init__(self):
        """Initializes a ArcMicrophoneResource."""
        self._mic_app_start_time = None


    def start_microphone_app(self):
        """Starts microphone app to start recording.

        Starts microphone app. The app starts recorder itself after start up.

        @raises: ArcMicrophoneResourceException if microphone app is not ready
                 yet.

        """
        if not tag_exists(arc_resource_common.MicrophoneProps.READY_TAG_FILE):
            raise ArcMicrophoneResourceException(
                    'Microphone app is not ready yet.')

        if self._mic_app_start_time:
            raise ArcMicrophoneResourceException(
                    'Microphone app is already started.')

        # In case the permissions are cleared, set the permission again before
        # each start of the app.
        self._set_permission()
        self._start_app()
        self._mic_app_start_time = time.time()


    def stop_microphone_app(self, dest_path):
        """Stops microphone app and gets recorded audio file from container.

        Stops microphone app.
        Copies the recorded file from container to Cros device.
        Deletes the recorded file in container.

        @param dest_path: Destination path of the recorded file on Cros device.

        @raises: ArcMicrophoneResourceException if microphone app is not started
                 yet or is still recording.

        """
        if not self._mic_app_start_time:
            raise ArcMicrophoneResourceException(
                    'Recording is not started yet')

        if self._is_recording():
            raise ArcMicrophoneResourceException('Still recording')

        self._stop_app()
        self._get_file(dest_path)
        self._delete_file()

        self._mic_app_start_time = None


    def _is_recording(self):
        """Checks if microphone app is recording audio.

        We use the time stamp of app start up time to determine if app is still
        recording audio.

        @returns: True if microphone app is recording, False otherwise.

        """
        if not self._mic_app_start_time:
            return False

        return (time.time() - self._mic_app_start_time <
                (arc_resource_common.MicrophoneProps.RECORD_SECS +
                 arc_resource_common.MicrophoneProps.RECORD_FUZZ_SECS))


    def _set_permission(self):
        """Grants permissions to microphone app."""
        for permission in self._MICROPHONE_PERMISSIONS:
            arc.adb_shell('pm grant %s android.permission.%s' % (
                    pipes.quote(self._MICROPHONE_PACKAGE),
                    pipes.quote(permission)))


    def _start_app(self):
        """Starts microphone app."""
        arc.adb_shell('am start -W %s' % pipes.quote(self._MICROPHONE_ACTIVITY))


    def _stop_app(self):
        """Stops microphone app.

        Stops the microphone app process.

        """
        arc.adb_shell(
                'am force-stop %s' % pipes.quote(self._MICROPHONE_PACKAGE))


    def _get_file(self, dest_path):
        """Gets recorded audio file from container.

        Copies the recorded file from container to Cros device.

        @dest_path: Destination path of the recorded file on Cros device.

        """
        arc.adb_cmd('pull %s %s' % (pipes.quote(self._MICROPHONE_RECORD_PATH),
                                    pipes.quote(dest_path)))


    def _delete_file(self):
        """Removes the recorded file in container."""
        arc.adb_shell('rm %s' % pipes.quote(self._MICROPHONE_RECORD_PATH))


class ArcPlayMusicResourceException(Exception):
    """Exceptions in ArcPlayMusicResource."""
    pass


class ArcPlayMusicResource(object):
    """Class to manage Play Music app in container."""
    _PLAYMUSIC_PACKAGE = 'com.google.android.music'
    _PLAYMUSIC_FILE_FOLDER = '/storage/emulated/0/'
    _PLAYMUSIC_PERMISSIONS = ['WRITE_EXTERNAL_STORAGE', 'READ_EXTERNAL_STORAGE']
    _PLAYMUSIC_ACTIVITY = '.AudioPreview'
    _KEYCODE_MEDIA_STOP = 86

    def __init__(self):
        """Initializes an ArcPlayMusicResource."""
        self._files_pushed = []


    def set_playback_file(self, file_path):
        """Copies file into container.

        @param file_path: Path to the file to play on Cros host.

        @returns: Path to the file in container.

        """
        file_name = os.path.basename(file_path)
        dest_path = os.path.join(self._PLAYMUSIC_FILE_FOLDER, file_name)

        # pipes.quote is deprecated in 2.7 (but still available).
        # It should be replaced by shlex.quote in python 3.3.
        arc.adb_cmd('push %s %s' % (pipes.quote(file_path),
                                    pipes.quote(dest_path)))

        self._files_pushed.append(dest_path)

        return dest_path


    def start_playback(self, dest_path):
        """Starts Play Music app to play an audio file.

        @param dest_path: The file path in container.

        @raises ArcPlayMusicResourceException: Play Music app is not ready or
                                               playback file is not set yet.

        """
        if not tag_exists(arc_resource_common.PlayMusicProps.READY_TAG_FILE):
            raise ArcPlayMusicResourceException(
                    'Play Music app is not ready yet.')

        if dest_path not in self._files_pushed:
            raise ArcPlayMusicResourceException(
                    'Playback file is not set yet')

        # In case the permissions are cleared, set the permission again before
        # each start of the app.
        self._set_permission()
        self._start_app(dest_path)


    def _set_permission(self):
        """Grants permissions to Play Music app."""
        for permission in self._PLAYMUSIC_PERMISSIONS:
            arc.adb_shell('pm grant %s android.permission.%s' % (
                    pipes.quote(self._PLAYMUSIC_PACKAGE),
                    pipes.quote(permission)))


    def _start_app(self, dest_path):
        """Starts Play Music app playing an audio file.

        @param dest_path: Path to the file to play in container.

        """
        ext = os.path.splitext(dest_path)[1]
        command = ('am start -a android.intent.action.VIEW'
                   ' -d "file://%s" -t "audio/%s"'
                   ' -n "%s/%s"'% (
                          pipes.quote(dest_path), pipes.quote(ext),
                          pipes.quote(self._PLAYMUSIC_PACKAGE),
                          pipes.quote(self._PLAYMUSIC_ACTIVITY)))
        logging.debug(command)
        arc.adb_shell(command)


    def stop_playback(self):
        """Stops Play Music app.

        Stops the Play Music app by media key event.

        """
        arc.send_keycode(self._KEYCODE_MEDIA_STOP)


    def cleanup(self):
        """Removes the files to play in container."""
        for path in self._files_pushed:
            arc.adb_shell('rm %s' % pipes.quote(path))
        self._files_pushed = []


class ArcPlayVideoResourceException(Exception):
    """Exceptions in ArcPlayVideoResource."""
    pass


class ArcPlayVideoResource(object):
    """Class to manage Play Video app in container."""
    _PLAYVIDEO_PACKAGE = 'org.chromium.arc.testapp.video'
    _PLAYVIDEO_ACTIVITY = 'org.chromium.arc.testapp.video/.MainActivity'
    _PLAYVIDEO_EXIT_TAG = "/mnt/sdcard/ArcVideoTest.tag"
    _PLAYVIDEO_FILE_FOLDER = '/storage/emulated/0/'
    _PLAYVIDEO_PERMISSIONS = ['WRITE_EXTERNAL_STORAGE', 'READ_EXTERNAL_STORAGE']
    _KEYCODE_MEDIA_PLAY = 126
    _KEYCODE_MEDIA_PAUSE = 127
    _KEYCODE_MEDIA_STOP = 86

    def __init__(self):
        """Initializes an ArcPlayVideoResource."""
        self._files_pushed = []


    def prepare_playback(self, file_path, fullscreen=True):
        """Copies file into the container and starts the video player app.

        @param file_path: Path to the file to play on Cros host.
        @param fullscreen: Plays the video in fullscreen.

        """
        if not tag_exists(arc_resource_common.PlayVideoProps.READY_TAG_FILE):
            raise ArcPlayVideoResourceException(
                    'Play Video app is not ready yet.')
        file_name = os.path.basename(file_path)
        dest_path = os.path.join(self._PLAYVIDEO_FILE_FOLDER, file_name)

        # pipes.quote is deprecated in 2.7 (but still available).
        # It should be replaced by shlex.quote in python 3.3.
        arc.adb_cmd('push %s %s' % (pipes.quote(file_path),
                                    pipes.quote(dest_path)))

        # In case the permissions are cleared, set the permission again before
        # each start of the app.
        self._set_permission()
        self._start_app(dest_path)
        if fullscreen:
            self.set_fullscreen()


    def set_fullscreen(self):
        """Sends F4 keyevent to set fullscreen."""
        input_player = input_playback.InputPlayback()
        input_player.emulate(input_type='keyboard')
        input_player.find_connected_inputs()
        input_player.blocking_playback_of_default_file(
                input_type='keyboard', filename='keyboard_f4')


    def start_playback(self, blocking_secs=None):
        """Starts Play Video app to play a video file.

        @param blocking_secs: A positive number indicates the timeout to wait
                              for the playback is finished. Set None to make
                              it non-blocking.

        @raises ArcPlayVideoResourceException: Play Video app is not ready or
                                               playback file is not set yet.

        """
        arc.send_keycode(self._KEYCODE_MEDIA_PLAY)

        if blocking_secs:
            tag = lambda : arc.check_android_file_exists(
                    self._PLAYVIDEO_EXIT_TAG)
            exception = error.TestFail('video playback timeout')
            utils.poll_for_condition(tag, exception, blocking_secs)


    def _set_permission(self):
        """Grants permissions to Play Video app."""
        arc.grant_permissions(
                self._PLAYVIDEO_PACKAGE, self._PLAYVIDEO_PERMISSIONS)


    def _start_app(self, dest_path):
        """Starts Play Video app playing a video file.

        @param dest_path: Path to the file to play in container.

        """
        arc.adb_shell('am start --activity-clear-top '
                      '--es PATH {} {}'.format(
                      pipes.quote(dest_path), self._PLAYVIDEO_ACTIVITY))


    def pause_playback(self):
        """Pauses Play Video app.

        Pauses the Play Video app by media key event.

        """
        arc.send_keycode(self._KEYCODE_MEDIA_PAUSE)


    def stop_playback(self):
        """Stops Play Video app.

        Stops the Play Video app by media key event.

        """
        arc.send_keycode(self._KEYCODE_MEDIA_STOP)


    def cleanup(self):
        """Removes the files to play in container."""
        for path in self._files_pushed:
            arc.adb_shell('rm %s' % pipes.quote(path))
        self._files_pushed = []


class ArcResource(object):
    """Class to manage multimedia resource in container.

    @properties:
        microphone: The instance of ArcMicrophoneResource for microphone app.
        play_music: The instance of ArcPlayMusicResource for music app.
        play_video: The instance of ArcPlayVideoResource for video app.

    """
    def __init__(self):
        self.microphone = ArcMicrophoneResource()
        self.play_music = ArcPlayMusicResource()
        self.play_video = ArcPlayVideoResource()


    def cleanup(self):
        """Clean up the resources."""
        self.play_music.cleanup()
        self.play_video.cleanup()
