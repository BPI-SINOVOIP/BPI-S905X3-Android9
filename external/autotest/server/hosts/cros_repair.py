# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os
import time

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import hosts
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.server import afe_utils
from autotest_lib.server import crashcollect
from autotest_lib.server.hosts import repair
from autotest_lib.server.hosts import cros_firmware

# _DEV_MODE_ALLOW_POOLS - The set of pools that are allowed to be
# in dev mode (usually, those should be unmanaged devices)
#
_DEV_MODE_ALLOWED_POOLS = set(
    global_config.global_config.get_config_value(
            'CROS',
            'pools_dev_mode_allowed',
            type=str,
            default='',
            allow_blank=True).split(','))

# Setting to suppress dev mode check; primarily used for moblab where all
# DUT's are in dev mode.
_DEV_MODE_ALWAYS_ALLOWED = global_config.global_config.get_config_value(
            'CROS',
            'dev_mode_allowed',
            type=bool,
            default=False)

# Triggers for the 'au', 'powerwash', and 'usb' repair actions.
# These are also used as dependencies in the `CrosHost` repair
# sequence, as follows:
#
# au:
#   - triggers: _CROS_AU_TRIGGERS
#   - depends on: _CROS_USB_TRIGGERS + _CROS_POWERWASH_TRIGGERS
#
# powerwash:
#   - triggers: _CROS_POWERWASH_TRIGGERS + _CROS_AU_TRIGGERS
#   - depends on: _CROS_USB_TRIGGERS
#
# usb:
#   - triggers: _CROS_USB_TRIGGERS + _CROS_POWERWASH_TRIGGERS +
#               _CROS_AU_TRIGGERS
#   - no dependencies
#
# N.B. AC power detection depends on software on the DUT, and there
# have been bugs where detection failed even though the DUT really
# did have power.  So, we make the 'power' verifier a trigger for
# reinstall repair actions, too.
#
# TODO(jrbarnette):  AU repair can't fix all problems reported by
# the 'cros' verifier; it's listed as an AU trigger as a
# simplification.  The ultimate fix is to split the 'cros' verifier
# into smaller individual verifiers.
_CROS_AU_TRIGGERS = ('power', 'rwfw', 'python', 'cros',)
_CROS_POWERWASH_TRIGGERS = ('tpm', 'good_au', 'ext4',)
_CROS_USB_TRIGGERS = ('ssh', 'writable',)


class ACPowerVerifier(hosts.Verifier):
    """Check for AC power and a reasonable battery charge."""

    def verify(self, host):
        # Temporarily work around a problem caused by some old FSI
        # builds that don't have the power_supply_info command by
        # ignoring failures.  The repair triggers believe that this
        # verifier can't be fixed by re-installing, which means if a DUT
        # gets stuck with one of those old builds, it can't be repaired.
        #
        # TODO(jrbarnette): This is for crbug.com/599158; we need a
        # better solution.
        try:
            info = host.get_power_supply_info()
        except:
            logging.exception('get_power_supply_info() failed')
            return
        try:
            if info['Line Power']['online'] != 'yes':
                raise hosts.AutoservVerifyError(
                        'AC power is not plugged in')
        except KeyError:
            logging.info('Cannot determine AC power status - '
                         'skipping check.')
        try:
            if float(info['Battery']['percentage']) < 50.0:
                raise hosts.AutoservVerifyError(
                        'Battery is less than 50%')
        except KeyError:
            logging.info('Cannot determine battery status - '
                         'skipping check.')

    @property
    def description(self):
        return 'The DUT is plugged in to AC power'


class WritableVerifier(hosts.Verifier):
    """
    Confirm the stateful file systems are writable.

    The standard linux response to certain unexpected file system errors
    (including hardware errors in block devices) is to change the file
    system status to read-only.  This checks that that hasn't happened.

    The test covers the two file systems that need to be writable for
    critical operations like AU:
      * The (unencrypted) stateful system which includes
        /mnt/stateful_partition.
      * The encrypted stateful partition, which includes /var.

    The test doesn't check various bind mounts; those are expected to
    fail the same way as their underlying main mounts.  Whether the
    Linux kernel can guarantee that is untested...
    """

    # N.B. Order matters here:  Encrypted stateful is loop-mounted from
    # a file in unencrypted stateful, so we don't test for errors in
    # encrypted stateful if unencrypted fails.
    _TEST_DIRECTORIES = ['/mnt/stateful_partition', '/var/tmp']

    def verify(self, host):
        # This deliberately stops looking after the first error.
        # See above for the details.
        for testdir in self._TEST_DIRECTORIES:
            filename = os.path.join(testdir, 'writable_test')
            command = 'touch %s && rm %s' % (filename, filename)
            rv = host.run(command=command, ignore_status=True)
            if rv.exit_status != 0:
                msg = 'Can\'t create a file in %s' % testdir
                raise hosts.AutoservVerifyError(msg)

    @property
    def description(self):
        return 'The stateful filesystems are writable'


class EXT4fsErrorVerifier(hosts.Verifier):
    """
    Confirm we have not seen critical file system kernel errors.
    """
    def verify(self, host):
        # grep for stateful FS errors of the type "EXT4-fs error (device sda1):"
        command = ("dmesg | grep -E \"EXT4-fs error \(device "
                   "$(cut -d ' ' -f 5,9 /proc/$$/mountinfo | "
                   "grep -e '^/mnt/stateful_partition ' | "
                   "cut -d ' ' -f 2 | cut -d '/' -f 3)\):\"")
        output = host.run(command=command, ignore_status=True).stdout
        if output:
            sample = output.splitlines()[0]
            message = 'Saw file system error: %s' % sample
            raise hosts.AutoservVerifyError(message)
        # Check for other critical FS errors.
        command = 'dmesg | grep "This should not happen!!  Data will be lost"'
        output = host.run(command=command, ignore_status=True).stdout
        if output:
            message = 'Saw file system error: Data will be lost'
            raise hosts.AutoservVerifyError(message)
        else:
            logging.error('Could not determine stateful mount.')

    @property
    def description(self):
        return 'Did not find critical file system errors'


class UpdateSuccessVerifier(hosts.Verifier):
    """
    Checks that the DUT successfully finished its last provision job.

    At the start of any update (e.g. for a Provision job), the code
    creates a marker file named `host.PROVISION_FAILED`.  The file is
    located in a part of the stateful partition that will be removed if
    an update finishes successfully.  Thus, the presence of the file
    indicates that a prior update failed.

    The verifier tests for the existence of the marker file and fails if
    it still exists.
    """
    def verify(self, host):
        result = host.run('test -f %s' % host.PROVISION_FAILED,
                          ignore_status=True)
        if result.exit_status == 0:
            raise hosts.AutoservVerifyError(
                    'Last AU on this DUT failed')

    @property
    def description(self):
        return 'The most recent AU attempt on this DUT succeeded'


class TPMStatusVerifier(hosts.Verifier):
    """Verify that the host's TPM is in a good state."""

    def verify(self, host):
        if _is_virual_machine(host):
            # We do not forward host TPM / emulated TPM to qemu VMs, so skip
            # this verification step.
            logging.debug('Skipped verification %s on VM', self)
            return

        # This cryptohome command emits status information in JSON format. It
        # looks something like this:
        # {
        #    "installattrs": {
        #       ...
        #    },
        #    "mounts": [ {
        #       ...
        #    } ],
        #    "tpm": {
        #       "being_owned": false,
        #       "can_connect": true,
        #       "can_decrypt": false,
        #       "can_encrypt": false,
        #       "can_load_srk": true,
        #       "can_load_srk_pubkey": true,
        #       "enabled": true,
        #       "has_context": true,
        #       "has_cryptohome_key": false,
        #       "has_key_handle": false,
        #       "last_error": 0,
        #       "owned": true
        #    }
        # }
        output = host.run('cryptohome --action=status').stdout.strip()
        try:
            status = json.loads(output)
        except ValueError:
            logging.info('Cannot determine the Crytohome valid status - '
                         'skipping check.')
            return
        try:
            tpm = status['tpm']
            if not tpm['enabled']:
                raise hosts.AutoservVerifyError(
                        'TPM is not enabled -- Hardware is not working.')
            if not tpm['can_connect']:
                raise hosts.AutoservVerifyError(
                        ('TPM connect failed -- '
                         'last_error=%d.' % tpm['last_error']))
            if tpm['owned'] and not tpm['can_load_srk']:
                raise hosts.AutoservVerifyError(
                        'Cannot load the TPM SRK')
            if tpm['can_load_srk'] and not tpm['can_load_srk_pubkey']:
                raise hosts.AutoservVerifyError(
                        'Cannot load the TPM SRK public key')
        except KeyError:
            logging.info('Cannot determine the Crytohome valid status - '
                         'skipping check.')

    @property
    def description(self):
        return 'The host\'s TPM is available and working'


class PythonVerifier(hosts.Verifier):
    """Confirm the presence of a working Python interpreter."""

    def verify(self, host):
        result = host.run('python -c "import cPickle"',
                          ignore_status=True)
        if result.exit_status != 0:
            message = 'The python interpreter is broken'
            if result.exit_status == 127:
                search = host.run('which python', ignore_status=True)
                if search.exit_status != 0 or not search.stdout:
                    message = ('Python is missing; may be caused by '
                               'powerwash')
            raise hosts.AutoservVerifyError(message)

    @property
    def description(self):
        return 'Python on the host is installed and working'


class DevModeVerifier(hosts.Verifier):
    """Verify that the host is not in dev mode."""

    def verify(self, host):
        # Some pools are allowed to be in dev mode
        info = host.host_info_store.get()
        if (_DEV_MODE_ALWAYS_ALLOWED or
                bool(info.pools & _DEV_MODE_ALLOWED_POOLS)):
            return

        result = host.run('crossystem devsw_boot', ignore_status=True).stdout
        if result != '0':
            raise hosts.AutoservVerifyError('The host is in dev mode')

    @property
    def description(self):
        return 'The host should not be in dev mode'


class HWIDVerifier(hosts.Verifier):
    """Verify that the host has HWID & serial number."""

    def verify(self, host):
        try:
            info = host.host_info_store.get()

            hwid = host.run('crossystem hwid', ignore_status=True).stdout
            if hwid:
                info.attributes['HWID'] = hwid

            serial_number = host.run('vpd -g serial_number',
                                     ignore_status=True).stdout
            if serial_number:
                info.attributes['serial_number'] = serial_number

            if info != host.host_info_store.get():
                host.host_info_store.commit(info)
        except Exception as e:
            logging.exception('Failed to get HWID & Serial Number for host ',
                              '%s: %s', host.hostname, str(e))

    @property
    def description(self):
        return 'The host should have valid HWID and Serial Number'


class JetstreamServicesVerifier(hosts.Verifier):
    """Verify that Jetstream services are running."""

    # Retry for b/62576902
    @retry.retry(error.AutoservError, timeout_min=1, delay_sec=10)
    def verify(self, host):
        try:
            if not host.upstart_status('ap-controller'):
                raise hosts.AutoservVerifyError(
                    'ap-controller service is not running')
        except error.AutoservRunError:
            raise hosts.AutoservVerifyError(
                'ap-controller service not found')

        try:
            host.run('pgrep ap-controller')
        except error.AutoservRunError:
            raise hosts.AutoservVerifyError(
                'ap-controller process is not running')

    @property
    def description(self):
        return 'Jetstream services must be running'


class _ResetRepairAction(hosts.RepairAction):
    """Common handling for repair actions that reset a DUT."""

    def _collect_logs(self, host):
        """Collect logs from a successfully repaired DUT."""
        dirname = 'after_%s' % self.tag
        local_log_dir = crashcollect.get_crashinfo_dir(host, dirname)
        host.collect_logs('/var/log', local_log_dir, ignore_errors=True)
        # Collect crash info.
        crashcollect.get_crashinfo(host, None)

    def _check_reset_success(self, host):
        """Check whether reset succeeded, and gather logs if possible."""
        if host.wait_up(host.BOOT_TIMEOUT):
            try:
                # Collect logs once we regain ssh access before
                # clobbering them.
                self._collect_logs(host)
            except Exception:
                # If the DUT is up, we want to declare success, even if
                # log gathering fails for some reason.  So, if there's
                # a failure, just log it and move on.
                logging.exception('Unexpected failure in log '
                                  'collection during %s.',
                                  self.tag)
            return
        raise hosts.AutoservRepairError(
                'Host %s is still offline after %s.' %
                (host.hostname, self.tag))


class ServoSysRqRepair(_ResetRepairAction):
    """
    Repair a Chrome device by sending a system request to the kernel.

    Sending 3 times the Alt+VolUp+x key combination (aka sysrq-x)
    will ask the kernel to panic itself and reboot while conserving
    the kernel logs in console ramoops.
    """

    def repair(self, host):
        if not host.servo:
            raise hosts.AutoservRepairError(
                    '%s has no servo support.' % host.hostname)
        # Press 3 times Alt+VolUp+X
        # no checking DUT health between each press as
        # killing Chrome is not really likely to fix the DUT SSH.
        for _ in range(3):
            try:
                host.servo.sysrq_x()
            except error.TestFail, ex:
                raise hosts.AutoservRepairError(
                      'cannot press sysrq-x: %s.' % str(ex))
            # less than 5 seconds between presses.
            time.sleep(2.0)
        self._check_reset_success(host)

    @property
    def description(self):
        return 'Reset the DUT via keyboard sysrq-x'


class ServoResetRepair(_ResetRepairAction):
    """Repair a Chrome device by resetting it with servo."""

    def repair(self, host):
        if not host.servo:
            raise hosts.AutoservRepairError(
                    '%s has no servo support.' % host.hostname)
        host.servo.get_power_state_controller().reset()
        self._check_reset_success(host)

    @property
    def description(self):
        return 'Reset the DUT via servo'


class CrosRebootRepair(repair.RebootRepair):
    """Repair a CrOS target by clearing dev mode and rebooting it."""

    def repair(self, host):
        # N.B. We need to reboot regardless of whether clearing
        # dev_mode succeeds or fails.
        host.run('/usr/share/vboot/bin/set_gbb_flags.sh 0',
                 ignore_status=True)
        host.run('crossystem disable_dev_request=1',
                 ignore_status=True)
        super(CrosRebootRepair, self).repair(host)

    @property
    def description(self):
        return 'Reset GBB flags and Reboot the host'


class AutoUpdateRepair(hosts.RepairAction):
    """
    Repair by re-installing a test image using autoupdate.

    Try to install the DUT's designated "stable test image" using the
    standard procedure for installing a new test image via autoupdate.
    """

    def repair(self, host):
        afe_utils.machine_install_and_update_labels(host, repair=True)

    @property
    def description(self):
        return 'Re-install the stable build via AU'


class PowerWashRepair(AutoUpdateRepair):
    """
    Powerwash the DUT, then re-install using autoupdate.

    Powerwash the DUT, then attempt to re-install a stable test image as
    for `AutoUpdateRepair`.
    """

    def repair(self, host):
        host.run('echo "fast safe" > '
                 '/mnt/stateful_partition/factory_install_reset')
        host.reboot(timeout=host.POWERWASH_BOOT_TIMEOUT, wait=True)
        super(PowerWashRepair, self).repair(host)

    @property
    def description(self):
        return 'Powerwash and then re-install the stable build via AU'


class ServoInstallRepair(hosts.RepairAction):
    """
    Reinstall a test image from USB using servo.

    Use servo to re-install the DUT's designated "stable test image"
    from servo-attached USB storage.
    """

    def repair(self, host):
        if not host.servo:
            raise hosts.AutoservRepairError(
                    '%s has no servo support.' % host.hostname)
        host.servo_install(host.stage_image_for_servo())

    @property
    def description(self):
        return 'Reinstall from USB using servo'


class JetstreamRepair(hosts.RepairAction):
    """Repair by restarting Jetstrem services."""

    def repair(self, host):
        host.cleanup_services()

    @property
    def description(self):
        return 'Restart Jetstream services'


def _cros_verify_dag():
    """Return the verification DAG for a `CrosHost`."""
    FirmwareStatusVerifier = cros_firmware.FirmwareStatusVerifier
    FirmwareVersionVerifier = cros_firmware.FirmwareVersionVerifier
    verify_dag = (
        (repair.SshVerifier,         'ssh',      ()),
        (DevModeVerifier,            'devmode',  ('ssh',)),
        (HWIDVerifier,               'hwid',     ('ssh',)),
        (ACPowerVerifier,            'power',    ('ssh',)),
        (EXT4fsErrorVerifier,        'ext4',     ('ssh',)),
        (WritableVerifier,           'writable', ('ssh',)),
        (TPMStatusVerifier,          'tpm',      ('ssh',)),
        (UpdateSuccessVerifier,      'good_au',  ('ssh',)),
        (FirmwareStatusVerifier,     'fwstatus', ('ssh',)),
        (FirmwareVersionVerifier,    'rwfw',     ('ssh',)),
        (PythonVerifier,             'python',   ('ssh',)),
        (repair.LegacyHostVerifier,  'cros',     ('ssh',)),
    )
    return verify_dag


def _cros_basic_repair_actions():
    """Return the basic repair actions for a `CrosHost`"""
    FirmwareRepair = cros_firmware.FirmwareRepair
    repair_actions = (
        # RPM cycling must precede Servo reset:  if the DUT has a dead
        # battery, we need to reattach AC power before we reset via servo.
        (repair.RPMCycleRepair, 'rpm', (), ('ssh', 'power',)),
        (ServoSysRqRepair, 'sysrq', (), ('ssh',)),
        (ServoResetRepair, 'servoreset', (), ('ssh',)),

        # N.B. FirmwareRepair can't fix a 'good_au' failure directly,
        # because it doesn't remove the flag file that triggers the
        # failure.  We include it as a repair trigger because it's
        # possible the the last update failed because of the firmware,
        # and we want the repair steps below to be able to trust the
        # firmware.
        (FirmwareRepair, 'firmware', (), ('ssh', 'fwstatus', 'good_au',)),

        (CrosRebootRepair, 'reboot', ('ssh',), ('devmode', 'writable',)),
    )
    return repair_actions


def _cros_extended_repair_actions(au_triggers=_CROS_AU_TRIGGERS,
                                  powerwash_triggers=_CROS_POWERWASH_TRIGGERS,
                                  usb_triggers=_CROS_USB_TRIGGERS):
    """Return the extended repair actions for a `CrosHost`"""

    # The dependencies and triggers for the 'au', 'powerwash', and 'usb'
    # repair actions stack up:  Each one is able to repair progressively
    # more verifiers than the one before.  The 'triggers' lists specify
    # the progression.

    repair_actions = (
        (AutoUpdateRepair, 'au',
                usb_triggers + powerwash_triggers, au_triggers),
        (PowerWashRepair, 'powerwash',
                usb_triggers, powerwash_triggers + au_triggers),
        (ServoInstallRepair, 'usb',
                (), usb_triggers + powerwash_triggers + au_triggers),
    )
    return repair_actions


def _cros_repair_actions():
    """Return the repair actions for a `CrosHost`."""
    repair_actions = (_cros_basic_repair_actions() +
                      _cros_extended_repair_actions())
    return repair_actions


def create_cros_repair_strategy():
    """Return a `RepairStrategy` for a `CrosHost`."""
    verify_dag = _cros_verify_dag()
    repair_actions = _cros_repair_actions()
    return hosts.RepairStrategy(verify_dag, repair_actions)


def _moblab_verify_dag():
    """Return the verification DAG for a `MoblabHost`."""
    FirmwareVersionVerifier = cros_firmware.FirmwareVersionVerifier
    verify_dag = (
        (repair.SshVerifier,         'ssh',     ()),
        (ACPowerVerifier,            'power',   ('ssh',)),
        (FirmwareVersionVerifier,    'rwfw',    ('ssh',)),
        (PythonVerifier,             'python',  ('ssh',)),
        (repair.LegacyHostVerifier,  'cros',    ('ssh',)),
    )
    return verify_dag


def _moblab_repair_actions():
    """Return the repair actions for a `MoblabHost`."""
    repair_actions = (
        (repair.RPMCycleRepair, 'rpm', (), ('ssh', 'power',)),
        (AutoUpdateRepair, 'au', ('ssh',), _CROS_AU_TRIGGERS),
    )
    return repair_actions


def create_moblab_repair_strategy():
    """
    Return a `RepairStrategy` for a `MoblabHost`.

    Moblab is a subset of the CrOS verify and repair.  Several pieces
    are removed because they're not expected to be meaningful.  Some
    others are removed for more specific reasons:

    'tpm':  Moblab DUTs don't run the tests that matter to this
        verifier.  TODO(jrbarnette)  This assertion is unproven.

    'good_au':  This verifier can't pass, because the Moblab AU
        procedure doesn't properly delete CrosHost.PROVISION_FAILED.
        TODO(jrbarnette) We should refactor _machine_install() so that
        it can be different for Moblab.

    'firmware':  Moblab DUTs shouldn't be in FAFT pools, so we don't try
        this.

    'powerwash':  Powerwash on Moblab causes trouble with deleting the
        DHCP leases file, so we skip it.
    """
    verify_dag = _moblab_verify_dag()
    repair_actions = _moblab_repair_actions()
    return hosts.RepairStrategy(verify_dag, repair_actions)


def _jetstream_repair_actions():
    """Return the repair actions for a `JetstreamHost`."""
    au_triggers = _CROS_AU_TRIGGERS + ('jetstream_services',)
    repair_actions = (
        _cros_basic_repair_actions() +
        (
            (JetstreamRepair, 'jetstream_repair',
             _CROS_USB_TRIGGERS + _CROS_POWERWASH_TRIGGERS, au_triggers),
        ) +
        _cros_extended_repair_actions(au_triggers=au_triggers))
    return repair_actions


def _jetstream_verify_dag():
    """Return the verification DAG for a `JetstreamHost`."""
    verify_dag = _cros_verify_dag() + (
        (JetstreamServicesVerifier, 'jetstream_services', ('ssh',)),
    )
    return verify_dag


def create_jetstream_repair_strategy():
    """
    Return a `RepairStrategy` for a `JetstreamHost`.

    The Jetstream repair strategy is based on the CrOS verify and repair,
    but adds the JetstreamServicesVerifier.
    """
    verify_dag = _jetstream_verify_dag()
    repair_actions = _jetstream_repair_actions()
    return hosts.RepairStrategy(verify_dag, repair_actions)


# TODO(pprabhu) Move this to a better place. I have no idea what that place
# would be.
def _is_virual_machine(host):
    """Determine whether the given |host| is a virtual machine.

    @param host: a hosts.Host object.
    @returns True if the host is a virtual machine, False otherwise.
    """
    output = host.run('cat /proc/cpuinfo | grep "model name"')
    return output.stdout and 'qemu' in output.stdout.lower()
