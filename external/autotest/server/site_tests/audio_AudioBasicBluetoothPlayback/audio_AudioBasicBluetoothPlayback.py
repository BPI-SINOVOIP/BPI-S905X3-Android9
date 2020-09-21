# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side bluetooth playback test using the Chameleon board."""

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


class audio_AudioBasicBluetoothPlayback(audio_test.AudioTest):
    """Server side bluetooth playback audio test.

    This test talks to a Chameleon board and a Cros device to verify
    bluetooth playback audio function of the Cros device.

    """
    version = 1
    DELAY_AFTER_DISABLING_MODULE_SECONDS = 30
    DELAY_AFTER_DISCONNECT_SECONDS = 5
    DELAY_AFTER_ENABLING_MODULE_SECONDS = 10
    DELAY_AFTER_RECONNECT_SECONDS = 5
    DELAY_BEFORE_RECORD_SECONDS = 0.5
    RECORD_SECONDS = 5
    SUSPEND_SECONDS = 30
    RESUME_TIMEOUT_SECS = 60
    PRC_RECONNECT_TIMEOUT = 60
    BLUETOOTH_RECONNECT_TIMEOUT_SECS = 30
    DELAY_FOR_A2DP_RECONNECT_AFTER_SUSPEND = 10

    def disconnect_connect_bt(self, link):
        """Performs disconnect and connect BT module

        @param link: binder link to control BT adapter

        """

        logging.info("Disconnecting BT module...")
        link.adapter_disconnect_module()
        time.sleep(self.DELAY_AFTER_DISCONNECT_SECONDS)
        audio_test_utils.check_audio_nodes(self.audio_facade,
                                           (['INTERNAL_SPEAKER'], None))
        logging.info("Connecting BT module...")
        link.adapter_connect_module()
        time.sleep(self.DELAY_AFTER_RECONNECT_SECONDS)


    def disable_enable_bt(self, link):
        """Performs turn off and then on BT module

        @param link: binder link to control BT adapter

        """

        logging.info("Turning off BT module...")
        link.disable_bluetooth_module()
        time.sleep(self.DELAY_AFTER_DISABLING_MODULE_SECONDS)
        audio_test_utils.check_audio_nodes(self.audio_facade,
                                           (['INTERNAL_SPEAKER'], None))
        logging.info("Turning on BT module...")
        link.enable_bluetooth_module()
        time.sleep(self.DELAY_AFTER_ENABLING_MODULE_SECONDS)
        logging.info("Connecting BT module...")
        link.adapter_connect_module()
        time.sleep(self.DELAY_AFTER_RECONNECT_SECONDS)


    def bluetooth_nodes_plugged(self):
        """Checks if bluetooth nodes are plugged.

        @returns: True if bluetooth nodes are plugged. False otherwise.

        """
        return audio_test_utils.bluetooth_nodes_plugged(self.audio_facade)


    def dump_logs_after_nodes_changed(self):
        """Dumps the log after unexpected NodesChanged signal happens."""
        audio_test_utils.dump_cros_audio_logs(
                self.host, self.audio_facade, self.resultsdir,
                'after_nodes_changed')


    def run_once(self, host, suspend=False,
                 disable=False, disconnect=False, check_quality=False):
        """Running Bluetooth basic audio tests

        @param host: device under test host
        @param suspend: suspend flag to enable suspend before play/record
        @param disable: disable flag to disable BT module before play/record
        @param disconnect: disconnect flag to disconnect BT module
            before play/record
        @param check_quality: flag to check audio quality.

        """
        self.host = host
        golden_file = audio_test_data.FREQUENCY_TEST_FILE

        factory = remote_facade_factory.RemoteFacadeFactory(
                host, results_dir=self.resultsdir)
        self.audio_facade = factory.create_audio_facade()

        chameleon_board = host.chameleon
        chameleon_board.setup_and_reset(self.outputdir)

        widget_factory = chameleon_audio_helper.AudioWidgetFactory(
                factory, host)

        source = widget_factory.create_widget(
            chameleon_audio_ids.CrosIds.BLUETOOTH_HEADPHONE)
        bluetooth_widget = widget_factory.create_widget(
            chameleon_audio_ids.PeripheralIds.BLUETOOTH_DATA_RX)
        recorder = widget_factory.create_widget(
            chameleon_audio_ids.ChameleonIds.LINEIN)

        binder = widget_factory.create_binder(
                source, bluetooth_widget, recorder)

        with chameleon_audio_helper.bind_widgets(binder):

            audio_test_utils.dump_cros_audio_logs(
                    host, self.audio_facade, self.resultsdir, 'after_binding')

            # For DUTs with permanently connected audio jack cable
            # Bluetooth output node should be selected explicitly.
            output_nodes, _ = self.audio_facade.get_plugged_node_types()
            if 'HEADPHONE' in output_nodes or 'LINEOUT' in output_nodes:
                self.audio_facade.set_chrome_active_node_type('BLUETOOTH',
                                                              None)
            # Checks the node selected by Cras is correct.
            audio_test_utils.check_audio_nodes(self.audio_facade,
                                               (['BLUETOOTH'], None))

            self.audio_facade.set_selected_output_volume(80)

            # Starts playing, waits for some time, and then starts recording.
            # This is to avoid artifact caused by codec initialization.
            source.set_playback_data(golden_file)

            # Create link to control BT adapter.
            link = binder.get_binders()[0].get_link()

            if disable:
                self.disable_enable_bt(link)
            if disconnect:
                self.disconnect_connect_bt(link)
            if suspend:
                audio_test_utils.suspend_resume(host, self.SUSPEND_SECONDS)

            utils.poll_for_condition(condition=factory.ready,
                                     timeout=self.PRC_RECONNECT_TIMEOUT,
                                     desc='multimedia server reconnect')

            if disable or disconnect or suspend:
                audio_test_utils.dump_cros_audio_logs(
                        host, self.audio_facade, self.resultsdir,
                        'after_action')

            # Gives DUT some time to auto-reconnect bluetooth after resume.
            if suspend:
                utils.poll_for_condition(
                        condition=self.bluetooth_nodes_plugged,
                        timeout=self.BLUETOOTH_RECONNECT_TIMEOUT_SECS,
                        desc='bluetooth node auto-reconnect after suspend')
                # Delay some time for A2DP to be reconnected.
                # Normally this happens several seconds after HSP is
                # reconnected. However, there is no event what we can wait for,
                # so just wait some time.
                time.sleep(self.DELAY_FOR_A2DP_RECONNECT_AFTER_SUSPEND)

            with audio_test_utils.monitor_no_nodes_changed(
                    self.audio_facade, self.dump_logs_after_nodes_changed):
                # Checks the node selected by Cras is correct again.
                audio_test_utils.check_audio_nodes(self.audio_facade,
                                                   (['BLUETOOTH'], None))

                logging.info('Start playing %s on Cros device',
                             golden_file.path)

                source.start_playback()

                time.sleep(self.DELAY_BEFORE_RECORD_SECONDS)
                logging.info('Start recording from Chameleon.')
                recorder.start_recording()

                time.sleep(self.RECORD_SECONDS)

                recorder.stop_recording()
                logging.info('Stopped recording from Chameleon.')

                audio_test_utils.dump_cros_audio_logs(
                        host, self.audio_facade, self.resultsdir,
                        'after_recording')

                recorder.read_recorded_binary()
                logging.info('Read recorded binary from Chameleon.')

                recorded_file = os.path.join(self.resultsdir, "recorded.raw")
                logging.info('Saving recorded data to %s', recorded_file)
                recorder.save_file(recorded_file)

        # Removes the beginning of recorded data. This is to avoid artifact
        # caused by Chameleon codec initialization in the beginning of
        # recording.
        recorder.remove_head(0.5)

        recorded_file = os.path.join(self.resultsdir, "recorded_clipped.raw")
        logging.info('Saving clipped data to %s', recorded_file)
        recorder.save_file(recorded_file)

        # Compares data by frequency. Audio signal recorded by microphone has
        # gone through analog processing and through the air.
        # This suffers from codec artifacts and noise on the path.
        # Comparing data by frequency is more robust than comparing by
        # correlation, which is suitable for fully-digital audio path like USB
        # and HDMI.
        audio_test_utils.check_recorded_frequency(
                golden_file, recorder, check_anomaly=check_quality)
