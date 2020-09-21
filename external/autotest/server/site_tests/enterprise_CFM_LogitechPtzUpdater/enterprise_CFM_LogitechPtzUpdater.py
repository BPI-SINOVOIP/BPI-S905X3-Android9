# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Autotest for Logitech PTZPro firmware updater functionality and udev rule."""

from __future__ import print_function
import logging
import os
import re
import time

from autotest_lib.client.common_lib.cros import power_cycle_usb_util

from autotest_lib.client.common_lib import error
from autotest_lib.server import test

POWER_CYCLE_WAIT_TIME = 1  # seconds
UPDATER_WAIT_TIME = 100  # seconds


class enterprise_CFM_LogitechPtzUpdater(test.test):
    """Logitech Ptz Pro firmware updater functionality test in Chrome Box.

    The procedure of the test is:
    1. setup old firmware as dufault firmware on DUT
    2. flash old version FW to device,
    3. setup new firmware as default firmware on DUT
    4. power cycle usb port to simulate unplug and replug of device, which
       should be able to trigger udev rule and run the updater,
    5. wait for the updater to finish,
    6. run fw updater again and verify that the FW in device is consistent
       with latest FW within system by checking the output.
    """

    version = 1

    _LOG_FILE_PATH = '/tmp/logitech-updater.log'
    _FW_PATH_BASE = '/lib/firmware/logitech'
    _FW_PKG_ORIGIN = 'ptzpro2'
    _FW_PKG_BACKUP = 'ptzpro2_backup'
    _FW_PKG_TEST = 'ptzpro2_154'
    _FW_PATH_ORIGIN = os.path.join(_FW_PATH_BASE, _FW_PKG_ORIGIN)
    _FW_PATH_BACKUP = os.path.join(_FW_PATH_BASE, _FW_PKG_BACKUP)
    _FW_PATH_TEST = os.path.join(_FW_PATH_BASE, _FW_PKG_TEST)
    _DUT_BOARD = 'guado'
    _SIS_VID = '046d'
    _SIS_PID = '085f'

    def initialize(self, host):
        self.host = host
        self.log_file = self._LOG_FILE_PATH
        self.board = self._DUT_BOARD
        self.vid = self._SIS_VID
        self.pid = self._SIS_PID
        # Open log file object.
        self.log_file_obj = open(self.log_file, 'w')

    def cleanup(self):
        self.log_file_obj.close()
        test.test.cleanup(self)

        # Delete test firmware package.
        cmd = 'rm -rf {}'.format(self._FW_PATH_TEST)
        self._run_cmd(cmd)

        # Delete the symlink created.
        cmd = 'rm {}'.format(self._FW_PATH_ORIGIN)
        self._run_cmd(cmd)

        # Move the backup package back.
        cmd = 'mv {} {}'.format(self._FW_PATH_BACKUP, self._FW_PATH_ORIGIN)
        self._run_cmd(cmd)

    def _run_cmd(self, command, str_compare='', print_output=False):
        """Run command line on DUT.

        Run commands on DUT. Wait for command to complete, then check
        the output for expected string.

        @param command: command line to run in dut.
        @param str_compare: a piece of string we want to see in the
                output of running the command.
        @param print_output: if true, print command output in log.

        @returns the command output and a bool value. If str_compare is
               in command output, return true. Otherwise return false.

        """

        logging.info('Execute: %s', command)
        result = self.host.run(command, ignore_status=True)
        if result.stderr:
            output = result.stderr
        else:
            output = result.stdout
        if print_output:
            logging.info('Output: %s', output.split('\n'))
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

        Query the DUT's filesystem /dev/root, often manifested as
        /dev/dm-0 or is mounted as read-only or not.

        @returns True if the /dev/root is read-writable. False otherwise.
        """

        cmd = 'cat /proc/mounts | grep "/dev/root"'
        result, _ = self._run_cmd(cmd)
        fields = re.split(' |,', result)
        return True if fields.__len__() >= 4 and fields[3] == 'rw' else False

    def copy_firmware(self):
        """Copy test firmware from server to DUT."""

        current_dir = os.path.dirname(os.path.realpath(__file__))
        src_firmware_path = os.path.join(current_dir, self._FW_PKG_TEST)
        dst_firmware_path = self._FW_PATH_BASE
        logging.info('Copy firmware from {} to {}.'.format(src_firmware_path,
                                                           dst_firmware_path))
        self.host.send_file(src_firmware_path, dst_firmware_path, delete_dest=True)

    def triger_updater(self):
        """Triger udev rule to run fw updater by power cycling the usb."""

        try:
            power_cycle_usb_util.power_cycle_usb_vidpid(self.host, self.board,
                                                        self.vid, self.pid)
        except KeyError:
            raise error.TestFail('Counld\'t find target device: '
                                 'vid:pid {}:{}'.format(self.vid, self.pid))

    def setup_fw(self, firmware_package):
        """Setup firmware package that is going to be used for updating."""

        firmware_path = os.path.join(self._FW_PATH_BASE, firmware_package)
        cmd = 'ln -sfn {} {}'.format(firmware_path, self._FW_PATH_ORIGIN)
        self._run_cmd(cmd)

    def flash_fw(self, str_compare='', print_output=False, force=False):
        """Flash certain firmware to device.

        Run logitech firmware updater on DUT to flash the firmware setuped
        to target device (PTZ Pro 2).

        @param force: run with force update, will bypass fw version check.
        @param str_compare, print_output: the same as function _run_cmd.

        """

        if force:
            cmd_run_updater = ('/usr/sbin/logitech-updater'
                               ' --log_to=stdout --update --force')
        else:
            cmd_run_updater = ('/usr/sbin/logitech-updater --log_to=stdout --update')
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
        cmd = 'mv {} {}'.format(self._FW_PATH_ORIGIN, self._FW_PATH_BACKUP)
        self._run_cmd(cmd)
        self.copy_firmware()

        # Flash old FW to device.
        self.setup_fw(self._FW_PKG_TEST)
        expect_output = 'Done. Updated firmwares successfully.'
        output, succeed = self.flash_fw(str_compare=expect_output, force=True)
        self.log_file_obj.write('{}Log info for writing '
                                'old firmware{}\n'.format('-' * 8, '-' * 8))
        self.log_file_obj.write(output)
        if not succeed:
            raise error.TestFail('Expect \'{}\' in output, '
                                 'but didn\'t find it.'.format(expect_output))

        # Triger udev to run FW updater.
        self.setup_fw(self._FW_PKG_BACKUP)
        self.triger_updater()

        # Wait for fw updater to finish.
        time.sleep(UPDATER_WAIT_TIME)

        # Try flash the new firmware, should detect same fw version.
        expect_output = 'Firmware is up to date.'
        output, succeed = self.flash_fw(str_compare=expect_output)
        self.log_file_obj.write('{}Log info for writing '
                                'new firmware{}\n'.format('-' * 8, '-' * 8))
        self.log_file_obj.write(output)
        if not succeed:
            raise error.TestFail('Expect {} in output '
                                 'but didn\'t find it.'.format(expect_output))
