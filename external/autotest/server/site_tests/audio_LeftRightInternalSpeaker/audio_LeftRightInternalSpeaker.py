# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side internal speaker test using the Chameleon board,
audio board and the audio box enclosure for sound isolation."""

import logging
import os
import time

from autotest_lib.server.cros.audio import audio_test
from autotest_lib.client.cros.audio import audio_test_data
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import chameleon_audio_helper
from autotest_lib.client.cros.chameleon import chameleon_audio_ids
from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.multimedia import remote_facade_factory


class audio_LeftRightInternalSpeaker(audio_test.AudioTest):
    """Server side left/right internal speaker audio test.

    This test verifies:
    1. When a file with audio on the left channel is played, that a sound is
       emitted from at least one speaker.
    2. When a file with audio on the right channel is played, that a sound
       is emitted from at least one speaker.

    This test cannot verify:
    1. If the speaker making the sound is not the one corresponding to the
       channel the audio was embedded in in the file.

    """
    version = 1
    DELAY_BEFORE_RECORD_SECONDS = 0.5
    DELAY_AFTER_BINDING = 0.5
    RECORD_SECONDS = 8
    RIGHT_WAV_FILE_URL = (
        'http://commondatastorage.googleapis.com/chromiumos-test-assets-'
        'public/audio_test/chameleon/Speaker/right_440_half.wav')
    LEFT_WAV_FILE_URL = (
        'http://commondatastorage.googleapis.com/chromiumos-test-assets-'
        'public/audio_test/chameleon/Speaker/left_440_half.wav')

    def run_once(self, host, player):
        """

        Entry point for test case.

        @param host: A reference to the DUT.
        @param player: A string representing what audio player to use. Could
                       be 'native' or 'browser'.

        """

        if not audio_test_utils.has_internal_speaker(host):
            return

        host.chameleon.setup_and_reset(self.outputdir)

        facade_factory = remote_facade_factory.RemoteFacadeFactory(
            host,
            results_dir=self.resultsdir)
        self.audio_facade = facade_factory.create_audio_facade()
        self.browser_facade = facade_factory.create_browser_facade()

        widget_factory = chameleon_audio_helper.AudioWidgetFactory(
            facade_factory,
            host)
        self.sound_source = widget_factory.create_widget(
            chameleon_audio_ids.CrosIds.SPEAKER)
        self.sound_recorder = widget_factory.create_widget(
            chameleon_audio_ids.ChameleonIds.MIC)

        self.play_and_record(
            host,
            player,
            'left')
        self.process_and_save_data(channel='left')
        self.validate_recorded_data(channel='left')

        self.play_and_record(
            host,
            player,
            'right')
        self.process_and_save_data(channel='right')
        self.validate_recorded_data(channel='right')


    def play_and_record(self, host, player, channel):
        """Play file using given details and record playback.

        The recording is accessible through the recorder object and doesn't
        need to be returned explicitly.

        @param host: The DUT.
        @param player: String name of audio player we intend to use.
        @param channel: Either 'left' or 'right'

        """

        #audio_facade = factory.create_audio_facade()
        audio_test_utils.dump_cros_audio_logs(
            host, self.audio_facade, self.resultsdir,
            'before_recording_' + channel)

        # Verify that output node is correct.
        output_nodes, _ = self.audio_facade.get_selected_node_types()
        if output_nodes != ['INTERNAL_SPEAKER']:
            raise error.TestFail(
                '%s rather than internal speaker is selected on Cros '
                'device' % output_nodes)
        self.audio_facade.set_selected_output_volume(80)

        if player == 'native':
            data_format=dict(file_type='raw',
                             sample_format='S16_LE',
                             channel=2,
                             rate=48000)
            if channel == 'left':
                frequencies = [440, 0]
            else:
                frequencies = [0, 440]
            sound_file = audio_test_data.GenerateAudioTestData(
                    data_format=data_format,
                    path=os.path.join(self.bindir, '440_half.raw'),
                    duration_secs=10,
                    frequencies=frequencies)

            logging.info('Going to use cras_test_client on CrOS')
            logging.info('Playing the file %s', sound_file)
            self.sound_source.set_playback_data(sound_file)
            self.sound_source.start_playback()
            time.sleep(self.DELAY_BEFORE_RECORD_SECONDS)
            self.sound_recorder.start_recording()
            time.sleep(self.RECORD_SECONDS)
            self.sound_recorder.stop_recording()
            self.sound_source.stop_playback()
            sound_file.delete()
            logging.info('Recording finished. Was done in format %s',
                         self.sound_recorder.data_format)

        elif player == 'browser':
            if channel == 'left':
                sound_file = self.LEFT_WAV_FILE_URL
            else:
                sound_file = self.RIGHT_WAV_FILE_URL

            tab_descriptor = self.browser_facade.new_tab(sound_file)

            time.sleep(self.DELAY_BEFORE_RECORD_SECONDS)
            logging.info('Start recording from Chameleon.')
            self.sound_recorder.start_recording()

            time.sleep(self.RECORD_SECONDS)

            self.sound_recorder.stop_recording()
            logging.info('Stopped recording from Chameleon.')
            self.browser_facade.close_tab(tab_descriptor)

        else:
            raise error.TestFail(
                '%s is not in list of accepted audio players',
                player)

        audio_test_utils.dump_cros_audio_logs(
            host, self.audio_facade, self.resultsdir,
            'after_recording_' + channel)


    def process_and_save_data(self, channel):
        """Save recorded data to files and process for analysis.

        @param channel: 'left' or 'right'.

        """

        self.sound_recorder.read_recorded_binary()
        file_name = 'recorded_' + channel + '.raw'
        unprocessed_file = os.path.join(self.resultsdir, file_name)
        logging.info('Saving raw unprocessed output to %s', unprocessed_file)
        self.sound_recorder.save_file(unprocessed_file)

        # Removes the beginning of recorded data. This is to avoid artifact
        # caused by Chameleon codec initialization in the beginning of
        # recording.
        self.sound_recorder.remove_head(1.0)

        # Reduce noise
        self.sound_recorder.lowpass_filter(1000)
        file_name = 'recorded_filtered_' + channel + '.raw'
        processsed_file = os.path.join(self.resultsdir, file_name)
        logging.info('Saving processed sound output to %s', processsed_file)
        self.sound_recorder.save_file(processsed_file)


    def validate_recorded_data(self, channel):
        """Read processed data and validate by comparing to golden file.

        @param channel: 'left' or 'right'.

        """

        # Compares data by frequency. Audio signal recorded by microphone has
        # gone through analog processing and through the air.
        # This suffers from codec artifacts and noise on the path.
        # Comparing data by frequency is more robust than comparing by
        # correlation, which is suitable for fully-digital audio path like USB
        # and HDMI.
        logging.info('Validating recorded output for channel %s', channel)
        audio_test_utils.check_recorded_frequency(
            audio_test_data.SIMPLE_FREQUENCY_SPEAKER_TEST_FILE,
            self.sound_recorder,
            second_peak_ratio=0.1,
            ignore_frequencies=[50, 60])
