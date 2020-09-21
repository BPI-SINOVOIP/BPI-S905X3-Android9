# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

import common
from autotest_lib.client.common_lib import hosts
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server.hosts import repair


class _UpdateVerifier(hosts.Verifier):
    """
    Verifier to trigger a servo host update, if necessary.

    The operation doesn't wait for the update to complete and is
    considered a success whether or not the servo is currently
    up-to-date.
    """

    def verify(self, host):
        # First, only run this verifier if the host is in the physical lab.
        # Secondly, skip if the test is being run by test_that, because subnet
        # restrictions can cause the update to fail.
        if host.is_in_lab() and host.job and host.job.in_lab:
            host.update_image(wait_for_update=False)

    @property
    def description(self):
        return 'servo host software is up-to-date'


class _ConfigVerifier(hosts.Verifier):
    """
    Base verifier for the servo config file verifiers.
    """

    CONFIG_FILE = '/var/lib/servod/config'
    ATTR = ''

    @staticmethod
    def _get_config_val(host, config_file, attr):
        """
        Get the `attr` for `host` from `config_file`.

        @param host         Host to be checked for `config_file`.
        @param config_file  Path to the config file to be tested.
        @param attr         Attribute to get from config file.

        @return The attr val as set in the config file, or `None` if
                the file was absent.
        """
        getboard = ('CONFIG=%s ; [ -f $CONFIG ] && '
                    '. $CONFIG && echo $%s' % (config_file, attr))
        attr_val = host.run(getboard, ignore_status=True).stdout
        return attr_val.strip('\n') if attr_val else None

    @staticmethod
    def _validate_attr(host, val, expected_val, attr, config_file):
        """
        Check that the attr setting is valid for the host.

        This presupposes that a valid config file was found.  Raise an
        execption if:
          * There was no attr setting from the file (i.e. the setting
            is an empty string), or
          * The attr setting is valid, the attr is known,
            and the setting doesn't match the DUT.

        @param host         Host to be checked for `config_file`.
        @param val          Value to be tested.
        @param expected_val Expected value.
        @param attr         Attribute we're validating.
        @param config_file  Path to the config file to be tested.
        """
        if not val:
            raise hosts.AutoservVerifyError(
                    'config file %s exists, but %s '
                    'is not set' % (attr, config_file))
        if expected_val is not None and val != expected_val:
            raise hosts.AutoservVerifyError(
                    '%s is %s; it should be %s' % (attr, val, expected_val))


    def _get_configs(self, host):
        """
        Return all the config files to check.

        @param host     Host object.

        @return The list of config files to check.
        """
        # TODO(jrbarnette):  Testing `CONFIG_FILE` without a port number
        # is a legacy.  Ideally, we would force all servos in the lab to
        # update, and then remove this case.
        config_list = ['%s_%d' % (self.CONFIG_FILE, host.servo_port)]
        if host.servo_port == host.DEFAULT_PORT:
            config_list.append(self.CONFIG_FILE)
        return config_list

    @property
    def description(self):
        return 'servo %s setting is correct' % self.ATTR


class _SerialConfigVerifier(_ConfigVerifier):
    """
    Verifier for the servo SERIAL configuration.
    """

    ATTR = 'SERIAL'

    def verify(self, host):
        """
        Test whether the `host` has a `SERIAL` setting configured.

        This tests the config file names used by the `servod` upstart
        job for a valid setting of the `SERIAL` variable.  The following
        conditions raise errors:
          * The SERIAL setting doesn't match the DUT's entry in the AFE
            database.
          * There is no config file.
        """
        if not host.is_cros_host():
            return
        # Not all servo hosts will have a servo serial so don't verify if it's
        # not set.
        if host.servo_serial is None:
            return
        for config in self._get_configs(host):
            serialval = self._get_config_val(host, config, self.ATTR)
            if serialval is not None:
                self._validate_attr(host, serialval, host.servo_serial,
                                    self.ATTR, config)
                return
        msg = 'Servo serial is unconfigured; should be %s' % host.servo_serial
        raise hosts.AutoservVerifyError(msg)



class _BoardConfigVerifier(_ConfigVerifier):
    """
    Verifier for the servo BOARD configuration.
    """

    ATTR = 'BOARD'

    def verify(self, host):
        """
        Test whether the `host` has a `BOARD` setting configured.

        This tests the config file names used by the `servod` upstart
        job for a valid setting of the `BOARD` variable.  The following
        conditions raise errors:
          * A config file exists, but the content contains no setting
            for BOARD.
          * The BOARD setting doesn't match the DUT's entry in the AFE
            database.
          * There is no config file.
        """
        if not host.is_cros_host():
            return
        for config in self._get_configs(host):
            boardval = self._get_config_val(host, config, self.ATTR)
            if boardval is not None:
                self._validate_attr(host, boardval, host.servo_board, self.ATTR,
                                    config)
                return
        msg = 'Servo board is unconfigured'
        if host.servo_board is not None:
            msg += '; should be %s' % host.servo_board
        raise hosts.AutoservVerifyError(msg)


class _ServodJobVerifier(hosts.Verifier):
    """
    Verifier to check that the `servod` upstart job is running.
    """

    def verify(self, host):
        if not host.is_cros_host():
            return
        status_cmd = 'status servod PORT=%d' % host.servo_port
        job_status = host.run(status_cmd, ignore_status=True).stdout
        if 'start/running' not in job_status:
            raise hosts.AutoservVerifyError(
                    'servod not running on %s port %d' %
                    (host.hostname, host.servo_port))

    @property
    def description(self):
        return 'servod upstart job is running'


class _ServodConnectionVerifier(hosts.Verifier):
    """
    Verifier to check that we can connect to `servod`.

    This tests the connection to the target servod service with a simple
    method call.  As a side-effect, all servo signals are initialized to
    default values.

    N.B. Initializing servo signals is necessary because the power
    button and lid switch verifiers both test against expected initial
    values.
    """

    def verify(self, host):
        host.connect_servo()

    @property
    def description(self):
        return 'servod service is taking calls'


class _PowerButtonVerifier(hosts.Verifier):
    """
    Verifier to check sanity of the `pwr_button` signal.

    Tests that the `pwr_button` signal shows the power button has been
    released.  When `pwr_button` is stuck at `press`, it commonly
    indicates that the ribbon cable is disconnected.
    """
    # TODO (crbug.com/646593) - Remove list below once servo has been updated
    # with a dummy pwr_button signal.
    _BOARDS_WO_PWR_BUTTON = ['arkham', 'storm', 'whirlwind', 'gale']

    def verify(self, host):
        if host.servo_board in self._BOARDS_WO_PWR_BUTTON:
            return
        button = host.get_servo().get('pwr_button')
        if button != 'release':
            raise hosts.AutoservVerifyError(
                    'Check ribbon cable: \'pwr_button\' is stuck')

    @property
    def description(self):
        return 'pwr_button control is normal'


class _LidVerifier(hosts.Verifier):
    """
    Verifier to check sanity of the `lid_open` signal.
    """

    def verify(self, host):
        lid_open = host.get_servo().get('lid_open')
        if lid_open != 'yes' and lid_open != 'not_applicable':
            raise hosts.AutoservVerifyError(
                    'Check lid switch: lid_open is %s' % lid_open)

    @property
    def description(self):
        return 'lid_open control is normal'


class _RestartServod(hosts.RepairAction):
    """Restart `servod` with the proper BOARD setting."""

    def repair(self, host):
        if not host.is_cros_host():
            raise hosts.AutoservRepairError(
                    'Can\'t restart servod: not running '
                    'embedded Chrome OS.')
        host.run('stop servod PORT=%d || true' % host.servo_port)
        serial = 'SERIAL=%s' % host.servo_serial if host.servo_serial else ''
        if host.servo_board:
            host.run('start servod BOARD=%s PORT=%d %s' %
                     (host.servo_board, host.servo_port, serial))
        else:
            # TODO(jrbarnette):  It remains to be seen whether
            # this action is the right thing to do...
            logging.warning('Board for DUT is unknown; starting '
                            'servod assuming a pre-configured '
                            'board.')
            host.run('start servod PORT=%d %s' % (host.servo_port, serial))
        # There's a lag between when `start servod` completes and when
        # the _ServodConnectionVerifier trigger can actually succeed.
        # The call to time.sleep() below gives time to make sure that
        # the trigger won't fail after we return.
        #
        # The delay selection was based on empirical testing against
        # servo V3 on a desktop:
        #   + 10 seconds was usually too slow; 11 seconds was
        #     usually fast enough.
        #   + So, the 20 second delay is about double what we
        #     expect to need.
        time.sleep(20)


    @property
    def description(self):
        return 'Start servod with the proper config settings.'


class _ServoRebootRepair(repair.RebootRepair):
    """
    Reboot repair action that also waits for an update.

    This is the same as the standard `RebootRepair`, but for
    a servo host, if there's a pending update, we wait for that
    to complete before rebooting.  This should ensure that the
    servo is up-to-date after reboot.
    """

    def repair(self, host):
        if host.is_localhost() or not host.is_cros_host():
            raise hosts.AutoservRepairError(
                'Target servo is not a test lab servo')
        host.update_image(wait_for_update=True)
        afe = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10)
        dut_list = host.get_attached_duts(afe)
        if len(dut_list) > 1:
            host.schedule_synchronized_reboot(dut_list, afe, force_reboot=True)
        else:
            super(_ServoRebootRepair, self).repair(host)

    @property
    def description(self):
        return 'Wait for update, then reboot servo host.'


class _DutRebootRepair(hosts.RepairAction):
    """
    Reboot DUT to recover some servo controls depending on EC console.

    Some servo controls, like lid_open, requires communicating with DUT through
    EC UART console. Failure of this kinds of controls can be recovered by
    rebooting the DUT.
    """

    def repair(self, host):
        host.get_servo().get_power_state_controller().reset()
        # Get the lid_open value which requires EC console.
        lid_open = host.get_servo().get('lid_open')
        if lid_open != 'yes' and lid_open != 'not_applicable':
            raise hosts.AutoservVerifyError(
                    'Still fail to contact EC console after rebooting DUT')

    @property
    def description(self):
        return 'Reset the DUT via servo'


def create_servo_repair_strategy():
    """
    Return a `RepairStrategy` for a `ServoHost`.
    """
    config = ['brd_config', 'ser_config']
    verify_dag = [
        (repair.SshVerifier,         'servo_ssh',   []),
        (_UpdateVerifier,            'update',      ['servo_ssh']),
        (_BoardConfigVerifier,       'brd_config',  ['servo_ssh']),
        (_SerialConfigVerifier,      'ser_config',  ['servo_ssh']),
        (_ServodJobVerifier,         'job',         config),
        (_ServodConnectionVerifier,  'servod',      ['job']),
        (_PowerButtonVerifier,       'pwr_button',  ['servod']),
        (_LidVerifier,               'lid_open',    ['servod']),
        # TODO(jrbarnette):  We want a verifier for whether there's
        # a working USB stick plugged into the servo.  However,
        # although we always want to log USB stick problems, we don't
        # want to fail the servo because we don't want a missing USB
        # stick to prevent, say, power cycling the DUT.
        #
        # So, it may be that the right fix is to put diagnosis into
        # ServoInstallRepair rather than add a verifier.
    ]

    servod_deps = ['job', 'servod', 'pwr_button']
    repair_actions = [
        (repair.RPMCycleRepair, 'rpm', [], ['servo_ssh']),
        (_RestartServod, 'restart', ['servo_ssh'], config + servod_deps),
        (_ServoRebootRepair, 'servo_reboot', ['servo_ssh'], servod_deps),
        (_DutRebootRepair, 'dut_reboot', ['servod'], ['lid_open']),
    ]
    return hosts.RepairStrategy(verify_dag, repair_actions)
