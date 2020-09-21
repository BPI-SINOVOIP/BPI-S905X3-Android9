# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Auto test for SiS firmware updater functionality and udev rule."""

from __future__ import print_function
import logging
import os
import re
import time

from autotest_lib.client.common_lib.cros import power_cycle_usb_util

from autotest_lib.client.common_lib import error
from autotest_lib.server import test

POWER_CYCLE_WAIT_TIME = 1   # seconds
UPDATER_WAIT_TIME = 80      # seconds
# This is the GPIO on guado.
FRONT_LEFT_USB_GPIO = 218


class enterprise_CFM_SiSFwUpdater(test.test):
    """
    SiS firmware updater functionality test in Chrome Box.

    The procedure of the test is:
    1. flash old version FW to device,
    2. power cycle usb port to simulate unplug and replug of device, which
         should be able to trigger udev rule and run the updater,
    3. wait for the updater to finish,
    4. run fw updater again and verify that the FW in device is consistent with
         latest FW within system by checking the output.
    """

    version = 1

    _LOG_FILE_PATH = '/tmp/sis-updater.log'
    _FW_PATH = '/lib/firmware/sis/'
    _OLD_FW_NAME = 'FW_Watchdog_0110.bin'
    _NEW_FW_NAME = 'WYD_101_WYD_9255_A353_V03.bin'
    _DUT_BOARD = 'guado'
    _SIS_VID = '266e'
    _SIS_PID = '0110'

    def initialize(self, host):
        self.host = host
        self.log_file = self._LOG_FILE_PATH
        self.old_fw_path = os.path.join(self._FW_PATH, self._OLD_FW_NAME)
        self.new_fw_path = os.path.join(self._FW_PATH, self._NEW_FW_NAME)
        self.usb_port_gpio_number = FRONT_LEFT_USB_GPIO
        self.board = self._DUT_BOARD
        self.vid = self._SIS_VID
        self.pid = self._SIS_PID
        # Open log file object.
        self.log_file_obj = open(self.log_file, 'w')

    def cleanup(self):
        self.log_file_obj.close()
        test.test.cleanup(self)
        cmd = 'rm -f {}'.format(self.old_fw_path)
        self._run_cmd(cmd)

    def _run_cmd(self, command, str_compare='', print_output=False):
        """
        Run command line on DUT.

        Run commands on DUT. Wait for command to complete, then check the
        output for expected string.

        @param command: command line to run in dut.
        @param str_compare: a piece of string we want to see in the output of
                running the command.
        @param print_output: if true, print command output in log.

        @returns the command output and a bool value. If str_compare is in
              command output, return true. Otherwise return false.

        """

        logging.info('Execute: %s', command)
        result = self.host.run(command, ignore_status=True)
        if result.stderr:
            output = result.stderr
        else:
            output = result.stdout
        if print_output:
            logging.info('Output: %s', ''.join(output))
        if str_compare and str_compare not in ''.join(output):
            return output, False
        else:
            return output, True

    def convert_rootfs_writable(self):
        """Remove rootfs verification on DUT, reboot,
        and remount the filesystem read-writable"""

        logging.info('Disabling rootfs verification...')
        self.remove_rootfs_verification()

        logging.info('Rebooting...')
        self.reboot()

        logging.info('Remounting..')
        cmd = 'mount -o remount,rw /'
        self._run_cmd(cmd)

    def remove_rootfs_verification(self):
        """Remove rootfs verification."""

        # 2 & 4 are default partitions, and the system boots from one of them.
        # Code from chromite/scripts/deploy_chrome.py
        KERNEL_A_PARTITION = 2
        KERNEL_B_PARTITION = 4

        cmd_template = ('/usr/share/vboot/bin/make_dev_ssd.sh --partitions %d '
                        '--remove_rootfs_verification --force')
        for partition in (KERNEL_A_PARTITION, KERNEL_B_PARTITION):
            cmd = cmd_template % partition
            self._run_cmd(cmd)

    def reboot(self):
        """Reboots the DUT."""

        self.host.reboot()

    def is_filesystem_readwrite(self):
        """Check if the root file system is read-writable.

        Query the DUT's filesystem /dev/root, often manifested as /dev/dm-0
        or  is mounted as read-only or not.

        @returns True if the /dev/root is read-writable. False otherwise.
        """

        cmd = 'cat /proc/mounts | grep "/dev/root"'
        result, _ = self._run_cmd(cmd)
        fields = re.split(' |,', result)
        return True if fields.__len__() >= 4 and fields[3] == 'rw' else False

    def copy_firmware(self):
        """Copy test firmware from server to DUT."""

        current_dir = os.path.dirname(os.path.realpath(__file__))
        src_firmware_path = os.path.join(current_dir, self._OLD_FW_NAME)
        dst_firmware_path = self._FW_PATH
        logging.info('Copy firmware from {} to {}.'.format(src_firmware_path,
                                                           dst_firmware_path))
        self.host.send_file(src_firmware_path, dst_firmware_path,
                            delete_dest=True)

    def triger_updater(self):
        """Triger udev rule to run fw updater."""

        try:
            power_cycle_usb_util.power_cycle_usb_vidpid(self.host, self.board,
                                                        self.vid, self.pid)
        except KeyError:
            raise error.TestFail('Counld\'t find target device: '
                                 'vid:pid {}:{}'.format(self.vid, self.pid))

    def flash_fw(self, fw_path, str_compare='', print_output=False):
        """
        Flash certain firmware to device.

        Run SiS firmware updater on DUT to flash the firmware given
        by fw_path to target device (Mimo).

        @param fw_path: the path to the firmware to flash.
        @param str_compare, print_output: the same as function _run_cmd.

        """
        cmd_run_updater = ('/usr/sbin/sis-updater '
                           '-ba -log_to=stdout {}'.format(fw_path))
        output, succeed = self._run_cmd(
            cmd_run_updater, str_compare=str_compare, print_output=print_output)
        return output, succeed

    def run_once(self):
        """Main test procedure."""

        # Make the DUT filesystem writable.
        if not self.is_filesystem_readwrite():
            logging.info('DUT root file system is not read-writable. '
                         'Converting it read-writable...')
            self.convert_rootfs_writable()
        else:
            logging.info('DUT is read-writable.')

        # Copy old FW to device.
        self.copy_firmware()

        # Flash old FW to device.
        expect_output = 'update firmware complete'
        output, succeed = self.flash_fw(self.old_fw_path,
                                        str_compare=expect_output)
        self.log_file_obj.write('{}Log info for writing '
                                'old firmware{}\n'.format('-'*8, '-'*8))
        self.log_file_obj.write(output)
        if not succeed:
            raise error.TestFail('Expect \'{}\' in output, '
                                 'but didn\'t find it.'.format(expect_output))

        # No need to manually triger udev to run FW updater here.
        # Previous FW updating process will reset SiS after it finish.

        # Wait for fw updater to finish.
        time.sleep(UPDATER_WAIT_TIME)

        # Try flash the new firmware, should detect same fw version.
        expect_output = 'The device has the same FW as system'
        output, succeed = self.flash_fw(self.new_fw_path,
                                        str_compare=expect_output)
        self.log_file_obj.write('{}Log info for writing '
                                'new firmware{}\n'.format('-'*8, '-'*8))
        self.log_file_obj.write(output)
        if not succeed:
            raise error.TestFail('Expect {} in output '
                                 'but didn\'t find it.'.format(expect_output))

