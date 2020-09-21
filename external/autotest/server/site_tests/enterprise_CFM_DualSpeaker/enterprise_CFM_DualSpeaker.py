# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import random
import re
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import power_cycle_usb_util
from autotest_lib.client.common_lib.cros.cfm import cras_node_collector
from autotest_lib.client.common_lib.cros.cfm.usb import cfm_usb_devices
from autotest_lib.client.common_lib.cros.cfm.usb import usb_device_collector
from autotest_lib.server.cros.cfm import cfm_base_test


# CFMs have a base volume level threshold. Setting the level below 2
# is interpreted by the CFM as 0.
CFM_VOLUME_LEVEL_LOWER_LIMIT = 2
CFM_VOLUME_LEVEL_UPPER_LIMIT = 100
JABRA = cfm_usb_devices.JABRA_SPEAK_410
DUAL_SPEAKER_DEVICE_NAME = JABRA.product
TIMEOUT_SECS  = 10


class enterprise_CFM_DualSpeaker(cfm_base_test.CfmBaseTest):
    """
    Tests that the following functionality works on CfM enrolled devices:

       1. Mixer mute/umute state should be in sync between CfM and 2 speakers.
       2. Volume of two speakers should be in sync with the volume set by CfM.
       3. When muting/unmuting speakers from CfM, #1 still holds.
       4. When changing volume from CfM, #2 still holds.
       5. After disconnect/re-connecting any speaker #1-4 still holds.
    """
    version = 1


    def _get_cras_jabra_speaker_nodes(self):
        """
        Gets jabra speaker nodes.

        @returns A list cras input nodes representing Jabra speakers.
        """
        nodes = self.cras_collector.get_output_nodes()
        return [n for n in nodes if
                DUAL_SPEAKER_DEVICE_NAME in n.device_name]

    def _get_amixer_jabra_mic_node_ids(self):
        """
        Gets jabra mixer (microphone) card IDs from arecord.

        @returns A list of mixer card IDs or [] if no mixer is found.
        """
        cmd = ("arecord -l"
               " | grep \"%s\""
               " | awk -v N=2 '{print $N}'" % DUAL_SPEAKER_DEVICE_NAME)
        mixer_cards = [s.strip().split(':')[0] for s in
                       self._host.run_output(cmd).splitlines()]
        return mixer_cards

    def _get_cras_default_speakers(self):
        """
        Gets the default speakers from cras_test_client.

        @returns A list of speaker nodes.
        """
        nodes = self.cras_collector.get_output_nodes()
        return [n for n in nodes if '*(default)' in n.node_name]

    def _get_cras_default_mixers(self):
        """
        Gets the default mixers from cras_test_client.

        @returns A list of speaker node IDs or [] if no device is found.
        """
        nodes = self.cras_collector.get_input_nodes()
        return [n for n in nodes if n.node_name.startswith('*')]

    def _get_cras_jabra_mixer_nodes(self):
        """
        Gets the mixer nodes from cras_test_client.

        @returns A list of mixer node IDs or [] if no device is found.
        """
        nodes = self.cras_collector.get_input_nodes()
        return [n for n in nodes if DUAL_SPEAKER_DEVICE_NAME in n.node_name]

    def _get_cras_speaker_volume(self, node_id):
        """
        Gets the speaker volume for a node from cras_test_client.

        @param node_id: the node ID to query.
        @returns the volume of speaker.
        """
        for node in self.cras_collector.get_output_nodes():
            if node.node_id == node_id:
                return node.volume

    def _get_mixer_mute_state(self, node_id):
        """
        Gets the speaker mute state from cras_test_client.

        @param node_id: the node to query.
        @returns True if speakers is muted, otherwise False.
        """
        cmd = ("amixer -c %s"
               " | grep \"Mono: Capture\""
               " | awk -v N=6 '{print $N}'" % node_id)
        mute = [s.strip() for s in
                self._host.run_output(cmd).splitlines()][0]

        if re.search(r"\[(off)\]", mute):
            return  True
        else:
            return False


    def _test_volume_sync_for_dual_speakers(self):
        """Checks whether volume is synced between dual speakers and CfM."""
        cfm_volume = self.cfm_facade.get_speaker_volume()
        # There is case where cfm_facade.get_speaker_volume() returns empty
        # string. Script polls volume from App up to 15 seconds or until
        # non-zero value is returned.
        poll_time = 0
        while not cfm_volume and poll_time < TIMEOUT_SECS:
            cfm_volume = self.cfm_facade.get_speaker_volume()
            time.sleep(1)
            logging.info('Checking volume set by App on CfM: %s', cfm_volume)
            poll_time += 1
        if not cfm_volume:
            logging.info('Volume returned from App is Null')
            return False
        nodes = self._get_cras_default_speakers()
        for node in nodes:
            cras_volume =  self._get_cras_speaker_volume(node.node_id)
            logging.info('Volume in CfM and cras are sync for '
                    'node %s? cfm: %s, cras: %s',
                     str(node), cfm_volume, cras_volume)
            if int(cfm_volume) != int(cras_volume):
                logging.error('Test _test_volume_sync_for_dual_speakers fails'
                              ' for node %s', node.device_name)
                return False
        return True


    def _test_mute_state_sync_for_dual_speakers(self):
        """Checks whether mute/unmute is sync between dual speakers and CfM."""
        cfm_mute = self.cfm_facade.is_mic_muted()
        if cfm_mute:
            logging.info('Mixer is muted from CfM.')
        else:
            logging.info('Mixer is not muted from CfM.')

        nodes = self._get_amixer_jabra_mic_node_ids()
        for node_id in nodes:
            amixer_mute =  self._get_mixer_mute_state(node_id)
            if amixer_mute:
                logging.info('amixer shows mic is muted for node %s.', node_id)
            else:
                logging.info('amixer shows mix not muted for node %s', node_id)
            if not cfm_mute == amixer_mute:
                logging.error('Test _test_mute_state_sync_for_dual_speakers '
                              'fails for node %s', node_id)
                return False
        return True


    def _has_dual_speakers(self):
        """
        Checks if there are dual speakers connected to the DUT.

        @returns True if there are dual speakers, false otherwise.
        """
        collector = usb_device_collector.UsbDeviceCollector(self._host)
        speakers = collector.get_devices_by_spec(JABRA)
        return len(speakers) == 2

    def _set_preferred_speaker(self, speaker_name):
        """Set preferred speaker to Dual speaker."""
        logging.info('CfM sets preferred speaker to %s.', speaker_name)
        self.cfm_facade.set_preferred_speaker(speaker_name)
        time.sleep(TIMEOUT_SECS)
        current_prefered_speaker = self.cfm_facade.get_preferred_speaker()
        logging.info('Prefered speaker set to %s', current_prefered_speaker)
        if speaker_name != current_prefered_speaker:
            raise error.TestFail('Failed to set prefered speaker! '
                                 'Expected %s, got %s',
                                 speaker_name, current_prefered_speaker)


    def _set_preferred_mixer(self, mixer_name):
        """Set preferred mixer/microphone to Dual speaker."""
        logging.info('CfM sets preferred mixer to %s.', mixer_name)
        self.cfm_facade.set_preferred_mic(mixer_name)
        time.sleep(TIMEOUT_SECS)
        current_prefered_mixer = self.cfm_facade.get_preferred_speaker()
        logging.info('Prefered mixer set to %s by CfM.', current_prefered_mixer)
        if mixer_name != current_prefered_mixer:
            raise error.TestFail('Failed to set prefered mixer! '
                                 'Expected %s, got %s',
                                 mixer_name, current_prefered_mixer)


    def  _set_preferred_speaker_mixer(self):
        """Sets preferred speaker and mixer to Dual speaker."""
        self._set_preferred_speaker(DUAL_SPEAKER_DEVICE_NAME)
        self._set_preferred_mixer(DUAL_SPEAKER_DEVICE_NAME)
        time.sleep(TIMEOUT_SECS)
        default_speaker_ids = set([
            n.node_id for n in self._get_cras_default_speakers()])
        cras_speaker_ids = set([
            n.node_id for n in self._get_cras_jabra_speaker_nodes()])
        if default_speaker_ids != cras_speaker_ids:
            raise error.TestFail('Dual speakers not set to preferred speaker '
                                 'dual=%s; cras=%s' % (default_speaker_ids,
                                                       cras_speaker_ids))
        default_mixer_ids = set([
            n.node_id for n in self._get_cras_default_mixers()])
        jabra_mixer_ids = set([
            n.node_id for n in self._get_cras_jabra_mixer_nodes()])
        if default_mixer_ids != jabra_mixer_ids:
            raise error.TestFail('Dual mixs is not set to preferred speaker.')


    def _test_dual_speaker_sanity(self):
        """
        Performs a speaker sanity check:
            1. Checks whether volume is sync between dual speakers and CfM
            2. Checks whether mute/unmute is sync between dual speakers and CfM.
        @returns True if passed, otherwise False.
        """
        volume = self._test_volume_sync_for_dual_speakers()
        mute = self._test_mute_state_sync_for_dual_speakers()
        return volume and mute


    def _test_mute_sync(self):
        """
        Mutes and unmutes speaker from CfM.
        Check whether mute/unmute is sync between dual speakers and CfM.
        @returns True if yes, else retrun False.
        """
        self.cfm_facade.mute_mic()
        if not self.cfm_facade.is_mic_muted():
            raise error.TestFail('CFM fails to mute mic')
        time.sleep(TIMEOUT_SECS)
        muted = self._test_mute_state_sync_for_dual_speakers()
        self.cfm_facade.unmute_mic()
        if self.cfm_facade.is_mic_muted():
            raise error.TestFail('CFM fails to unmute mic')
        time.sleep(TIMEOUT_SECS)
        unmuted = self._test_mute_state_sync_for_dual_speakers()
        return muted and unmuted

    def _test_volume_sync(self):
        """
        Changes speaker volume from CfM.
        Checks whether volume is sync between dual speakers and CfM.
        @returns True if check succeeds, otherwise False.
        """
        test_volume = random.randrange(CFM_VOLUME_LEVEL_LOWER_LIMIT,
                                       CFM_VOLUME_LEVEL_UPPER_LIMIT)
        self.cfm_facade.set_speaker_volume(str(test_volume))
        time.sleep(TIMEOUT_SECS)
        return self._test_volume_sync_for_dual_speakers()


    def run_once(self):
        """Runs the test."""
        logging.info('Sanity check and initilization:')
        if not self._has_dual_speakers():
            raise error.TestFail('No dual speakers found on DUT.')

        self.cras_collector = cras_node_collector.CrasNodeCollector(self._host)

        # Remove 'board:' prefix.
        board_name = self._host.get_board().split(':')[1]
        gpio_list = power_cycle_usb_util.get_target_all_gpio(
            self._host, board_name, JABRA.vendor_id,
            JABRA.product_id)
        if len(set(gpio_list)) == 1:
            raise error.TestFail('Speakers have to be tied to different GPIO.')

        self.cfm_facade.wait_for_hangouts_telemetry_commands()

        self._set_preferred_speaker_mixer()
        logging.info('1. Check CfM and dual speakers have the same setting '
                     'after joining meeting:')
        self.cfm_facade.start_new_hangout_session('test_cfm_dual_speaker')
        if not self._test_dual_speaker_sanity():
            raise error.TestFail('Dual speaker Sanity verification fails.')

        logging.info('1.1 Check CfM and dual microphones have the same '
                     'setting for Mute/unmute:')
        if not self._test_mute_sync():
            raise error.TestFail('Dual speaker Mute-unmute test'
                    ' verification fails')

        logging.info('1.2 Check CfM and dual Speakers have same volume: ')
        if not self._test_volume_sync():
            raise error.TestFail('Dual speaker volume test verification fails')

        for gpio in gpio_list:

            logging.info('2. Check CfM and speakers have the same setting '
                         'after flapping speaker:')
            logging.info('Power cycle one of Speaker %s', gpio)
            power_cycle_usb_util.power_cycle_usb_gpio(self._host,
                    gpio, TIMEOUT_SECS)
            time.sleep(TIMEOUT_SECS)

            logging.info('2.1. Check CfM and dual speakers have same setting')
            if not self._test_dual_speaker_sanity():
                raise error.TestFail('Dual speaker Sanity verification'
                        ' fails after disconnect/reconnect speaker')

            logging.info('2.1.1. Check CfM and microphones have same setting '
                         'for Mute/unmute:')
            if not self._test_mute_sync():
                raise error.TestFail(
                    'Dual speaker Mute-unmute test verification fails after '
                    'disconnect/reconnect speaker')

            logging.info('2.1.2. Check CfM and dual Speakers have same volume:')
            if not self._test_volume_sync():
                raise error.TestFail('Dual speaker volume test verification'
                        ' fails after disconnect/reconnect speaker')

        self.cfm_facade.end_hangout_session()
