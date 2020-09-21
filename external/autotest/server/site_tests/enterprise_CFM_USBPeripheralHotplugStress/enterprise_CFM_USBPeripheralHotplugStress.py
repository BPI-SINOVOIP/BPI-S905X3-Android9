# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.cfm import cfm_base_test

_SHORT_TIMEOUT = 5

_CAMERA = 'Camera'
_MICROPHONE = 'Microphone'
_SPEAKER = 'Speaker'


class enterprise_CFM_USBPeripheralHotplugStress(cfm_base_test.CfmBaseTest):
    """
    Uses servo to hotplug and unplug USB peripherals multiple times and
    verify's that the hotrod app appropriately detects the peripherals using
    app api's.
    """
    version = 1


    def _set_hub_power(self, on=True):
        """
        Setting USB hub power status

        @param on: To power on the servo usb hub or not.
        """
        # To power on the hub means to turn reset off.
        if on:
            self._host.servo.set('dut_hub1_rst1', 'off')
        else:
            self._host.servo.set('dut_hub1_rst1', 'on')
        time.sleep(_SHORT_TIMEOUT)


    def _set_preferred_peripheral(self, peripheral_dict):
        """
        Set perferred peripherals.

        @param peripheral_dict: Dictionary of peripherals.
        """
        avail_mics = self.cfm_facade.get_mic_devices()
        avail_speakers = self.cfm_facade.get_speaker_devices()
        avail_cameras = self.cfm_facade.get_camera_devices()

        if peripheral_dict.get(_MICROPHONE) in avail_mics:
            self.cfm_facade.set_preferred_mic(
                    peripheral_dict.get(_MICROPHONE))
        if peripheral_dict.get(_SPEAKER) in avail_speakers:
            self.cfm_facade.set_preferred_speaker(
                    peripheral_dict.get(_SPEAKER))
        if peripheral_dict.get(_CAMERA) in avail_cameras:
            self.cfm_facade.set_preferred_camera(
                    peripheral_dict.get(_CAMERA))


    def _check_peripheral(self, device_type, hub_on, peripheral_dict,
                          get_preferred, get_all):
        """
        Checks a connected peripheral depending on the usb hub state.

        @param device_type: The type of the peripheral.
        @param hub_on: wheter the USB hub is on or off.
        @param peripheral_dict: A dictionary of connected peripherals, keyed
            by type.
        @param get_preferred: Function that gets the prefered device for the
            specified type. Prefered means the CfM selects that device as active
            even if other devices of the same type are conncted to it
            (e.g. multiple cameras).
        @param get_all: Function that gets all conncted devices for the
            specified type.
        """
        device_name = peripheral_dict.get(device_type)
        prefered_peripheral = get_preferred()
        avail_devices = get_all()

        if hub_on:
            if device_name != prefered_peripheral:
                raise error.TestFail('%s not detected.' % device_type)
        else:
            if device_name == prefered_peripheral:
                raise error.TestFail('%s should not be detected.' % device_type)

        if avail_devices:
            if prefered_peripheral is None:
                raise error.TestFail('Available %s not selected.' % device_type)
            if ((not hub_on and device_name != prefered_peripheral) and
                (prefered_peripheral not in avail_devices)):
                raise error.TestFail('Available %s not selected.' % device_type)

        if hub_on:
            logging.info("[SUCCESS] %s has been detected.", device_type)
        else:
            logging.info("[SUCCESS] %s has not been detected.", device_type)


    def _check_peripherals(self, peripheral_dict, hub_on):
        """
        Sets the hub power and verifies the visibility of peripherals.

        @param peripheral_dict: Dictionary of peripherals to check.
        @param hub_on: To turn the USB hub on or off.
        """
        self._set_hub_power(hub_on)

        if _MICROPHONE in peripheral_dict:
            self._check_peripheral(
                _MICROPHONE,
                hub_on,
                peripheral_dict,
                self.cfm_facade.get_preferred_mic,
                self.cfm_facade.get_mic_devices)

        if _SPEAKER in peripheral_dict:
            self._check_peripheral(
                _SPEAKER,
                hub_on,
                peripheral_dict,
                self.cfm_facade.get_preferred_speaker,
                self.cfm_facade.get_speaker_devices)

        if _CAMERA in peripheral_dict:
            self._check_peripheral(
                _CAMERA,
                hub_on,
                peripheral_dict,
                self.cfm_facade.get_preferred_camera,
                self.cfm_facade.get_camera_devices)


    def run_once(self, repeat, peripheral_whitelist_dict):
        """
        Main function to run autotest.

        @param repeat: Number of times peripheral should be hotplugged.
        @param peripheral_whitelist_dict: Dictionary of peripherals to test.
        """
        self.cfm_facade.wait_for_hangouts_telemetry_commands()
        self._set_preferred_peripheral(peripheral_whitelist_dict)
        for _ in xrange(repeat):
            # Plug out.
            self._check_peripherals(peripheral_whitelist_dict, False)
            # Plug in.
            self._check_peripherals(peripheral_whitelist_dict, True)
