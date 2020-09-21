# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class firmware_LockedME(test.test):
    # Needed by autotest
    version = 1

    # Temporary file to read BIOS image into. We run in a tempdir anyway, so it
    # doesn't need a path.
    BIOS_FILE = 'bios.bin'

    def flashrom(self, ignore_status=False, args=()):
        """Run flashrom, expect it to work. Fail if it doesn't"""
        extra = ['-p', 'host'] + list(args)
        return utils.run('flashrom', ignore_status=ignore_status, args=extra)

    def determine_sw_wp_status(self):
        """Determine software write-protect status."""
        logging.info('Check that SW WP is enabled or not...')
        flashrom_result = self.flashrom(args=('--wp-status',))
        logging.info('The above flashrom command returns.... %s',
                flashrom_result.stdout)
        if (("disabled" in flashrom_result.stdout) and
                ("start=0x00000000, len=0x0000000" in flashrom_result.stdout)):
            return False
        else:
            return True

    def has_ME(self):
        """See if we can detect an ME.
        FREG* is printed only when HSFS_FDV is set, which means the descriptor
        table is valid. If we're running a BIOS without a valid descriptor this
        step will fail. Unfortunately, we don't know of a simple and reliable
        way to identify systems that have ME hardware.
        """
        logging.info('See if we have an ME...')
        r = self.flashrom(args=('-V',))
        return r.stdout.find("FREG0") >= 0

    def try_to_rewrite(self, sectname):
        """If we can modify the ME section, restore it and raise an error."""
        logging.info('Try to write section %s...', sectname)
        size = os.stat(sectname).st_size
        utils.run('dd', args=('if=/dev/urandom', 'of=newdata',
                              'count=1', 'bs=%d' % (size)))
        r = self.flashrom(args=('-w', self.BIOS_FILE,
                                '-i' , '%s:newdata' % (sectname),
                                '--fast-verify'),
                          ignore_status=True)
        if not r.exit_status:
            logging.info('Oops, it worked! Put it back...')
            self.flashrom(args=('-w', self.BIOS_FILE,
                                '-i', '%s:%s' % (sectname, sectname),
                                '--fast-verify'),
                          ignore_status=True)
            raise error.TestFail('%s is writable, ME is unlocked' % sectname)

    def check_manufacturing_mode(self):
        """Fail if manufacturing mode is not found or enbaled."""

        # See if coreboot told us that the ME is still in Manufacturing Mode.
        # It shouldn't be. We have to look only at the last thing it reports
        # because it reports the values twice and the first one isn't always
        # reliable.
        logging.info('Check for Manufacturing Mode...')
        last = None
        with open('/sys/firmware/log') as infile:
            for line in infile:
                if re.search('ME: Manufacturing Mode', line):
                    last = line
        if last is not None and last.find("YES") >= 0:
            raise error.TestFail("The ME is still in Manufacturing Mode")

    def check_region_inaccessible(self, sectname):
        """Test and ensure a region is not accessible by host CPU."""

        # flashrom should have read the section as all 0xff's. If not,
        # the ME is not locked.
        logging.info('%s should be all 0xff...' % sectname)
        with open(sectname, 'rb') as f:
            for c in f.read():
                if c != chr(0xff):
                    err_string = "%s was readable by flashrom" % sectname
                    raise error.TestFail(err_string)

        # See if it is writable.
        self.try_to_rewrite(sectname)

    def run_once(self, expect_me_present=True):
        """Fail unless the ME is locked.

        @param expect_me_present: False means the system has no ME.
        """
        cpu_arch = utils.get_cpu_arch()
        if cpu_arch == "arm":
            raise error.TestNAError('This test is not applicable, '
                    'because an ARM device has been detected. '
                    'ARM devices do not have an ME (Management Engine)')
        # If sw wp is on, and the ME regions are unlocked, they won't be
        # writable so will appear locked.
        if self.determine_sw_wp_status():
            raise error.TestFail('Software wp is enabled. Please disable '
                'software wp prior to running this test.')

        # See if the system even has an ME, and whether we expected that.
        if self.has_ME():
            if not expect_me_present:
                raise error.TestFail('We expected no ME, but found one anyway')
        else:
            if expect_me_present:
                raise error.TestNAError("No ME found. That's probably wrong.")
            else:
                logging.info('We expected no ME and we have no ME, so pass.')
                return

        # Make sure manufacturing mode is off.
        self.check_manufacturing_mode()

        # Read the image using flashrom.
        self.flashrom(args=('-r', self.BIOS_FILE))

        # Use 'IFWI' fmap region as a proxy for a device which doesn't
        # have a dedicated ME region in the boot media.
        r = utils.run('dump_fmap', args=('-p', self.BIOS_FILE))
        is_IFWI_platform = r.stdout.find("IFWI") >= 0

        # Get the bios image and extract the ME components
        logging.info('Pull the ME components from the BIOS...')
        dump_fmap_args = ['-x', self.BIOS_FILE, 'SI_DESC']
        inaccessible_sections = []
        if is_IFWI_platform:
            inaccessible_sections.append('DEVICE_EXTENSION')
        else:
            inaccessible_sections.append('SI_ME')
        dump_fmap_args.extend(inaccessible_sections)
        utils.run('dump_fmap', args=tuple(dump_fmap_args))

        # So far, so good, but we need to be certain. Rather than parse what
        # flashrom tells us about the ME-related registers, we'll just try to
        # change the ME components. We shouldn't be able to.
        self.try_to_rewrite('SI_DESC')
        for sectname in inaccessible_sections:
            self.check_region_inaccessible(sectname)

        # Okay, that's about all we can try. Looks like it's locked.
