# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, threading, time

from autotest_lib.client.common_lib.cros import crash_detector
from autotest_lib.server import autotest, test
from autotest_lib.client.common_lib import error

_WAIT_DELAY = 15
_LONG_TIMEOUT = 200
_LOWER_USB_PORT = 'usb_mux_sel3'
_SUSPEND_TIME = 30
_UPPER_USB_PORT = 'usb_mux_sel1'

class platform_ExternalUsbPeripherals(test.test):
    """Uses servo to repeatedly connect/remove USB devices during boot."""
    version = 1


    def getPluggedUsbDevices(self):
        """Determines the external USB devices plugged

        @returns plugged_list: List of plugged usb devices names

        """
        lsusb_output = self.host.run('lsusb').stdout.strip()
        items = lsusb_output.split('\n')
        plugged_list = []
        unnamed_device_count = 1
        for item in items:
            columns = item.split(' ')
            if len(columns) == 6 or len(' '.join(columns[6:]).strip()) == 0:
                logging.debug('Unnamed device located, adding generic name.')
                name = 'Unnamed device %d' % unnamed_device_count
                unnamed_device_count += 1
            else:
                name = ' '.join(columns[6:]).strip()
            plugged_list.append(name)
        return plugged_list


    def plug_peripherals(self, on=True):
        """Setting USB mux_3 plug status

        @param on: Connect the servo-usb port to DUT(plug) or to servo(unplug)

        """
        switch = 'dut_sees_usbkey'
        if not on:
            switch = 'servo_sees_usbkey'
        self.host.servo.set(self.plug_port, switch)
        self.pluged_status = on


    def action_login(self):
        """Login i.e. runs running client test

        @exception TestFail failed to login within timeout.

        """
        self.autotest_client.run_test(self.client_autotest,
                                      exit_without_logout=True)


    def wait_for_cmd_output(self, cmd, check, timeout, timeout_msg):
        """Waits till command output is meta

        @param cmd: executed command
        @param check: string to be checked for in cmd output
        @param timeout: max time in sec to wait for output
        @param timeout_msg: timeout failure message

        @returns True if check is found in command output; False otherwise
        """
        start_time = int(time.time())
        time_delta = 0
        command = '%s %s' % (cmd, check)
        logging.debug('Command: %s', command)
        while(self.host.run(command, ignore_status=True).exit_status != 0):
            time_delta = int(time.time()) - start_time
            if time_delta > timeout:
                self.add_failure('%s - %d sec' % (timeout_msg, timeout))
                return False
            time.sleep(0.5)
        logging.debug('Succeeded in :%d sec', time_delta)
        return True


    def suspend_for_time(self, suspend_time=_SUSPEND_TIME):
        """Calls the host method suspend with suspend_time argument.

        @param suspend_time: time to suspend the device for.

        """
        try:
            self.host.suspend(suspend_time=suspend_time)
        except error.AutoservSuspendError:
            pass


    def action_suspend(self):
        """Suspend i.e. powerd_dbus_suspend and wait

        @returns boot_id for the following resume
        """
        boot_id = self.host.get_boot_id()
        thread = threading.Thread(target=self.suspend_for_time)
        thread.start()
        self.host.test_wait_for_sleep(_LONG_TIMEOUT)
        logging.debug('--- Suspended')
        self.suspend_status = True
        return boot_id


    def action_resume(self, boot_id):
        """Resume i.e. press power key and wait

        @param boot_id: boot id obtained prior to suspending

        """
        self.host.test_wait_for_resume(boot_id, _LONG_TIMEOUT)
        logging.debug('--- Resumed')
        self.suspend_status = False

    def close_lid(self):
        """Close lid through servo to suspend the device."""
        boot_id = self.host.get_boot_id()
        logging.info('Closing lid...')
        self.host.servo.lid_close()
        self.host.test_wait_for_sleep(_LONG_TIMEOUT)
        self.suspend_status = True
        return boot_id


    def open_lid(self, boot_id):
        """Open lid through servo to resume."""
        logging.info('Opening lid...')
        self.host.servo.lid_open()
        self.host.test_wait_for_resume(boot_id, _LONG_TIMEOUT)
        self.suspend_status = False

    def crash_not_detected(self):
        """Finds new crash files and adds to failures list if any

        @returns True if there were not crashes; False otherwise
        """
        crash_files = self.detect_crash.get_new_crash_files()
        if crash_files:
            self.add_failure('CRASH DETECTED: %s' % str(crash_files))
            return False
        return True


    def check_plugged_usb_devices(self):
        """Checks the plugged peripherals match device list.

        @returns True if expected USB peripherals are detected; False otherwise
        """
        result = True
        if self.pluged_status and self.usb_list != None:
            # Check for mandatory USb devices passed by usb_list flag
            for usb_name in self.usb_list:
                found = self.wait_for_cmd_output(
                    'lsusb | grep -E ', usb_name, _WAIT_DELAY * 4,
                    'Not detecting %s' % usb_name)
                result = result and found
        time.sleep(_WAIT_DELAY)
        on_now = self.getPluggedUsbDevices()
        if self.pluged_status:
            if not self.diff_list.issubset(on_now):
                missing = str(self.diff_list.difference(on_now))
                self.add_failure('Missing connected peripheral(s) '
                                 'when plugged: %s ' % missing)
                result = False
        else:
            present = self.diff_list.intersection(on_now)
            if len(present) > 0:
                self.add_failure('Still presented peripheral(s) '
                                 'when unplugged: %s ' % str(present))
                result = False
        return result


    def check_usb_peripherals_details(self):
        """Checks the effect from plugged in USB peripherals.

        @returns True if command line output is matched successfuly; Else False
        """
        usb_check_result = True
        for cmd in self.usb_checks.keys():
            out_match_list = self.usb_checks.get(cmd)
            if cmd.startswith('loggedin:'):
                if not self.login_status:
                    continue
                cmd = cmd.replace('loggedin:','')
            # Run the usb check command
            for out_match in out_match_list:
                match_result = self.wait_for_cmd_output(
                    cmd, out_match, _WAIT_DELAY * 4,
                    'USB CHECKS DETAILS failed at %s %s:' % (cmd, out_match))
                usb_check_result = usb_check_result and match_result
        return usb_check_result


    def check_status(self):
        """Performs checks after each action:
            - for USB detected devices
            - for generated crash files
            - peripherals effect checks on cmd line

        @returns True if all of the iteration checks pass; False otherwise.
        """
        result = True
        if not self.suspend_status:
            # Detect the USB peripherals
            result = self.check_plugged_usb_devices()
            # Check for crash files
            if self.crash_check:
                result = result and self.crash_not_detected()
            if self.pluged_status and (self.usb_checks != None):
                # Check for plugged USB devices details
                result = result and self.check_usb_peripherals_details()
        return result


    def add_failure(self, reason):
        """ Adds a failure reason to list of failures to be reported at end

        @param reason: failure reason to record

        """
        if self.action_step is not None:
            self.fail_reasons.append('%s FAILS - %s' %
                                     (self.action_step, reason))


    def check_connected_peripherals(self):
        """ Verifies there are connected usb devices on servo

        @raise error.TestError: if no peripherals are shown
        """

        # Collect USB peripherals when unplugged
        self.plug_peripherals(False)
        time.sleep(_WAIT_DELAY)
        off_list = self.getPluggedUsbDevices()

        # Collect USB peripherals when plugged
        self.plug_peripherals(True)
        time.sleep(_WAIT_DELAY * 2)
        on_list = self.getPluggedUsbDevices()

        self.diff_list = set(on_list).difference(set(off_list))
        if len(self.diff_list) == 0:
            # Fail if no devices detected after
            raise error.TestError('No connected devices were detected. Make '
                                  'sure the devices are connected to USB_KEY '
                                  'and DUT_HUB1_USB on the servo board.')
        logging.debug('Connected devices list: %s', self.diff_list)


    def prep_servo_for_test(self, stress_rack):
        """Connects servo to DUT  and sets servo ports

        @param stress_rack: either to prep servo for stress tests, where
        usb_mux_1 port should be on. For usb peripherals on usb_mux_3,
        the port is on, and the oe2,oe4 poers are off.

        @returns port as string to plug/unplug the specific port
        """
        port = _LOWER_USB_PORT
        self.host.servo.switch_usbkey('dut')
        self.host.servo.set('dut_hub1_rst1','off')
        if stress_rack:
            port = _UPPER_USB_PORT
            self.host.servo.set(port, 'dut_sees_usbkey')
        else:
            self.host.servo.set(_UPPER_USB_PORT, 'servo_sees_usbkey')
            self.host.servo.set('usb_mux_oe2', 'off')
            self.host.servo.set('usb_mux_oe4', 'off')
        time.sleep(_WAIT_DELAY)
        return port


    def cleanup(self):
        """Disconnect servo hub"""
        self.plug_peripherals(False)
        self.host.servo.set('dut_hub1_rst1','on')
        self.host.reboot()


    def run_once(self, host, client_autotest, action_sequence, repeat,
                 usb_list=None, usb_checks=None,
                 crash_check=False, stress_rack=False):
        self.client_autotest = client_autotest
        self.host = host
        self.autotest_client = autotest.Autotest(self.host)
        self.usb_list = usb_list
        self.usb_checks = usb_checks
        self.crash_check = crash_check

        self.suspend_status = False
        self.login_status = False
        self.fail_reasons = list()
        self.action_step = None

        self.plug_port = self.prep_servo_for_test(stress_rack)

        # Unplug, plug, compare usb peripherals, and leave plugged.
        self.check_connected_peripherals()

        action_sequence = action_sequence.upper()
        actions = action_sequence.split(',')
        boot_id = 0
        self. detect_crash = crash_detector.CrashDetector(self.host)
        self.detect_crash.remove_crash_files()

        # Run camera client test to gather media_V4L2_test binary.
        if 'media_v4l2_test' in str(self.usb_checks):
            self.autotest_client.run_test("camera_V4L2")

        for iteration in xrange(1, repeat + 1):
            step = 0
            for action in actions:
                step += 1
                action = action.strip()
                self.action_step = 'STEP %d.%d. %s' % (iteration, step, action)
                logging.info(self.action_step)

                if action == 'RESUME':
                    self.action_resume(boot_id)
                    time.sleep(_WAIT_DELAY)
                elif action == 'OPENLID':
                    self.open_lid(boot_id)
                    time.sleep(_WAIT_DELAY)
                elif action == 'UNPLUG':
                    self.plug_peripherals(False)
                elif action == 'PLUG':
                    self.plug_peripherals(True)
                elif self.suspend_status == False:
                    if action.startswith('LOGIN'):
                        if self.login_status:
                            logging.debug('Skipping login. Already logged in.')
                            continue
                        else:
                            self.action_login()
                            self.login_status = True
                    elif action == 'REBOOT':
                        self.host.reboot()
                        time.sleep(_WAIT_DELAY * 3)
                        self.login_status = False
                    elif action == 'SUSPEND':
                        boot_id = self.action_suspend()
                    elif action == 'CLOSELID':
                        boot_id = self.close_lid()
                else:
                    logging.info('WRONG ACTION: %s .', self.action_step)
                self.check_status()

            if self.fail_reasons:
                raise error.TestFail('Failures reported: %s' %
                                     str(self.fail_reasons))
