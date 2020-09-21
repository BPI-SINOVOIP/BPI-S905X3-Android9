# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.cfm import cras_node_collector
from autotest_lib.client.cros.chameleon import motor_board
from autotest_lib.server.cros.cfm import cfm_base_test


class enterprise_CFM_USBSpeakerEndToEndSanity(cfm_base_test.CfmBaseTest):
    """Tests the USB speaker buttons work properly."""
    version = 1

    def initialize(self, host, usb_speaker):
        """
        Initializes common test properties.

        @param host: a host object representing the DUT.
        @param usb_speaker: The speaker to test.
        """
        super(enterprise_CFM_USBSpeakerEndToEndSanity, self).initialize(host)
        self.motor = self._host.chameleon.get_motor_board()
        self.cras_collector = cras_node_collector.CrasNodeCollector(self._host)
        self.usb_speaker = usb_speaker

    def _get_cras_output_node_volumes(self, speaker_name):
        """
        Gets the volume values for all output nodes with the specified name.

        @param speaker_name: The name of the speaker output node.
        @returns A list volume values.
        """
        nodes = self.cras_collector.get_output_nodes()
        return [n.volume for n in nodes if speaker_name in n.device_name]

    def test_increase_volume_button_on_speaker(self):
        """Tests that pressing the VOL_UP button increases the volume."""
        old_cras_volumes = self._get_cras_output_node_volumes(
            self.usb_speaker.product)

        self.motor.Touch(motor_board.ButtonFunction.VOL_UP)
        self.motor.Release(motor_board.ButtonFunction.VOL_UP)

        new_cras_volumes = self._get_cras_output_node_volumes(
            self.usb_speaker.product)

        # List comparison is done elemenet by element.
        if not new_cras_volumes > old_cras_volumes:
            raise error.TestFail('Speaker volume increase not reflected in '
                                 'cras volume output. Volume before button '
                                 'press: %s; volume after button press: %s.'
                                 % (old_cras_volumes, new_cras_volumes))

    def test_decrease_volume_button_on_speaker(self):
        """Tests that pressing the VOL_DOWN button decreases the volume."""
        old_cras_volumes = self._get_cras_output_node_volumes(
            self.usb_speaker.product)

        self.motor.Touch(motor_board.ButtonFunction.VOL_DOWN)
        self.motor.Release(motor_board.ButtonFunction.VOL_DOWN)

        new_cras_volumes = self._get_cras_output_node_volumes(
            self.usb_speaker.product)

        # List comparison is done elemenet by element.
        if not new_cras_volumes < old_cras_volumes:
            raise error.TestFail('Speaker volume decrease not reflected in '
                                 'cras volume output. Volume before button '
                                 'press: %s; volume after button press: %s.'
                                 % (old_cras_volumes, new_cras_volumes))

    def test_call_hangup_button_on_speaker(self):
        """Tests the HANG_UP button works."""
        self.motor.Touch(motor_board.ButtonFunction.HANG_UP)
        self.motor.Release(motor_board.ButtonFunction.HANG_UP)

    def test_call_button_on_speaker(self):
        """Tests the CALL button works."""
        self.motor.Touch(motor_board.ButtonFunction.CALL)
        self.motor.Release(motor_board.ButtonFunction.CALL)

    def test_mute_button_on_speaker(self):
        """Tests the MUTE button works."""
        self.motor.Touch(motor_board.ButtonFunction.MUTE)
        self.motor.Release(motor_board.ButtonFunction.MUTE)

    def run_once(self):
        """Runs the test."""
        self.test_increase_volume_button_on_speaker()
        self.test_decrease_volume_button_on_speaker()
        self.test_call_hangup_button_on_speaker()
        self.test_call_button_on_speaker()
        self.test_mute_button_on_speaker()
