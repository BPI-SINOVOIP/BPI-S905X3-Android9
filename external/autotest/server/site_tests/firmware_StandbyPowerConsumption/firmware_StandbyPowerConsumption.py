# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time, os

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


PP_PATH = '/dev/ttyUSB0'
PP_LOG = '/tmp/powerplay.log'
CMD = '(stty 115200 cs8 -ixon; cat) < ' + PP_PATH + ' > ' + PP_LOG
WAIT_DELAY = 10
LONG_TIMEOUT = 60


class firmware_StandbyPowerConsumption(FirmwareTest):
    """Test captures power consumption data of a ChromeOS device while the
    device is in hibernate mode. It uses a stand alone utility called
    'powerplay' which is instrumented on the device using the battery terminals
    to provide power and track consumption. More information about powerplay can
    be found at go/powerplay.
    """
    version = 1


    def initialize(self, host, cmdline_args):
        (super(firmware_StandbyPowerConsumption, self)
                .initialize(host, cmdline_args))
        self.switcher.setup_mode('normal')


    def get_monetary_current(self, pp_file):
        """Extract momentary current value from each line of powerplay data.

        @param pp_file: Log file containing complete powerplay data set.
        @return list containing momentary current values.
        """
        momentary_curr_list = list()
        for line in open(os.path.join(self.resultsdir, pp_file)):
            pp_data = (line.replace('\00', '').
                       replace(' ', ',').replace('\r', ''))
            if (not pp_data.startswith('#') and (len(pp_data) > 30)):
                if pp_data[0].isdigit():
                    momentary_curr_list.append(
                            float(pp_data[pp_data.index(',')+1:]
                            [:pp_data[pp_data.index(',')+1:].index(',')]))
        return momentary_curr_list


    def set_powerplay_visible_to_servo_host(self, on=False):
        """Setting USB hub to make powerplay visible to servo host.

        @param on: To make powerplay visible to servo host or not.
        """
        if on:
            self.host.servo.switch_usbkey('host')
            self.host.servo.set('usb_mux_sel3', 'servo_sees_usbkey')
            self.host.servo.set('dut_hub1_rst1', 'off')
        else:
            self.host.servo.switch_usbkey('dut')
            self.host.servo.set('usb_mux_sel3', 'dut_sees_usbkey')
            self.host.servo.set('dut_hub1_rst1', 'on')
        time.sleep(WAIT_DELAY)


    def run_once(self, host, hibernate_length):
        """Main function to run autotset.

        @param host: Host object representing the DUT.
        @param hibernate_length: Length of time dut should be in hibernate mode.
        """
        self.host = host

        if not self.check_ec_capability(['x86','lid']):
            raise error.TestNAError("Nothing need to be tested on this device")

        self.set_powerplay_visible_to_servo_host(False)
        self.ec.send_command("hibernate")
        logging.info("Hibernating for %s seconds...", hibernate_length)
        self.host.test_wait_for_sleep(LONG_TIMEOUT)
        self.set_powerplay_visible_to_servo_host(True)

        self.s_host = self.host._servo_host
        is_pp_connected = self.s_host.run('ls ' + PP_PATH, ignore_status=True)
        if is_pp_connected.exit_status:
            self.set_powerplay_visible_to_servo_host(False)
            self.servo.power_short_press()
            if not self.host.ping_wait_up(LONG_TIMEOUT):
                raise error.TestNAError('Device did not resume from hibernate.')
            raise error.TestFail("Could not find powerplay.")

        pid = self.s_host.run_background(CMD)
        time.sleep(hibernate_length)
        self.set_powerplay_visible_to_servo_host(False)
        self.s_host.run_background('kill -9 ' + pid)
        self.servo.power_short_press()

        pp_file = os.path.join(self.resultsdir, 'powerplay.log')
        self.s_host.get_file(PP_LOG, pp_file)

        momentary_current = self.get_monetary_current(pp_file)
        avg_current_usage = sum(momentary_current)/len(momentary_current)
        peak_current_usage = max(momentary_current)
        self.output_perf_value(description='average_current_usage',
                value=avg_current_usage, units='amps', higher_is_better=False)
        self.output_perf_value(description='peak_current_usage',
                value=peak_current_usage, units='amps', higher_is_better=False)

        perf_keyval = {}
        perf_keyval['average_current_usage'] = avg_current_usage
        perf_keyval['peak_current_usage'] = peak_current_usage
        self.write_perf_keyval(perf_keyval)

        del_pp_log = self.s_host.run('rm ' + PP_LOG, ignore_status=True)
        if del_pp_log.exit_status:
            raise error.TestNAError("Unable to delete powerplay.log on servo "
                                    "host.")

        if not self.host.ping_wait_up(LONG_TIMEOUT):
            raise error.TestNAError('Device did not resume from hibernate.')
