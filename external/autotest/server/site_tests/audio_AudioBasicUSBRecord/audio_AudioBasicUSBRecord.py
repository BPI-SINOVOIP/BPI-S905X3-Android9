# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side USB playback audio test using the Chameleon board."""

import logging
import os
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.cros.audio import audio_test_data
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import chameleon_audio_helper
from autotest_lib.client.cros.chameleon import chameleon_audio_ids
from autotest_lib.server.cros.audio import audio_test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class audio_AudioBasicUSBRecord(audio_test.AudioTest):
    """Server side USB capture audio test.

    This test talks to a Chameleon board and a Cros device to verify
    USB audio record function of the Cros device.

    """
    version = 1
    RECORD_SECONDS = 5
    SUSPEND_SECONDS = 30
    RPC_RECONNECT_TIMEOUT = 60

    def run_once(self, host, suspend=False):
        golden_file = audio_test_data.SWEEP_TEST_FILE

        chameleon_board = host.chameleon
        factory = remote_facade_factory.RemoteFacadeFactory(
                host, results_dir=self.resultsdir)

        chameleon_board.setup_and_reset(self.outputdir)

        widget_factory = chameleon_audio_helper.AudioWidgetFactory(
                factory, host)

        source = widget_factory.create_widget(
            chameleon_audio_ids.ChameleonIds.USBOUT)
        recorder = widget_factory.create_widget(
            chameleon_audio_ids.CrosIds.USBIN)
        binder = widget_factory.create_binder(source, recorder)

        with chameleon_audio_helper.bind_widgets(binder):
            # Checks the node selected by cras is correct.
            audio_facade = factory.create_audio_facade()

            audio_test_utils.dump_cros_audio_logs(
                    host, audio_facade, self.resultsdir, 'after_binding')

            audio_test_utils.check_audio_nodes(audio_facade,
                                               (None, ['USB']))
            logging.info('Setting playback data on Cros device')

            audio_facade.set_selected_output_volume(70)

            source.set_playback_data(golden_file)

            if suspend:
                audio_test_utils.suspend_resume(host, self.SUSPEND_SECONDS)
                utils.poll_for_condition(condition=factory.ready,
                                         timeout=self.RPC_RECONNECT_TIMEOUT,
                                         desc='multimedia server reconnect')
                audio_test_utils.check_audio_nodes(audio_facade,
                                                   (None, ['USB']))

            # Starts playing from Chameleon (which waits for Cros device),
            # waits for some time, and then starts recording from Cros device.
            logging.info('Start playing %s on Chameleon device',
                         golden_file.path)
            source.start_playback()

            logging.info('Start recording from Cros.')
            recorder.start_recording()

            time.sleep(self.RECORD_SECONDS)

            recorder.stop_recording()
            logging.info('Stopped recording from Cros.')

            audio_test_utils.dump_cros_audio_logs(
                    host, audio_facade, self.resultsdir, 'after_recording')

            recorder.read_recorded_binary()
            logging.info('Read recorded binary from Cros.')

        recorded_file = os.path.join(self.resultsdir, "recorded.raw")
        logging.info('Saving recorded data to %s', recorded_file)
        recorder.save_file(recorded_file)

        audio_test_utils.compare_recorded_correlation(golden_file, recorder)
