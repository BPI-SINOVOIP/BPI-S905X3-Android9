# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side ARC audio playback test using the Chameleon board."""

import logging
import os
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import chameleon_audio_helper
from autotest_lib.client.cros.chameleon import chameleon_audio_ids
from autotest_lib.client.cros.multimedia import arc_resource_common
from autotest_lib.server import autotest
from autotest_lib.server.cros.audio import audio_test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class audio_AudioARCPlayback(audio_test.AudioTest):
    """Server side ARC audio playback test.

    This test talks to a Chameleon board and a Cros device to verify
    audio playback function of the Cros device with ARC.

    """
    version = 1
    DELAY_AFTER_BINDING = 0.5
    WAIT_CLIENT_READY_TIMEOUT_SECS = 150
    WAIT_PLAYBACK_SECS = 10

    def run_once(self, host, source_id, sink_id, recorder_id,
                 golden_file, switch_hsp=False):
        """Runs record test through ARC on Cros device.

        @param host: device under test, a CrosHost.
        @param source_id: An ID defined in chameleon_audio_ids for source.
        @param sink_id: An ID defined in chameleon_audio_ids for sink if needed.
                        Currently this is only used on bluetooth.
        @param recorder_id: An ID defined in chameleon_audio_ids for recording.
        @param golden_file: A test file defined in audio_test_data.
        @param switch_hsp: Run a recording process on Cros device. This is
                           to trigger Cros switching from A2DP to HSP.

        """
        self.host = host

        if (source_id == chameleon_audio_ids.CrosIds.SPEAKER and
            not audio_test_utils.has_internal_speaker(host)):
            return

        self.client_at = None

        # Runs a client side test to start Chrome and install Play Music app.
        self.setup_playmusic_app()

        # Do not start Chrome because client side test had started it.
        # Do not install autotest because client side test had installed it.
        factory = remote_facade_factory.RemoteFacadeFactory(
                host, no_chrome=True, install_autotest=False,
                results_dir=self.resultsdir)

        # Setup Chameleon and create widgets.
        host.chameleon.setup_and_reset(self.outputdir)

        widget_factory = chameleon_audio_helper.AudioWidgetFactory(
                factory, host)

        source = widget_factory.create_widget(source_id, use_arc=True)
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

        # Second peak ratio is determined by quality of audio path.
        second_peak_ratio = audio_test_utils.get_second_peak_ratio(
                source_id=source_id,
                recorder_id=recorder_id,
                is_hsp=switch_hsp)

        with chameleon_audio_helper.bind_widgets(binder):
            time.sleep(self.DELAY_AFTER_BINDING)

            audio_facade = factory.create_audio_facade()

            audio_test_utils.dump_cros_audio_logs(
                    host, audio_facade, self.resultsdir, 'after_binding')

            # Checks the node selected by CRAS is correct.
            audio_test_utils.check_output_port(audio_facade, source.port_id)

            if switch_hsp:
                audio_test_utils.switch_to_hsp(audio_facade)

            logging.info('Setting playback file on Cros device')
            source.set_playback_data(golden_file)

            logging.info('Start recording from Chameleon')
            recorder.start_recording()

            logging.info('Start playing %s on Cros device',
                         golden_file.path)
            source.start_playback()

            time.sleep(self.WAIT_PLAYBACK_SECS)

            recorder.stop_recording()
            logging.info('Stopped recording from Chameleon.')

            audio_test_utils.dump_cros_audio_logs(
                    host, audio_facade, self.resultsdir,
                    'after_recording')

            recorder.read_recorded_binary()
            logging.info('Read recorded binary from Chameleon.')

            recorded_file = os.path.join(self.resultsdir, "recorded.raw")
            logging.info('Saving recorded data to %s', recorded_file)
            recorder.save_file(recorded_file)

            audio_test_utils.check_recorded_frequency(
                    golden_file, recorder,
                    second_peak_ratio=second_peak_ratio)


    def run_client_side_test(self):
        """Runs a client side test on Cros device in background."""
        self.client_at = autotest.Autotest(self.host)
        logging.info('Start running client side test %s',
                     arc_resource_common.PlayMusicProps.TEST_NAME)
        self.client_at.run_test(
                arc_resource_common.PlayMusicProps.TEST_NAME,
                background=True)


    def setup_playmusic_app(self):
        """Setups Play Music app on Cros device.

        Runs a client side test on Cros device to start Chrome and ARC and
        install Play Music app.
        Wait for it to be ready.

        """
        # Removes ready tag that server side test should wait for later.
        self.remove_ready_tag()

        # Runs the client side test.
        self.run_client_side_test()

        logging.info('Waiting for client side Play Music app to be ready')

        # Waits for ready tag to be posted by client side test.
        utils.poll_for_condition(condition=self.ready_tag_exists,
                                 timeout=self.WAIT_CLIENT_READY_TIMEOUT_SECS,
                                 desc='Wait for client side test being ready',
                                 sleep_interval=1)

        logging.info('Client side Play Music app is ready')


    def cleanup(self):
        """Cleanup of the test."""
        self.touch_exit_tag()
        super(audio_AudioARCPlayback, self).cleanup()


    def remove_ready_tag(self):
        """Removes ready tag on Cros device."""
        if self.ready_tag_exists():
            self.host.run(command='rm %s' % (
                    arc_resource_common.PlayMusicProps.READY_TAG_FILE))


    def touch_exit_tag(self):
        """Touches exit tag on Cros device to stop client side test."""
        self.host.run(command='touch %s' % (
                arc_resource_common.PlayMusicProps.EXIT_TAG_FILE))


    def ready_tag_exists(self):
        """Checks if ready tag exists.

        @returns: True if the tag file exists. False otherwise.

        """
        return self.host.path_exists(
                arc_resource_common.PlayMusicProps.READY_TAG_FILE)
