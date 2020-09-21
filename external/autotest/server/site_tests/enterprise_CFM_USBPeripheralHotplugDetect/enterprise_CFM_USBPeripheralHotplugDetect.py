# Copyright (c) 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import crash_detector
from autotest_lib.client.common_lib.cros import tpm_utils
from autotest_lib.client.common_lib.cros.cfm.usb import cfm_usb_devices
from autotest_lib.client.common_lib.cros.cfm.usb import usb_device_collector
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory

_SHORT_TIMEOUT = 5

# Constants for Hangouts UI peripherals names. Each entry is a tuple in the form
# of (<hangouts_name_regex>, <USB device spec>). We use regular expressions for
# the labels since these are not constant values, i.e. they depend on which USB
# port a device has been plugged in to.
# TODO(dtosic): Move this out to a separate util so other tests can reuse it.
_HANGOUTS_UI_CAMERA_NAMES = [
    (r'HD Pro Webcam C920 \(046d:082d\)', cfm_usb_devices.HD_PRO_WEBCAM_C920),
    (r'Logitech Webcam C930e \(046d:0843\)',
        cfm_usb_devices.LOGITECH_WEBCAM_C930E),
]

_HANGOUTS_UI_MICROPHONE_NAMES = [
    (r'Jabra SPEAK 410', cfm_usb_devices.JABRA_SPEAK_410),
    # Some cameras also have integrated microphones.
    (r'HD Pro Webcam C920: USB Audio:[0-9],[0-9]',
        cfm_usb_devices.HD_PRO_WEBCAM_C920),
    (r'Logitech Webcam C930e: USB Audio:[0-9],[0-9]',
        cfm_usb_devices.LOGITECH_WEBCAM_C930E),
]

_HANGOUTS_UI_SPEAKER_NAMES = [
    (r'Jabra SPEAK 410', cfm_usb_devices.JABRA_SPEAK_410),
]

def _get_usb_spec_for_hangouts_device_name(device_name, name_to_spec_map):
    """Returns a USB spec (if found) for the specified name, or None."""
    for name_regex, usb_spec in name_to_spec_map:
        if re.match(name_regex, device_name):
            return usb_spec
    return None


class _Peripherals(object):
    """Utility class for storing peripherals names by type."""

    def __init__(self):
        self._dict = {
            'Microphone': [],
            'Speaker': [],
            'Camera':[]
        }

    def add_mic(self, spec):
        """Registers a mic name."""
        self._add_device('Microphone', spec)

    def add_speaker(self, spec):
        """Registers a speaker name."""
        self._add_device('Speaker', spec)

    def add_camera(self, spec):
        """Registers a camera name."""
        self._add_device('Camera', spec)

    def _add_device(self, type, spec):
        self._dict[type].append(str(spec))

    def _get_by_type(self, type):
        return self._dict[type]

    def get_diff(self, other_peripherals):
        """Returns a diff dictonary for other_peripherals."""
        peripherals_diff = {}
        for type in self._dict:
            self_devices = set(self._get_by_type(type))
            other_devices = set(other_peripherals._get_by_type(type))
            type_diff = other_devices.difference(self_devices)
            if type_diff:
                peripherals_diff[type] = type_diff
        return peripherals_diff

    def __repr__(self):
     return str(self._dict)


class enterprise_CFM_USBPeripheralHotplugDetect(test.test):
    """
    Uses servo to hotplug and detect USB peripherals on CrOS and hotrod.

    It compares attached audio/video peripheral names on CrOS against what
    Hangouts detects.
    """
    version = 1


    def _get_usb_device_types(self, vid_pid):
        """
        Returns a list of types (based on lsusb) for the specified vid:pid.

        @param vid_pid: The vid:pid of the device to query.
        @returns List of device types (string).
        """
        details_list = []
        cmd = 'lsusb -v -d %s' % vid_pid
        cmd_out = self.client.run(cmd).stdout.strip().split('\n')
        for line in cmd_out:
            if (any(phrase in line for phrase in ('bInterfaceClass',
                    'wTerminalType'))):
                device_type = line.split(None)[2]
                details_list.append(device_type)

        return details_list


    def _get_cros_usb_peripherals(self, peripherals_to_check):
        """
        Queries cros for connected USB devices.

        @param peripherals_to_check: A list of peripherals to query. If a
            connected USB device is not in this list it will be ignored.
        @returns  A _Peripherals object
        """
        cros_peripherals = _Peripherals()
        device_collector = usb_device_collector.UsbDeviceCollector(self.client)
        for device in device_collector.get_usb_devices():
            vid_pid = device.vid_pid
            device_types = self._get_usb_device_types(vid_pid)
            device_spec = cfm_usb_devices.get_usb_device_spec(vid_pid)
            if device_spec in peripherals_to_check:
                if 'Microphone' in device_types:
                    cros_peripherals.add_mic(device_spec)
                if 'Speaker' in device_types:
                    cros_peripherals.add_speaker(device_spec)
                if 'Video' in device_types:
                    cros_peripherals.add_camera(device_spec)
        return cros_peripherals


    def _enroll_device_and_skip_oobe(self):
        """Enroll device into CFM and skip CFM oobe."""
        self.cfm_facade.enroll_device()
        self.cfm_facade.skip_oobe_after_enrollment()
        self.cfm_facade.wait_for_hangouts_telemetry_commands()


    def _get_connected_cfm_hangouts_peripherals(self, peripherals_to_check):
        """
        Gets peripheral information as reported by Hangouts.

        @param peripherals_to_check: A list of peripherals to query. If a
            connected USB device is not in this list it will be ignored.
        @returns  A _Peripherals object
        """
        cfm_peripherals = _Peripherals()
        for mic_name in self.cfm_facade.get_mic_devices():
            mic_spec = _get_usb_spec_for_hangouts_device_name(
                mic_name, _HANGOUTS_UI_MICROPHONE_NAMES)
            if mic_spec and mic_spec in peripherals_to_check:
                cfm_peripherals.add_mic(mic_spec)

        for speaker_name in self.cfm_facade.get_speaker_devices():
            speaker_spec = _get_usb_spec_for_hangouts_device_name(
                speaker_name, _HANGOUTS_UI_SPEAKER_NAMES)
            if speaker_spec and speaker_spec in peripherals_to_check:
                cfm_peripherals.add_speaker(speaker_spec)

        for camera_name in self.cfm_facade.get_camera_devices():
            logging.info("Ccamera_name: %s", camera_name)
            camera_spec = _get_usb_spec_for_hangouts_device_name(
                camera_name, _HANGOUTS_UI_CAMERA_NAMES)
            logging.info("camera_spec: %s", camera_spec)
            if camera_spec and camera_spec in peripherals_to_check:
                cfm_peripherals.add_camera(camera_spec)
        return cfm_peripherals


    def _upload_crash_count(self, count):
        """Uploads crash count based on length of crash_files list."""
        self.output_perf_value(description='number_of_crashes',
                               value=int(count),
                               units='count', higher_is_better=False)


    def run_once(self, host, peripherals_to_check):
        """
        Main function to run autotest.

        @param host: Host object representing the DUT.
        @param peripherals_to_check: List of USB specs to check.
        """
        self.client = host
        self.crash_list =[]

        factory = remote_facade_factory.RemoteFacadeFactory(
                host, no_chrome=True)
        self.cfm_facade = factory.create_cfm_facade()

        detect_crash = crash_detector.CrashDetector(self.client)

        tpm_utils.ClearTPMOwnerRequest(self.client)

        factory = remote_facade_factory.RemoteFacadeFactory(
                host, no_chrome=True)
        self.cfm_facade = factory.create_cfm_facade()

        if detect_crash.is_new_crash_present():
            self.crash_list.append('New Warning or Crash Detected before ' +
                                   'plugging in usb peripherals.')

        # Turns on the USB port on the servo so that any peripheral connected to
        # the DUT it is visible.
        if self.client.servo:
            self.client.servo.switch_usbkey('dut')
            self.client.servo.set('usb_mux_sel3', 'dut_sees_usbkey')
            time.sleep(_SHORT_TIMEOUT)
            self.client.servo.set('dut_hub1_rst1', 'off')
            time.sleep(_SHORT_TIMEOUT)

        if detect_crash.is_new_crash_present():
            self.crash_list.append('New Warning or Crash Detected after ' +
                                   'plugging in usb peripherals.')

        cros_peripherals = self._get_cros_usb_peripherals(
            peripherals_to_check)
        logging.info('Peripherals detected by CrOS: %s', cros_peripherals)

        try:
            self._enroll_device_and_skip_oobe()
            cfm_peripherals = self._get_connected_cfm_hangouts_peripherals(
                peripherals_to_check)
            logging.info('Peripherals detected by hangouts: %s',
                          cfm_peripherals)
        except Exception as e:
            exception_msg = str(e)
            if self.crash_list:
                crash_identified_at = (' ').join(self.crash_list)
                exception_msg += '. ' + crash_identified_at
                self._upload_crash_count(len(detect_crash.get_crash_files()))
            raise error.TestFail(str(exception_msg))

        if detect_crash.is_new_crash_present():
            self.crash_list.append('New Warning or Crash detected after '
                                   'device enrolled into CFM.')

        tpm_utils.ClearTPMOwnerRequest(self.client)

        if self.crash_list:
            crash_identified_at = (', ').join(self.crash_list)
        else:
            crash_identified_at = 'No Crash or Warning detected.'

        self._upload_crash_count(len(detect_crash.get_crash_files()))

        peripherals_diff = cfm_peripherals.get_diff(cros_peripherals)
        if peripherals_diff:
            raise error.TestFail(
                'Peripherals do not match.\n'
                'Diff: {0} \n Cros: {1} \n Hangouts: {2} \n.'
                'No of Crashes: {3}. Crashes: {4}'.format(
                peripherals_diff, cros_peripherals, cfm_peripherals,
                len(detect_crash.get_crash_files()), crash_identified_at))
