# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a FAFT test for TCPC firmware updates.

This test forces TCPC firmware updates for the specified TCPCs.

The test is invoked with additional arguments to specify alternate
TCPC firmware blobs.  These are "edited" into the DUT's bios.bin
normally extracted from the system shellball.  Then, the bios.bin is
flashed into the DUT and the DUT is rebooted.

Under normal conditions, the TCPC firmware blobs will be updated as
part of software sync when the DUT reboots.  Software sync checks that
the new firmware is actually running on the TCPCs, however it can also
be audited after the fact using the firmware_CompareChipFwToShellBall
FAFT test for independent verification.

This test should be invoked twice: the 1st time to "downgrade" the
TCPC firmware, then a 2nd time to restore the production TCPC
firmware.  Alternatively, the system can be reflashed with a
production bios.bin (and rebooted) to restore the TCPC firmware.

The parade ps8751 (and similar) parts can be re-flashed indefinitely.
However, the analogix parts can only be updated about 100 times which
means it is not feasible to include them in continuous automated
testing.

This test will only replace existing TCPC firmware blobs in bios.bin.
If the corresponding binary blobs are not found in cbfs, it is assumed
that the release does not support the requested TCPCs.  Alternatively,
a bios.bin can be specified when invoking the test that will be used
insteade of the bios.bin normally extracted from the DUT's system
shellball.
"""

import logging
import os
import tempfile

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.cros import chip_utils
from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ChipFwUpdate(FirmwareTest):

    """Updates DUT firmware image with specified firmware blobs.

    If a new bios.bin is offered, it replaces the
    existing bios.bin.
    Then, if new chip firmware blobs are offered, they
    replace existing firmware blobs in bios.bin.
    Finally the system shellball is repacked.

    A reboot must be issued for the new firmware to be applied
    during software sync.

    Use the firmware_ChipFwUpdate test to verify that the new
    firmware was applied.
    """
    version = 1

    BIOS = 'bios.bin'
    HEXDUMP = 'hexdump -v -e \'1/1 "0x%02x\\n"\''

    def initialize(self, host, cmdline_args):
        dict_args = utils.args_to_dict(cmdline_args)
        super(firmware_ChipFwUpdate,
              self).initialize(host, cmdline_args)

        self.new_bios_path = dict_args['bios'] if 'bios' in dict_args else None

        self.clear_set_gbb_flags(
            vboot.GBB_FLAG_DISABLE_EC_SOFTWARE_SYNC |
            vboot.GBB_FLAG_DISABLE_PD_SOFTWARE_SYNC, 0)

        self.dut_bios_path = None
        self.cbfs_work_dir = None

        # set of chip types found in CBFS
        self.cbfs_chip_types = set()
        # dict of chip FW updates from the cmd line
        self.req_chip_updates = {}

        # see if comand line specified new firmware blobs
        # for chips we know about

        for chip in chip_utils.chip_id_map.itervalues():
            chip_name = chip.chip_name
            if chip_name not in dict_args:
                continue
            chip_file = dict_args[chip_name]
            if not os.path.exists(chip_file):
                raise error.TestError('file %s not found' % chip_file)
            c = chip()
            c.set_from_file(chip_file)
            if chip_name in self.req_chip_updates:
                raise error.TestError('multiple %s args' % chip_name)
            logging.info('request chip %s fw 0x%02x from command line',
                         c.chip_name, c.fw_ver)
            self.req_chip_updates[chip_name] = c

    def dut_setup_cbfs(self):
        """Sets up a work dir for cbfstool.

        Creates a fresh temp. dir for cbfstool to manipulate bios.bin.
        """

        cbfs_path = self.faft_client.updater.cbfs_setup_work_dir()
        bios_relative_path = self.faft_client.updater.get_bios_relative_path()
        self.cbfs_work_dir = cbfs_path
        self.dut_bios_path = os.path.join(cbfs_path, bios_relative_path)

    def cbfs_extract_chips(self):
        """Extracts interesting firmware blobs from cbfs.

        Iterates over requested chip updates and looks for corresponding
        firmware blobs in cbfs.  Firmware blobs are then extracted into
        cbfs_work_dir.
        """

        for chip in self.req_chip_updates.itervalues():
            logging.info('checking for %s firmware in %s',
                         chip.chip_name, self.BIOS)

            if not self.faft_client.updater.cbfs_extract_chip(chip.fw_name):
                logging.warning('%s firmware not bundled in %s',
                                chip.chip_name, self.BIOS)
                continue

            hashblob = self.faft_client.updater.cbfs_get_chip_hash(
                chip.fw_name)
            if not hashblob:
                logging.warning('%s firmware hash not extracted from %s',
                                chip.chip_name, self.BIOS)
                continue

            bundled_fw_ver = chip.fw_ver_from_hash(hashblob)
            if not bundled_fw_ver:
                raise error.TestFail(
                    'could not decode %s firmware hash: %s' % (
                        chip.chip_name, hashblob))

            self.cbfs_chip_types.add(type(chip))
            logging.info('%s bundled firmware for %s is version %s',
                         self.BIOS, chip.chip_name, bundled_fw_ver)

    def cbfs_replace_chips(self, host):
        """Iterates over known chips in cbfs.

        For each chip that has an update specified on the command line,
        copies the firmware (bin, hash) to DUT and updates cbfs in
        bios.bin.

        Args:
            host: host handle to the DUT.
        """

        for chip in self.cbfs_chip_types:
            chip_name = chip.chip_name
            logging.info('replacing %s firmware in %s', chip_name, self.BIOS)

            fw_update = self.req_chip_updates[chip_name]
            fw_hash = fw_update.compute_hash_bytes()
            (fd, n) = tempfile.mkstemp()
            with os.fdopen(fd, 'wb') as f:
                f.write(fw_hash)

            try:
                host.send_file(n,
                               os.path.join(
                                   self.cbfs_work_dir,
                                   fw_update.cbfs_hash_name))
            finally:
                os.unlink(n)

            host.send_file(fw_update.fw_file_name,
                           os.path.join(
                               self.cbfs_work_dir,
                               fw_update.cbfs_bin_name))

            if not self.faft_client.updater.cbfs_replace_chip(
                    fw_update.fw_name):
                raise error.TestFail('could not replace %s blobs in cbfs' %
                                     fw_update.chip_name)

    def dut_sign_and_flash_bios(self, host):
        """Signs the BIOS and flashes the DUT with it.

        Args:
            host: host handle to the DUT.
        """

        if not self.faft_client.updater.cbfs_sign_and_flash():
            raise error.TestFail('could not re-sign %s' % self.dut_bios_path)
        host.reboot()

    def run_once(self, host):
        # Make sure the client library is on the device so that the proxy
        # code is there when we try to call it.

        if not self.req_chip_updates:
            logging.info('no FW updates requested, skipping test')
            return

        self.dut_setup_cbfs()
        if self.new_bios_path:
            host.send_file(self.new_bios_path, self.dut_bios_path)

        self.cbfs_extract_chips()
        if not self.cbfs_chip_types:
            logging.info('firmware does not support requested updates, '
                         'skipping test')
            return

        self.cbfs_replace_chips(host)
        self.dut_sign_and_flash_bios(host)
