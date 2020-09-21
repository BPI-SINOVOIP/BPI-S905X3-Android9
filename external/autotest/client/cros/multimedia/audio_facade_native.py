# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Facade to access the audio-related functionality."""

import functools
import glob
import logging
import numpy as np
import os
import tempfile

from autotest_lib.client.cros import constants
from autotest_lib.client.cros.audio import audio_helper
from autotest_lib.client.cros.audio import cmd_utils
from autotest_lib.client.cros.audio import cras_dbus_utils
from autotest_lib.client.cros.audio import cras_utils
from autotest_lib.client.cros.multimedia import audio_extension_handler


class AudioFacadeNativeError(Exception):
    """Error in AudioFacadeNative."""
    pass


def check_arc_resource(func):
    """Decorator function for ARC related functions in AudioFacadeNative."""
    @functools.wraps(func)
    def wrapper(instance, *args, **kwargs):
        """Wrapper for the methods to check _arc_resource.

        @param instance: Object instance.

        @raises: AudioFacadeNativeError if there is no ARC resource.

        """
        if not instance._arc_resource:
            raise AudioFacadeNativeError('There is no ARC resource.')
        return func(instance, *args, **kwargs)
    return wrapper


def file_contains_all_zeros(path):
    """Reads a file and checks whether the file contains all zeros."""
    with open(path) as f:
        binary = f.read()
        # Assume data is in 16 bit signed int format. The real format
        # does not matter though since we only care if there is nonzero data.
        np_array = np.fromstring(binary, dtype='<i2')
        return not np.any(np_array)


class AudioFacadeNative(object):
    """Facede to access the audio-related functionality.

    The methods inside this class only accept Python native types.

    """
    _CAPTURE_DATA_FORMATS = [
            dict(file_type='raw', sample_format='S16_LE',
                 channel=1, rate=48000),
            dict(file_type='raw', sample_format='S16_LE',
                 channel=2, rate=48000)]

    _PLAYBACK_DATA_FORMAT = dict(
            file_type='raw', sample_format='S16_LE', channel=2, rate=48000)

    def __init__(self, resource, arc_resource=None):
        """Initializes an audio facade.

        @param resource: A FacadeResource object.
        @param arc_resource: An ArcResource object.

        """
        self._resource = resource
        self._recorder = None
        self._player = None
        self._counter = None
        self._loaded_extension_handler = None
        self._arc_resource = arc_resource


    @property
    def _extension_handler(self):
        """Multimedia test extension handler."""
        if not self._loaded_extension_handler:
            extension = self._resource.get_extension(
                    constants.AUDIO_TEST_EXTENSION)
            logging.debug('Loaded extension: %s', extension)
            self._loaded_extension_handler = (
                    audio_extension_handler.AudioExtensionHandler(extension))
        return self._loaded_extension_handler


    def get_audio_devices(self):
        """Returns the audio devices from chrome.audio API.

        @returns: Checks docstring of get_audio_devices of AudioExtensionHandler.

        """
        return self._extension_handler.get_audio_devices()


    def set_chrome_active_volume(self, volume):
        """Sets the active audio output volume using chrome.audio API.

        @param volume: Volume to set (0~100).

        """
        self._extension_handler.set_active_volume(volume)


    def set_chrome_mute(self, mute):
        """Mutes the active audio output using chrome.audio API.

        @param mute: True to mute. False otherwise.

        """
        self._extension_handler.set_mute(mute)


    def get_chrome_active_volume_mute(self):
        """Gets the volume state of active audio output using chrome.audio API.

        @param returns: A tuple (volume, mute), where volume is 0~100, and mute
                        is True if node is muted, False otherwise.

        """
        return self._extension_handler.get_active_volume_mute()


    def set_chrome_active_node_type(self, output_node_type, input_node_type):
        """Sets active node type through chrome.audio API.

        The node types are defined in cras_utils.CRAS_NODE_TYPES.
        The current active node will be disabled first if the new active node
        is different from the current one.

        @param output_node_type: A node type defined in
                                 cras_utils.CRAS_NODE_TYPES. None to skip.
        @param input_node_type: A node type defined in
                                 cras_utils.CRAS_NODE_TYPES. None to skip

        """
        if output_node_type:
            node_id = cras_utils.get_node_id_from_node_type(
                    output_node_type, False)
            self._extension_handler.set_active_node_id(node_id)
        if input_node_type:
            node_id = cras_utils.get_node_id_from_node_type(
                    input_node_type, True)
            self._extension_handler.set_active_node_id(node_id)


    def cleanup(self):
        """Clean up the temporary files."""
        for path in glob.glob('/tmp/playback_*'):
            os.unlink(path)

        for path in glob.glob('/tmp/capture_*'):
            os.unlink(path)

        if self._recorder:
            self._recorder.cleanup()
        if self._player:
            self._player.cleanup()

        if self._arc_resource:
            self._arc_resource.cleanup()


    def playback(self, file_path, data_format, blocking=False):
        """Playback a file.

        @param file_path: The path to the file.
        @param data_format: A dict containing data format including
                            file_type, sample_format, channel, and rate.
                            file_type: file type e.g. 'raw' or 'wav'.
                            sample_format: One of the keys in
                                           audio_data.SAMPLE_FORMAT.
                            channel: number of channels.
                            rate: sampling rate.
        @param blocking: Blocks this call until playback finishes.

        @returns: True.

        @raises: AudioFacadeNativeError if data format is not supported.

        """
        logging.info('AudioFacadeNative playback file: %r. format: %r',
                     file_path, data_format)

        if data_format != self._PLAYBACK_DATA_FORMAT:
            raise AudioFacadeNativeError(
                    'data format %r is not supported' % data_format)

        self._player = Player()
        self._player.start(file_path, blocking)

        return True


    def stop_playback(self):
        """Stops playback process."""
        self._player.stop()


    def start_recording(self, data_format):
        """Starts recording an audio file.

        Currently the format specified in _CAPTURE_DATA_FORMATS is the only
        formats.

        @param data_format: A dict containing:
                            file_type: 'raw'.
                            sample_format: 'S16_LE' for 16-bit signed integer in
                                           little-endian.
                            channel: channel number.
                            rate: sampling rate.


        @returns: True

        @raises: AudioFacadeNativeError if data format is not supported.

        """
        logging.info('AudioFacadeNative record format: %r', data_format)

        if data_format not in self._CAPTURE_DATA_FORMATS:
            raise AudioFacadeNativeError(
                    'data format %r is not supported' % data_format)

        self._recorder = Recorder()
        self._recorder.start(data_format)

        return True


    def stop_recording(self):
        """Stops recording an audio file.

        @returns: The path to the recorded file.
                  None if capture device is not functional.

        """
        self._recorder.stop()
        if file_contains_all_zeros(self._recorder.file_path):
            logging.error('Recorded file contains all zeros. '
                          'Capture device is not functional')
            return None
        return self._recorder.file_path


    def set_selected_output_volume(self, volume):
        """Sets the selected output volume.

        @param volume: the volume to be set(0-100).

        """
        cras_utils.set_selected_output_node_volume(volume)


    def set_input_gain(self, gain):
        """Sets the system capture gain.

        @param gain: the capture gain in db*100 (100 = 1dB)

        """
        cras_utils.set_capture_gain(gain)


    def set_selected_node_types(self, output_node_types, input_node_types):
        """Set selected node types.

        The node types are defined in cras_utils.CRAS_NODE_TYPES.

        @param output_node_types: A list of output node types.
                                  None to skip setting.
        @param input_node_types: A list of input node types.
                                 None to skip setting.

        """
        cras_utils.set_selected_node_types(output_node_types, input_node_types)


    def get_selected_node_types(self):
        """Gets the selected output and input node types.

        @returns: A tuple (output_node_types, input_node_types) where each
                  field is a list of selected node types defined in
                  cras_utils.CRAS_NODE_TYPES.

        """
        return cras_utils.get_selected_node_types()


    def get_plugged_node_types(self):
        """Gets the plugged output and input node types.

        @returns: A tuple (output_node_types, input_node_types) where each
                  field is a list of plugged node types defined in
                  cras_utils.CRAS_NODE_TYPES.

        """
        return cras_utils.get_plugged_node_types()


    def dump_diagnostics(self, file_path):
        """Dumps audio diagnostics results to a file.

        @param file_path: The path to dump results.

        @returns: True

        """
        with open(file_path, 'w') as f:
            f.write(audio_helper.get_audio_diagnostics())
        return True


    def start_counting_signal(self, signal_name):
        """Starts counting DBus signal from Cras.

        @param signal_name: Signal of interest.

        """
        if self._counter:
            raise AudioFacadeNativeError('There is an ongoing counting.')
        self._counter = cras_dbus_utils.CrasDBusBackgroundSignalCounter()
        self._counter.start(signal_name)


    def stop_counting_signal(self):
        """Stops counting DBus signal from Cras.

        @returns: Number of signals starting from last start_counting_signal
                  call.

        """
        if not self._counter:
            raise AudioFacadeNativeError('Should start counting signal first')
        result = self._counter.stop()
        self._counter = None
        return result


    def wait_for_unexpected_nodes_changed(self, timeout_secs):
        """Waits for unexpected nodes changed signal.

        @param timeout_secs: Timeout in seconds for waiting.

        """
        cras_dbus_utils.wait_for_unexpected_nodes_changed(timeout_secs)


    @check_arc_resource
    def start_arc_recording(self):
        """Starts recording using microphone app in container."""
        self._arc_resource.microphone.start_microphone_app()


    @check_arc_resource
    def stop_arc_recording(self):
        """Checks the recording is stopped and gets the recorded path.

        The recording duration of microphone app is fixed, so this method just
        copies the recorded result from container to a path on Cros device.

        """
        _, file_path = tempfile.mkstemp(prefix='capture_', suffix='.amr-nb')
        self._arc_resource.microphone.stop_microphone_app(file_path)
        return file_path


    @check_arc_resource
    def set_arc_playback_file(self, file_path):
        """Copies the audio file to be played into container.

        User should call this method to put the file into container before
        calling start_arc_playback.

        @param file_path: Path to the file to be played on Cros host.

        @returns: Path to the file in container.

        """
        return self._arc_resource.play_music.set_playback_file(file_path)


    @check_arc_resource
    def start_arc_playback(self, path):
        """Start playback through Play Music app.

        Before calling this method, user should call set_arc_playback_file to
        put the file into container.

        @param path: Path to the file in container.

        """
        self._arc_resource.play_music.start_playback(path)


    @check_arc_resource
    def stop_arc_playback(self):
        """Stop playback through Play Music app."""
        self._arc_resource.play_music.stop_playback()


class RecorderError(Exception):
    """Error in Recorder."""
    pass


class Recorder(object):
    """The class to control recording subprocess.

    Properties:
        file_path: The path to recorded file. It should be accessed after
                   stop() is called.

    """
    def __init__(self):
        """Initializes a Recorder."""
        _, self.file_path = tempfile.mkstemp(prefix='capture_', suffix='.raw')
        self._capture_subprocess = None


    def start(self, data_format):
        """Starts recording.

        Starts recording subprocess. It can be stopped by calling stop().

        @param data_format: A dict containing:
                            file_type: 'raw'.
                            sample_format: 'S16_LE' for 16-bit signed integer in
                                           little-endian.
                            channel: channel number.
                            rate: sampling rate.

        @raises: RecorderError: If recording subprocess is terminated
                 unexpectedly.

        """
        self._capture_subprocess = cmd_utils.popen(
                cras_utils.capture_cmd(
                        capture_file=self.file_path, duration=None,
                        channels=data_format['channel'],
                        rate=data_format['rate']))


    def stop(self):
        """Stops recording subprocess."""
        if self._capture_subprocess.poll() is None:
            self._capture_subprocess.terminate()
        else:
            raise RecorderError(
                    'Recording process was terminated unexpectedly.')


    def cleanup(self):
        """Cleanup the resources.

        Terminates the recording process if needed.

        """
        if self._capture_subprocess and self._capture_subprocess.poll() is None:
            self._capture_subprocess.terminate()


class PlayerError(Exception):
    """Error in Player."""
    pass


class Player(object):
    """The class to control audio playback subprocess.

    Properties:
        file_path: The path to the file to play.

    """
    def __init__(self):
        """Initializes a Player."""
        self._playback_subprocess = None


    def start(self, file_path, blocking):
        """Starts recording.

        Starts recording subprocess. It can be stopped by calling stop().

        @param file_path: The path to the file.
        @param blocking: Blocks this call until playback finishes.

        """
        self._playback_subprocess = cras_utils.playback(
                blocking, playback_file=file_path)


    def stop(self):
        """Stops playback subprocess."""
        cmd_utils.kill_or_log_returncode(self._playback_subprocess)


    def cleanup(self):
        """Cleanup the resources.

        Terminates the playback process if needed.

        """
        self.stop()
