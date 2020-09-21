# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides the audio widgets related to ARC used in audio tests."""

import copy
import tempfile

from autotest_lib.client.cros.audio import audio_test_data
from autotest_lib.client.cros.chameleon import audio_widget

class CrosInputWidgetARCHandler(audio_widget.CrosInputWidgetHandler):
    """

    This class abstracts a Cros device audio input widget ARC handler.

    """
    # AMR-NB uses variable bit rates so we set sample_format to None.
    # Other format info are actually useless for sox because sox can read them
    # from file header.
    _SOURCE_FORMAT = dict(file_type='amr-nb',
                          sample_format=None,
                          channel=1,
                          rate=8000)

    def start_recording(self):
        """Starts recording audio through ARC."""
        self._audio_facade.start_arc_recording()


    def stop_recording(self):
        """Stops recording audio through ARC.

        @returns:
            A tuple (remote_path, format).
                remote_path: The path to the recorded file on Cros device.
                format: A dict containing:
                    file_type: 'raw'.
                    sample_format: 'S16_LE' for 16-bit signed integer in
                                   little-endian.
                    channel: channel number.
                    rate: sampling rate.

        """
        return (self._audio_facade.stop_arc_recording(),
                self._DEFAULT_DATA_FORMAT)


    def get_recorded_binary(self, remote_path, record_format):
        """Gets remote recorded file binary from Cros device..

        Gets and reads recorded file from Cros device.
        The argument 'record_format' is what API user want on output.
        The real file format of file at 'remote_path' can be another source
        format. This method handles the format conversion from source format
        into record_format, and returns the converted binary.

        Handle the format conversion from source format into record_format.

        @param remote_path: The path to the recorded file on Cros device.
        @param record_format: The data format of returned binary.
                     A dict containing
                     file_type: 'raw' or 'wav'.
                     sample_format: 'S32_LE' for 32-bit signed integer in
                                    little-endian. Refer to aplay manpage for
                                    other formats.
                     channel: channel number.
                     rate: sampling rate.

        @returns: The recorded binary.

        @raises: CrosInputWidgetHandlerError if record_format is not correct.

        """
        if record_format != self._DEFAULT_DATA_FORMAT:
            raise audio_widget.CrosInputWidgetHandlerError(
                    'Record format %r is not valid' % record_format)

        ext = '.' + self._SOURCE_FORMAT['file_type']
        with tempfile.NamedTemporaryFile(prefix='recorded_', suffix=ext) as f:
            self._audio_facade.get_recorded_file(remote_path, f.name)

            # Handles conversion from source format into record_format.
            test_data = audio_test_data.AudioTestData(
                    self._SOURCE_FORMAT, f.name)
            converted_test_data = test_data.convert(record_format, 1.0)
            try:
                return converted_test_data.get_binary()
            finally:
                converted_test_data.delete()


class CrosOutputWidgetARCHandlerError(Exception):
    """Error in CrosOutputWidgetARCHandler."""
    pass


class CrosOutputWidgetARCHandler(audio_widget.CrosOutputWidgetHandler):
    """This class abstracts a Cros device audio output widget ARC handler."""
    _SUPPORTED_FILE_TYPES = ['wav', 'mp3']
    _DEFAULT_FILE_TYPE = 'wav'

    def set_playback_data(self, test_data):
        """Sets data to play.

        @param test_data: An AudioTestData object.

        @returns: Path to the file in container on Cros host.

        """
        # Handle the format conversion because ARC does not recognize raw file.
        if test_data.data_format['file_type'] not in self._SUPPORTED_FILE_TYPES:
            new_data_format = copy.deepcopy(test_data.data_format)
            new_data_format['file_type'] = self._DEFAULT_FILE_TYPE
            test_data = test_data.convert(new_data_format, 1.0)
        return self._audio_facade.set_arc_playback_file(test_data.path)


    def start_playback(self, path, blocking=False):
        """Starts playing audio.

        @param path: Path to the file to play in container on Cros host.
        @param blocking: Blocks this call until playback finishes.

        @raises: NotImplementedError if blocking is True.

        """
        if blocking:
            raise NotImplementedError(
                    'Blocking playback on ARC is not supported.')
        self._audio_facade.start_arc_playback(path)


    def stop_playback(self):
        """Stops playing audio."""
        self._audio_facade.stop_arc_playback()
