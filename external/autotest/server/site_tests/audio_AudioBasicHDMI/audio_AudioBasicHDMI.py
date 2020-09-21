# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side HDMI audio test using the Chameleon board."""

import logging
import os
import threading
import time

from autotest_lib.client.cros.audio import audio_test_data
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import chameleon_audio_helper
from autotest_lib.client.cros.chameleon import chameleon_audio_ids
from autotest_lib.server.cros.audio import audio_test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class audio_AudioBasicHDMI(audio_test.AudioTest):
    """Server side HDMI audio test.

    This test talks to a Chameleon board and a Cros device to verify
    HDMI audio function of the Cros device.

    """
    version = 2
    DELAY_BEFORE_PLAYBACK = 2
    DELAY_AFTER_PLAYBACK = 2
    CONNECT_TIMEOUT_SEC = 30
    SUSPEND_SEC = 15
    WAIT_TO_REBIND_SEC = 15
    WEB_PLAYBACK_SEC = 15

    def cleanup(self):
        """Restore the CPU scaling governor mode."""
        self._system_facade.set_scaling_governor_mode(0, self._original_mode)
        logging.debug('Set CPU0 mode to %s', self._original_mode)


    def set_high_performance_mode(self):
        """Set the CPU scaling governor mode to performance mode."""
        self._original_mode = self._system_facade.set_scaling_governor_mode(
                0, 'performance')
        logging.debug('Set CPU0 scaling governor mode to performance, '
                      'original_mode: %s', self._original_mode)


    def playback_and_suspend(self, audio_facade, while_playback):
        """ Does playback and suspend-resume.

        @param audio_facade: audio facade to check nodes.
        @param while_playback: whether to play when suspending.
        """
        if while_playback:
            logging.info('Playing audio served on the web...')
            web_file = audio_test_data.HEADPHONE_10MIN_TEST_FILE
            file_url = getattr(web_file, 'url', None)
            browser_facade = self.factory.create_browser_facade()
            tab_descriptor = browser_facade.new_tab(file_url)
            time.sleep(self.WEB_PLAYBACK_SEC)
        logging.info('Suspending...')
        boot_id = self.host.get_boot_id()
        self.host.suspend(suspend_time=self.SUSPEND_SEC)
        self.host.test_wait_for_resume(boot_id, self.CONNECT_TIMEOUT_SEC)
        logging.info('Resumed and back online.')
        time.sleep(self.WAIT_TO_REBIND_SEC)

        # Stop audio playback by closing the browser tab.
        if while_playback:
            browser_facade.close_tab(tab_descriptor)
        audio_test_utils.check_audio_nodes(audio_facade,
                                        (['HDMI'], None))


    def run_once(self, host, suspend=False, while_playback=False,
                 check_quality=False):
        """Running basic HDMI audio tests.

        @param host: device under test host
        @param suspend: whether to suspend
        @param while_playback: whether to suspend while audio playback
        @param check_quality: True to check quality.

        """
        golden_file = audio_test_data.FREQUENCY_TEST_FILE
        self.host = host

        # Dump audio diagnostics data for debugging.
        chameleon_board = host.chameleon
        self.factory = remote_facade_factory.RemoteFacadeFactory(
                host, results_dir=self.resultsdir)

        # For DUTs with permanently connected audio jack cable
        # connecting HDMI won't switch automatically the node. Adding
        # audio_jack_plugged flag to select HDMI node after binding.
        audio_facade = self.factory.create_audio_facade()
        output_nodes, _ = audio_facade.get_selected_node_types()
        audio_jack_plugged = False
        if output_nodes == ['HEADPHONE'] or output_nodes == ['LINEOUT']:
            audio_jack_plugged = True
            logging.debug('Found audio jack plugged!')

        self._system_facade = self.factory.create_system_facade()
        self.set_high_performance_mode()

        chameleon_board.setup_and_reset(self.outputdir)

        widget_factory = chameleon_audio_helper.AudioWidgetFactory(
                self.factory, host)

        source = widget_factory.create_widget(
            chameleon_audio_ids.CrosIds.HDMI)
        recorder = widget_factory.create_widget(
            chameleon_audio_ids.ChameleonIds.HDMI)
        binder = widget_factory.create_binder(source, recorder)

        with chameleon_audio_helper.bind_widgets(binder):
            audio_test_utils.dump_cros_audio_logs(
                    host, audio_facade, self.resultsdir, 'after_binding')

            # HDMI node needs to be selected, when audio jack is plugged
            if audio_jack_plugged:
                audio_facade.set_chrome_active_node_type('HDMI', None)
            audio_test_utils.check_audio_nodes(audio_facade,
                                               (['HDMI'], None))

            # Suspend after playing audio (if directed) and resume
            # before the HDMI audio test.
            if suspend:
                self.playback_and_suspend(audio_facade, while_playback)

            source.set_playback_data(golden_file)

            logging.info('Start recording from Chameleon.')
            recorder.start_recording()

            time.sleep(self.DELAY_BEFORE_PLAYBACK)

            logging.info('Start playing %s on Cros device',
                         golden_file.path)
            source.start_playback(blocking=True)

            logging.info('Stopped playing %s on Cros device',
                         golden_file.path)
            time.sleep(self.DELAY_AFTER_PLAYBACK)

            audio_test_utils.dump_cros_audio_logs(
                    host, audio_facade, self.resultsdir, 'after_recording')

            recorder.stop_recording()
            logging.info('Stopped recording from Chameleon.')
            recorder.read_recorded_binary()

            recorded_file = os.path.join(self.resultsdir, "recorded.raw")
            logging.info('Saving recorded data to %s', recorded_file)
            recorder.save_file(recorded_file)

            audio_test_utils.check_recorded_frequency(
                    golden_file, recorder, check_artifacts=check_quality)
