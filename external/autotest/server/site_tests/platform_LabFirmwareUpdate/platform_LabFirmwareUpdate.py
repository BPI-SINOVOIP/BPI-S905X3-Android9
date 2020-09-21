# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server import test
from autotest_lib.server.cros.faft.rpc_proxy import RPCProxy

class platform_LabFirmwareUpdate(test.test):
    """For test or lab devices.  Test will fail if Software write protection
       is enabled.  Test will compare the installed firmware to those in
       the shellball.  If differ, execute chromeos-firmwareupdate
       --mode=recovery to reset RO and RW firmware. Basic procedure are:

       - check software write protect, if enable, attemp reset.
       - fail test if software write protect is enabled.
       - check if ec is available on DUT.
       - get RO, RW versions of firmware, if RO != RW, update=True
       - get shellball versions of firmware
       - compare shellball version to DUT, update=True if shellball != DUT.
       - run chromeos-firwmareupdate --mode=recovery if update==True
       - reboot
    """
    version = 1

    # TODO(kmshelton): Move most of the logic in this test to a unit tested
    # library.
    def initialize(self, host):
        self.host = host
        # Make sure the client library is on the device so that the proxy
        # code is there when we try to call it.
        client_at = autotest.Autotest(self.host)
        client_at.install()
        self.faft_client = RPCProxy(self.host)

        # Check if EC, PD is available.
        # Check if DUT software write protect is disabled, failed otherwise.
        self._run_cmd('flashrom -p host --wp-status', checkfor='is disabled')
        self.has_ec = False
        mosys_output = self._run_cmd('mosys')
        if 'EC information' in mosys_output:
            self.has_ec = True
            self._run_cmd('flashrom -p ec --wp-status', checkfor='is disabled')

    def _run_cmd(self, command, checkfor=''):
        """Run command on dut and return output.
           Optionally check output contain string 'checkfor'.
        """
        logging.info('Execute: %s', command)
        output = self.host.run(command, ignore_status=True).stdout
        logging.info('Output: %s', output.split('\n'))
        if checkfor and checkfor not in ''.join(output):
            raise error.TestFail('Expect %s in output of %s' %
                                 (checkfor, ' '.join(output)))
        return output

    def _get_version(self):
        """Retrive RO, RW EC/PD version."""
        ro = None
        rw = None
        lines = self._run_cmd('ectool version', checkfor='version')
        for line in lines.splitlines():
            if line.startswith('RO version:'):
                parts = line.split(':')
                ro = parts[1].strip()
            if line.startswith('RW version:'):
                parts = line.split(':')
                rw = parts[1].strip()
        return (ro, rw)

    def _bios_version(self):
        """Retrive RO, RW BIOS version."""
        ro = self.faft_client.system.get_crossystem_value('ro_fwid')
        rw = self.faft_client.system.get_crossystem_value('fwid')
        return (ro, rw)

    def _construct_fw_version(self, fw_ro, fw_rw):
        """Construct a firmware version string in a consistent manner.

        @param fw_ro: A string representing the version of a read-only
                      firmware.
        @param fw_rw: A string representing the version of a read-write
                      firmware.

        @returns a string constructed from fw_ro and fw_rw

        """
        if fw_ro == fw_rw:
            return fw_rw
        else:
            return '%s,%s' % (fw_ro, fw_rw)

    def _get_version_all(self):
        """Retrive BIOS, EC, and PD firmware version.

        @return firmware version tuple (bios, ec)
        """
        bios_version = None
        ec_version = None
        if self.has_ec:
            (ec_ro, ec_rw) = self._get_version()
            ec_version = self._construct_fw_version(ec_ro, ec_rw)
            logging.info('Installed EC version: %s', ec_version)
        (bios_ro, bios_rw) = self._bios_version()
        bios_version = self._construct_fw_version(bios_ro, bios_rw)
        logging.info('Installed BIOS version: %s', bios_version)
        return (bios_version, ec_version)

    def _get_shellball_version(self):
        """Get shellball firmware version.

        @return shellball firmware version tuple (bios, ec)
        """
        bios = None
        ec = None
        bios_ro = None
        bios_rw = None
        ec_ro = None
        ec_rw = None
        shellball = self._run_cmd('/usr/sbin/chromeos-firmwareupdate -V')
        # TODO(kmshelton): Add a structured output option (likely a protobuf)
        # to chromeos-firmwareupdate so the below can become less fragile.
        for line in shellball.splitlines():
            if line.startswith('BIOS version:'):
                parts = line.split(':')
                bios_ro = parts[1].strip()
                logging.info('shellball ro bios %s', bios_ro)
            if line.startswith('BIOS (RW) version:'):
                parts = line.split(':')
                bios_rw = parts[1].strip()
                logging.info('shellball rw bios %s', bios_rw)
            if line.startswith('EC version:'):
                parts = line.split(':')
                ec_ro = parts[1].strip()
                logging.info('shellball ro ec %s', ec_ro)
            elif line.startswith('EC (RW) version:'):
                parts = line.split(':')
                ec_rw = parts[1].strip()
                logging.info('shellball rw ec %s', ec_rw)
        # Shellballs do not always contain a RW version.
        if bios_rw is not None:
          bios = self._construct_fw_version(bios_ro, bios_rw)
        else:
          bios = bios_ro
        if ec_rw is not None:
          ec = self._construct_fw_version(ec_ro, ec_rw)
        else:
          ec = ec_ro
        return (bios, ec)

    def run_once(self, replace=True):
        # Get DUT installed firmware versions.
        (installed_bios, installed_ec) = self._get_version_all()

        # Get shellball firmware versions.
        (shball_bios, shball_ec) = self._get_shellball_version()

        # Figure out if update is needed.
        need_update = False
        if installed_bios != shball_bios:
            need_update = True
            logging.info('BIOS mismatch %s, will update to %s',
                         installed_bios, shball_bios)
        if installed_ec and installed_ec != shball_ec:
            need_update = True
            logging.info('EC mismatch %s, will update to %s',
                         installed_ec, shball_ec)

        # Update and reboot if needed.
        if need_update:
            output = self._run_cmd('/usr/sbin/chromeos-firmwareupdate '
                                   ' --mode=recovery', '(recovery) completed.')
            self.host.reboot()
            # Check that installed firmware match the shellball.
            (bios, ec) = self._get_version_all()
            # TODO(kmshelton): Refactor this test to use named tuples so that
            # the comparison is eaiser to grok.
            if (bios != shball_bios or ec != shball_ec):
                logging.info('shball bios/ec: %s/%s',
                             shball_bios, shball_ec)
                logging.info('installed bios/ec: %s/%s', bios, ec)
                raise error.TestFail('Version mismatch after firmware update')
            logging.info('*** Done firmware updated to match shellball. ***')
        else:
            logging.info('*** No firmware update is needed. ***')

