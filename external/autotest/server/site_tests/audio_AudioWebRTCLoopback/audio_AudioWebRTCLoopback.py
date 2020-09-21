# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side WebRTC audio test using the Chameleon board."""

import logging
import os
import time

from autotest_lib.client.cros.audio import audio_test_data
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import chameleon_audio_helper
from autotest_lib.client.cros.chameleon import chameleon_audio_ids
from autotest_lib.client.cros.multimedia import webrtc_utils
from autotest_lib.server.cros.audio import audio_test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class audio_AudioWebRTCLoopback(audio_test.AudioTest):
    """Server side WebRTC loopback audio test.

    This test talks to a Chameleon board and a Cros device to verify
    WebRTC audio function of the Cros device.
    A sine tone is played to Cros device from Chameleon USB.
    Through AppRTC loopback page, the sine tone is played to Chameleon
    LineIn from Cros device headphone.
    Using USB as audio source because it can provide completely correct
    data to loopback. This enables the test to do quality verification on
    headphone.

                  ----------->->->------------
         USB out |                            | USB in
           -----------                    --------              AppRTC loopback
          | Chameleon |                  |  Cros  |  <--------> webpage
           -----------                    --------
         Line-In |                            |  Headphone
                  -----------<-<-<------------


    The recorded audio is copied to server side and examined for quality.

    """
    version = 1
    RECORD_SECONDS = 10
    DELAY_AFTER_BINDING_SECONDS = 0.5

    def run_once(self, host, check_quality=False, chrome_block_size=None):
        """Running basic headphone audio tests.

        @param host: device under test host
        @param check_quality: flag to check audio quality.
        @param chrome_block_size: A number to be passed to Chrome
                                  --audio-buffer-size argument to specify
                                  block size.

        """
        if not audio_test_utils.has_headphone(host):
            logging.info('Skip the test because there is no headphone')
            return

        golden_file = audio_test_data.GenerateAudioTestData(
                data_format=dict(file_type='wav',
                                 sample_format='S16_LE',
                                 channel=2,
                                 rate=48000),
                path=os.path.join(self.bindir, 'fix_660_16.wav'),
                duration_secs=60,
                frequencies=[660, 660])

        chameleon_board = host.chameleon

        # Checks if a block size is specified for Chrome.
        extra_browser_args = None
        if chrome_block_size:
            extra_browser_args = ['--audio-buffer-size=%d' % chrome_block_size]

        factory = remote_facade_factory.RemoteFacadeFactory(
                host, results_dir=self.resultsdir,
                extra_browser_args=extra_browser_args)

        chameleon_board.setup_and_reset(self.outputdir)

        widget_factory = chameleon_audio_helper.AudioWidgetFactory(
                factory, host)

        headphone = widget_factory.create_widget(
            chameleon_audio_ids.CrosIds.HEADPHONE)
        linein = widget_factory.create_widget(
            chameleon_audio_ids.ChameleonIds.LINEIN)
        headphone_linein_binder = widget_factory.create_binder(headphone, linein)

        usb_out = widget_factory.create_widget(chameleon_audio_ids.ChameleonIds.USBOUT)
        usb_in = widget_factory.create_widget(chameleon_audio_ids.CrosIds.USBIN)
        usb_binder = widget_factory.create_binder(usb_out, usb_in)

        with chameleon_audio_helper.bind_widgets(headphone_linein_binder):
            with chameleon_audio_helper.bind_widgets(usb_binder):
                time.sleep(self.DELAY_AFTER_BINDING_SECONDS)
                audio_facade = factory.create_audio_facade()

                audio_test_utils.dump_cros_audio_logs(
                        host, audio_facade, self.resultsdir, 'after_binding')

                # Checks whether line-out or headphone is detected.
                hp_jack_node_type = audio_test_utils.check_hp_or_lineout_plugged(
                        audio_facade)

                # Checks headphone and USB nodes are plugged.
                # Let Chrome select the proper I/O nodes.
                # Input is USB, output is headphone.
                audio_test_utils.check_and_set_chrome_active_node_types(
                        audio_facade=audio_facade,
                        output_type=hp_jack_node_type,
                        input_type='USB')

                logging.info('Setting playback data on Chameleon')
                usb_out.set_playback_data(golden_file)

                browser_facade = factory.create_browser_facade()
                apprtc = webrtc_utils.AppRTCController(browser_facade)
                logging.info('Load AppRTC loopback webpage')
                apprtc.new_apprtc_loopback_page()

                logging.info('Start recording from Chameleon.')
                linein.start_recording()

                logging.info('Start playing %s on Cros device',
                        golden_file.path)
                usb_out.start_playback()

                time.sleep(self.RECORD_SECONDS)

                linein.stop_recording()
                logging.info('Stopped recording from Chameleon.')

                audio_test_utils.dump_cros_audio_logs(
                        host, audio_facade, self.resultsdir, 'after_recording')

                usb_out.stop_playback()

                linein.read_recorded_binary()
                logging.info('Read recorded binary from Chameleon.')

        golden_file.delete()

        recorded_file = os.path.join(self.resultsdir, "recorded.raw")
        logging.info('Saving recorded data to %s', recorded_file)
        linein.save_file(recorded_file)

        diagnostic_path = os.path.join(
                self.resultsdir,
                'audio_diagnostics.txt.after_recording')
        logging.info('Examine diagnostic file at %s', diagnostic_path)
        diag_warning_msg = audio_test_utils.examine_audio_diagnostics(
                diagnostic_path)
        if diag_warning_msg:
            logging.warning(diag_warning_msg)

        # Raise error.TestFail if there is issue.
        audio_test_utils.check_recorded_frequency(
                golden_file, linein, check_artifacts=check_quality)
