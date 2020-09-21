# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time
import re

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.cfm.usb import cfm_usb_devices
from autotest_lib.client.common_lib.cros.cfm.usb import usb_device_collector
from autotest_lib.server.cros.cfm import cfm_base_test

class enterprise_CFM_PTZStress(cfm_base_test.CfmBaseTest):
    """
    Executes the following tests on CFM devices:

       1. Enroll the device and join a meeting.
       2. During meeting PTZ the camera according to the control file.
    Verify the following functionalities:

       1. Camera is enumerated.
       2. Verify PTZ signal is sent to the camera.
    """
    version = 1

    def check_camera_enumeration(self, camera_name):
        """
        Checks if there is any camera connected to the DUT.
            If so, return the USB bus number the camera is on.
        @param camera_name the camera's name under test
        @returns The USB bus number the camera is on, if there is only
            1 camera connected, false otherwise.
        """
        collector = usb_device_collector.UsbDeviceCollector(self._host)
        camera = collector.get_devices_by_spec(camera_name)
        if len(camera) == 1:
            bus_number = camera[0].bus
            logging.info('Camera enumerated: {} on bus {}'.
                format(camera_name,bus_number))
            return bus_number
        raise error.TestFail('Camera failed to enumerate')


    def dump_usbmon_traffic(self, bus, usb_trace_path):
        """
        Start usbmon with specified bus and dump the traffic to file
        @param bus bus number the camera is on
        @param usb_trace_path the USB traces file path
        """
        cmd = ('cat /sys/kernel/debug/usb/usbmon/{}u > {} &'.
            format(bus, usb_trace_path))
        try:
            self._host.run(cmd, ignore_status = True)
        except Exception as e:
            logging.info('Fail to run cmd {}. Error: {}'.
                format(cmd, str(e)))
        logging.info('Usbmon traffic dumped to {}'.format(usb_trace_path))


    def check_usbmon_traffic(self, usb_trace_path):
        """
        Check traces
        @param usb_trace_path the USB traces file path
        """
        cmd = ('cat {} & '.format(usb_trace_path))
        try:
            traces = self._host.run_output(cmd, ignore_status = True)
            if re.search('C Ii', traces) and re.search('S Ii', traces):
                logging.info('PTZ signal verified')
            else:
                raise error.TestFail('PTZ signal did not go through')
        except Exception as e:
            logging.info('Fail to run cmd {}. Error: {}'.format(cmd, str(e)))


    def clean_usb_traces_file(self, usb_trace_path):
        """
        Clean traces file
        @param usb_trace_path the USB traces file path
        """
        cmd = ('rm {}'.format(usb_trace_path))
        try:
            self._host.run(cmd, ignore_status = True)
        except Exception as e:
            raise error.TestFail('Fail to run cmd {}. Error: {}'.format(cmd, str(e)))
        logging.info('Cleaned up traces in {}'.format(usb_trace_path))


    def run_once(self, host, test_config, ptz_motion_sequence):
        """Runs the test."""
        self.cfm_facade.wait_for_meetings_telemetry_commands()
        for loop_no in xrange(1, test_config['repeat'] + 1):
            logging.info('Test Loop : {}'.format(loop_no))
            bus = self.check_camera_enumeration(test_config['camera'])
            self.cfm_facade.start_meeting_session()
            self.dump_usbmon_traffic(bus, test_config['usb_trace_path'])
            for motion in ptz_motion_sequence:
                self.cfm_facade.move_camera(motion)
                time.sleep(test_config['motion_duration'])
            self.check_usbmon_traffic(test_config['usb_trace_path'])
            self.cfm_facade.end_meeting_session()
            self.clean_usb_traces_file(test_config['usb_trace_path'])
