# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side audio volume test using the Chameleon board."""

import logging
import os
import time

from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import chameleon_audio_ids
from autotest_lib.client.cros.chameleon import chameleon_audio_helper
from autotest_lib.server.cros.audio import audio_test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class audio_AudioArtifacts(audio_test.AudioTest):
    """Server side audio artifacts test.

    This test talks to a Chameleon board and a Cros device to detect
    audio artifacts from the Cros device.

    """
    version = 1
    RECORD_SECONDS = 20
    DELAY_AFTER_BINDING = 0.5
    KEEP_VOLUME_SECONDS = 0.5
    START_PLAYBACK_SECONDS = 0.5

    def run_once(self, host, source_id, sink_id, recorder_id,
                 golden_file, switch_hsp=False,
                 mute_duration_in_secs=None,
                 volume_changes=None,
                 record_secs=None):
        """Running audio volume test.

        mute_duration_in_secs and volume_changes couldn't be both not None.

        @param host: device under test CrosHost
        @param source_id: An ID defined in chameleon_audio_ids for source.
        @param sink_id: An ID defined in chameleon_audio_ids for sink if needed.
                        Currently this is only used on bluetooth.
        @param recorder: An ID defined in chameleon_audio_ids for recording.
        @param golden_file: A test file defined in audio_test_data.
        @param switch_hsp: Run a recording process on Cros device. This is
                           to trigger Cros switching from A2DP to HSP.
        @param mute_duration_in_secs: List of each duration of mute. For example,
                                      [0.4, 0.6] will have two delays, each
                                      duration will be 0.4 secs and 0.6 secs.
                                      And, between delays, there will be
                                      KEEP_VOLUME_SECONDS secs sine wave.
        @param volume_changes: List consisting of -1 and +1, where -1 denoting
                           decreasing volume, +1 denoting increasing volume.
                           Between each changes, the volume will be kept for
                           KEEP_VOLUME_SECONDS secs.
        @param record_secs: The duration of recording in seconds. If it is not
                            set, duration of the test data will be used. If
                            duration of test data is not set either, default
                            RECORD_SECONDS will be used.

        """
        if (source_id == chameleon_audio_ids.CrosIds.SPEAKER and
            not audio_test_utils.has_internal_speaker(host)):
            return

        chameleon_board = host.chameleon
        factory = remote_facade_factory.RemoteFacadeFactory(
                host, results_dir=self.resultsdir)

        chameleon_board.setup_and_reset(self.outputdir)

        widget_factory = chameleon_audio_helper.AudioWidgetFactory(
                factory, host)

        source = widget_factory.create_widget(source_id)
        recorder = widget_factory.create_widget(recorder_id)

        # Chameleon Mic does not need binding.
        binding = (recorder_id != chameleon_audio_ids.ChameleonIds.MIC)

        binder = None

        if binding:
            if sink_id:
                sink = widget_factory.create_widget(sink_id)
                binder = widget_factory.create_binder(source, sink, recorder)
            else:
                binder = widget_factory.create_binder(source, recorder)

        start_volume, low_volume, high_volume = 75, 50, 100

        if not record_secs:
            if golden_file.duration_secs:
                record_secs = golden_file.duration_secs
            else:
                record_secs = self.RECORD_SECONDS
        logging.debug('Record duration: %f seconds', record_secs)

        # Checks if the file is local or is served on web.
        file_url = getattr(golden_file, 'url', None)

        with chameleon_audio_helper.bind_widgets(binder):
            # Checks the node selected by cras is correct.
            time.sleep(self.DELAY_AFTER_BINDING)
            audio_facade = factory.create_audio_facade()

            audio_test_utils.dump_cros_audio_logs(
                    host, audio_facade, self.resultsdir, 'after_binding')

            audio_facade.set_chrome_active_volume(start_volume)

            audio_test_utils.check_output_port(audio_facade, source.port_id)

            if switch_hsp:
                audio_test_utils.switch_to_hsp(audio_facade)

            if not file_url:
                logging.info('Setting playback data on Cros device')
                source.set_playback_data(golden_file)

            logging.info('Start recording from Chameleon.')
            recorder.start_recording()

            if not file_url:
                logging.info('Start playing %s on Cros device',
                             golden_file.path)
                source.start_playback()
            else:
                logging.info('Start playing %s on Cros device using browser',
                             file_url)
                browser_facade = factory.create_browser_facade()
                tab_descriptor = browser_facade.new_tab(file_url)

            if volume_changes:
                time.sleep(self.START_PLAYBACK_SECONDS)
                for x in volume_changes:
                    if x == -1:
                        audio_facade.set_chrome_active_volume(low_volume)
                    if x == +1:
                        audio_facade.set_chrome_active_volume(high_volume)
                    time.sleep(self.KEEP_VOLUME_SECONDS)
                passed_time_secs = self.START_PLAYBACK_SECONDS
                passed_time_secs += len(volume_changes) * self.KEEP_VOLUME_SECONDS
                rest = max(0, record_secs - passed_time_secs)
                time.sleep(rest)
            elif mute_duration_in_secs:
                time.sleep(self.START_PLAYBACK_SECONDS)
                passed_time_seconds = self.START_PLAYBACK_SECONDS
                for mute_secs in mute_duration_in_secs:
                    audio_facade.set_chrome_active_volume(0)
                    time.sleep(mute_secs)
                    audio_facade.set_chrome_active_volume(start_volume)
                    time.sleep(self.KEEP_VOLUME_SECONDS)
                    passed_time_seconds += mute_secs + self.KEEP_VOLUME_SECONDS
                rest = max(0, record_secs - passed_time_seconds)
                time.sleep(rest)
            else:
                time.sleep(record_secs)

            recorder.stop_recording()
            logging.info('Stopped recording from Chameleon.')

            audio_test_utils.dump_cros_audio_logs(
                    host, audio_facade, self.resultsdir,
                    'after_recording')

            if file_url:
                browser_facade.close_tab(tab_descriptor)
            else:
                source.stop_playback()

            recorder.read_recorded_binary()
            logging.info('Read recorded binary from Chameleon.')

            recorded_file = os.path.join(
                    self.resultsdir, "recorded.raw" )
            logging.info('Saving recorded data to %s', recorded_file)
            recorder.save_file(recorded_file)

            audio_test_utils.check_recorded_frequency(
                    golden_file, recorder,
                    check_artifacts=True,
                    mute_durations=mute_duration_in_secs,
                    volume_changes=volume_changes)

