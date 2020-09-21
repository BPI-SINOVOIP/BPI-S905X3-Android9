#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import common
from autotest_lib.server.hosts import cros_firmware
from autotest_lib.server.hosts import cros_repair
from autotest_lib.server.hosts import repair


CROS_VERIFY_DAG = (
    (repair.SshVerifier, 'ssh', ()),
    (cros_repair.DevModeVerifier, 'devmode', ('ssh',)),
    (cros_repair.HWIDVerifier,    'hwid',    ('ssh',)),
    (cros_repair.ACPowerVerifier, 'power', ('ssh',)),
    (cros_repair.EXT4fsErrorVerifier, 'ext4', ('ssh',)),
    (cros_repair.WritableVerifier, 'writable', ('ssh',)),
    (cros_repair.TPMStatusVerifier, 'tpm', ('ssh',)),
    (cros_repair.UpdateSuccessVerifier, 'good_au', ('ssh',)),
    (cros_firmware.FirmwareStatusVerifier, 'fwstatus', ('ssh',)),
    (cros_firmware.FirmwareVersionVerifier, 'rwfw', ('ssh',)),
    (cros_repair.PythonVerifier, 'python', ('ssh',)),
    (repair.LegacyHostVerifier, 'cros', ('ssh',)),
)

CROS_REPAIR_ACTIONS = (
    (repair.RPMCycleRepair, 'rpm', (), ('ssh', 'power',)),
    (cros_repair.ServoSysRqRepair, 'sysrq', (), ('ssh',)),
    (cros_repair.ServoResetRepair, 'servoreset', (), ('ssh',)),
    (cros_firmware.FirmwareRepair,
     'firmware', (), ('ssh', 'fwstatus', 'good_au')),
    (cros_repair.CrosRebootRepair,
     'reboot', ('ssh',), ('devmode', 'writable',)),
    (cros_repair.AutoUpdateRepair,
     'au',
     ('ssh', 'writable', 'tpm', 'good_au', 'ext4'),
     ('power', 'rwfw', 'python', 'cros')),
    (cros_repair.PowerWashRepair,
     'powerwash',
     ('ssh', 'writable'),
     ('tpm', 'good_au', 'ext4', 'power', 'rwfw', 'python', 'cros')),
    (cros_repair.ServoInstallRepair,
     'usb',
     (),
     ('ssh', 'writable', 'tpm', 'good_au', 'ext4', 'power', 'rwfw',
      'python', 'cros')),
)

MOBLAB_VERIFY_DAG = (
    (repair.SshVerifier, 'ssh', ()),
    (cros_repair.ACPowerVerifier, 'power', ('ssh',)),
    (cros_firmware.FirmwareVersionVerifier, 'rwfw', ('ssh',)),
    (cros_repair.PythonVerifier, 'python', ('ssh',)),
    (repair.LegacyHostVerifier, 'cros', ('ssh',)),
)

MOBLAB_REPAIR_ACTIONS = (
    (repair.RPMCycleRepair, 'rpm', (), ('ssh', 'power',)),
    (cros_repair.AutoUpdateRepair,
     'au', ('ssh',), ('power', 'rwfw', 'python', 'cros',)),
)

JETSTREAM_VERIFY_DAG = (
    (repair.SshVerifier, 'ssh', ()),
    (cros_repair.DevModeVerifier, 'devmode', ('ssh',)),
    (cros_repair.HWIDVerifier,    'hwid',    ('ssh',)),
    (cros_repair.ACPowerVerifier, 'power', ('ssh',)),
    (cros_repair.EXT4fsErrorVerifier, 'ext4', ('ssh',)),
    (cros_repair.WritableVerifier, 'writable', ('ssh',)),
    (cros_repair.TPMStatusVerifier, 'tpm', ('ssh',)),
    (cros_repair.UpdateSuccessVerifier, 'good_au', ('ssh',)),
    (cros_firmware.FirmwareStatusVerifier, 'fwstatus', ('ssh',)),
    (cros_firmware.FirmwareVersionVerifier, 'rwfw', ('ssh',)),
    (cros_repair.PythonVerifier, 'python', ('ssh',)),
    (repair.LegacyHostVerifier, 'cros', ('ssh',)),
    (cros_repair.JetstreamServicesVerifier, 'jetstream_services', ('ssh',)),
)

JETSTREAM_REPAIR_ACTIONS = (
    (repair.RPMCycleRepair, 'rpm', (), ('ssh', 'power',)),
    (cros_repair.ServoSysRqRepair, 'sysrq', (), ('ssh',)),
    (cros_repair.ServoResetRepair, 'servoreset', (), ('ssh',)),
    (cros_firmware.FirmwareRepair,
     'firmware', (), ('ssh', 'fwstatus', 'good_au')),
    (cros_repair.CrosRebootRepair,
     'reboot', ('ssh',), ('devmode', 'writable',)),
    (cros_repair.JetstreamRepair,
     'jetstream_repair',
     ('ssh', 'writable', 'tpm', 'good_au', 'ext4'),
     ('power', 'rwfw', 'python', 'cros', 'jetstream_services')),
    (cros_repair.AutoUpdateRepair,
     'au',
     ('ssh', 'writable', 'tpm', 'good_au', 'ext4'),
     ('power', 'rwfw', 'python', 'cros', 'jetstream_services')),
    (cros_repair.PowerWashRepair,
     'powerwash',
     ('ssh', 'writable'),
     ('tpm', 'good_au', 'ext4', 'power', 'rwfw', 'python', 'cros',
      'jetstream_services')),
    (cros_repair.ServoInstallRepair,
     'usb',
     (),
     ('ssh', 'writable', 'tpm', 'good_au', 'ext4', 'power', 'rwfw', 'python',
      'cros', 'jetstream_services')),
)

class CrosRepairUnittests(unittest.TestCase):

    maxDiff = None

    def test_cros_repair(self):
        verify_dag = cros_repair._cros_verify_dag()
        self.assertTupleEqual(verify_dag, CROS_VERIFY_DAG)
        self.check_verify_dag(verify_dag)
        repair_actions = cros_repair._cros_repair_actions()
        self.assertTupleEqual(repair_actions, CROS_REPAIR_ACTIONS)
        self.check_repair_actions(verify_dag, repair_actions)

    def test_moblab_repair(self):
        verify_dag = cros_repair._moblab_verify_dag()
        self.assertTupleEqual(verify_dag, MOBLAB_VERIFY_DAG)
        self.check_verify_dag(verify_dag)
        repair_actions = cros_repair._moblab_repair_actions()
        self.assertTupleEqual(repair_actions, MOBLAB_REPAIR_ACTIONS)
        self.check_repair_actions(verify_dag, repair_actions)

    def test_jetstream_repair(self):
        verify_dag = cros_repair._jetstream_verify_dag()
        self.assertTupleEqual(verify_dag, JETSTREAM_VERIFY_DAG)
        self.check_verify_dag(verify_dag)
        repair_actions = cros_repair._jetstream_repair_actions()
        self.assertTupleEqual(repair_actions, JETSTREAM_REPAIR_ACTIONS)
        self.check_repair_actions(verify_dag, repair_actions)

    def check_verify_dag(self, verify_dag):
        """Checks that dependency labels are defined."""
        labels = [n[1] for n in verify_dag]
        for node in verify_dag:
            for dep in node[2]:
                self.assertIn(dep, labels)

    def check_repair_actions(self, verify_dag, repair_actions):
        """Checks that dependency and trigger labels are defined."""
        verify_labels = [n[1] for n in verify_dag]
        for action in repair_actions:
            deps = action[2]
            triggers = action[3]
            for label in deps + triggers:
                self.assertIn(label, verify_labels)


if __name__ == '__main__':
    unittest.main()
