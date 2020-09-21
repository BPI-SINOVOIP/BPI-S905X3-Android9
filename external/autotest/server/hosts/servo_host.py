# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Expects to be run in an environment with sudo and no interactive password
# prompt, such as within the Chromium OS development chroot.


"""This file provides core logic for servo verify/repair process."""


import httplib
import logging
import socket
import xmlrpclib

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import control_data
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import host_states
from autotest_lib.client.common_lib import hosts
from autotest_lib.client.common_lib import lsbrelease_utils
from autotest_lib.client.common_lib.cros import autoupdater
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.client.common_lib.cros.network import ping_runner
from autotest_lib.client.cros import constants as client_constants
from autotest_lib.server import afe_utils
from autotest_lib.server import site_utils as server_site_utils
from autotest_lib.server.cros import dnsname_mangler
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server.cros.dynamic_suite import control_file_getter
from autotest_lib.server.cros.servo import servo
from autotest_lib.server.hosts import servo_repair
from autotest_lib.server.hosts import ssh_host
from autotest_lib.site_utils.rpm_control_system import rpm_client

try:
    from chromite.lib import metrics
except ImportError:
    metrics = utils.metrics_mock


# Names of the host attributes in the database that represent the values for
# the servo_host and servo_port for a servo connected to the DUT.
SERVO_HOST_ATTR = 'servo_host'
SERVO_PORT_ATTR = 'servo_port'
SERVO_BOARD_ATTR = 'servo_board'
SERVO_SERIAL_ATTR = 'servo_serial'

_CONFIG = global_config.global_config
ENABLE_SSH_TUNNEL_FOR_SERVO = _CONFIG.get_config_value(
        'CROS', 'enable_ssh_tunnel_for_servo', type=bool, default=False)

AUTOTEST_BASE = _CONFIG.get_config_value(
        'SCHEDULER', 'drone_installation_directory',
        default='/usr/local/autotest')

_SERVO_HOST_REBOOT_TEST_NAME = 'servohost_Reboot'
_SERVO_HOST_FORCE_REBOOT_TEST_NAME = 'servohost_Reboot.force_reboot'

class ServoHost(ssh_host.SSHHost):
    """Host class for a host that controls a servo, e.g. beaglebone."""

    DEFAULT_PORT = 9999

    # Timeout for initializing servo signals.
    INITIALIZE_SERVO_TIMEOUT_SECS = 60

    # Ready test function
    SERVO_READY_METHOD = 'get_version'

    REBOOT_CMD = 'sleep 1; reboot & sleep 10; reboot -f'


    def _initialize(self, servo_host='localhost',
                    servo_port=DEFAULT_PORT, servo_board=None,
                    servo_serial=None, is_in_lab=None, *args, **dargs):
        """Initialize a ServoHost instance.

        A ServoHost instance represents a host that controls a servo.

        @param servo_host: Name of the host where the servod process
                           is running.
        @param servo_port: Port the servod process is listening on.
        @param servo_board: Board that the servo is connected to.
        @param is_in_lab: True if the servo host is in Cros Lab. Default is set
                          to None, for which utils.host_is_in_lab_zone will be
                          called to check if the servo host is in Cros lab.

        """
        super(ServoHost, self)._initialize(hostname=servo_host,
                                           *args, **dargs)
        self.servo_port = servo_port
        self.servo_board = servo_board
        self.servo_serial = servo_serial
        self._servo = None
        self._repair_strategy = (
                servo_repair.create_servo_repair_strategy())
        self._is_localhost = (self.hostname == 'localhost')
        if self._is_localhost:
            self._is_in_lab = False
        elif is_in_lab is None:
            self._is_in_lab = utils.host_is_in_lab_zone(self.hostname)
        else:
            self._is_in_lab = is_in_lab

        # Commands on the servo host must be run by the superuser.
        # Our account on a remote host is root, but if our target is
        # localhost then we might be running unprivileged.  If so,
        # `sudo` will have to be added to the commands.
        if self._is_localhost:
            self._sudo_required = utils.system_output('id -u') != '0'
        else:
            self._sudo_required = False


    def connect_servo(self):
        """Establish a connection to the servod server on this host.

        Initializes `self._servo` and then verifies that all network
        connections are working.  This will create an ssh tunnel if
        it's required.

        As a side effect of testing the connection, all signals on the
        target servo are reset to default values, and the USB stick is
        set to the neutral (off) position.
        """
        servo_obj = servo.Servo(servo_host=self, servo_serial=self.servo_serial)
        timeout, _ = retry.timeout(
                servo_obj.initialize_dut,
                timeout_sec=self.INITIALIZE_SERVO_TIMEOUT_SECS)
        if timeout:
            raise hosts.AutoservVerifyError(
                    'Servo initialize timed out.')
        self._servo = servo_obj


    def disconnect_servo(self):
        """Disconnect our servo if it exists.

        If we've previously successfully connected to our servo,
        disconnect any established ssh tunnel, and set `self._servo`
        back to `None`.
        """
        if self._servo:
            # N.B. This call is safe even without a tunnel:
            # rpc_server_tracker.disconnect() silently ignores
            # unknown ports.
            self.rpc_server_tracker.disconnect(self.servo_port)
            self._servo = None


    def is_in_lab(self):
        """Check whether the servo host is a lab device.

        @returns: True if the servo host is in Cros Lab, otherwise False.

        """
        return self._is_in_lab


    def is_localhost(self):
        """Checks whether the servo host points to localhost.

        @returns: True if it points to localhost, otherwise False.

        """
        return self._is_localhost


    def get_servod_server_proxy(self):
        """Return a proxy that can be used to communicate with servod server.

        @returns: An xmlrpclib.ServerProxy that is connected to the servod
                  server on the host.
        """
        if ENABLE_SSH_TUNNEL_FOR_SERVO and not self.is_localhost():
            return self.rpc_server_tracker.xmlrpc_connect(
                    None, self.servo_port,
                    ready_test_name=self.SERVO_READY_METHOD,
                    timeout_seconds=60)
        else:
            remote = 'http://%s:%s' % (self.hostname, self.servo_port)
            return xmlrpclib.ServerProxy(remote)


    def is_cros_host(self):
        """Check if a servo host is running chromeos.

        @return: True if the servo host is running chromeos.
            False if it isn't, or we don't have enough information.
        """
        try:
            result = self.run('grep -q CHROMEOS /etc/lsb-release',
                              ignore_status=True, timeout=10)
        except (error.AutoservRunError, error.AutoservSSHTimeout):
            return False
        return result.exit_status == 0


    def make_ssh_command(self, user='root', port=22, opts='', hosts_file=None,
                         connect_timeout=None, alive_interval=None,
                         alive_count_max=None, connection_attempts=None):
        """Override default make_ssh_command to use tuned options.

        Tuning changes:
          - ConnectTimeout=30; maximum of 30 seconds allowed for an SSH
          connection failure. Consistency with remote_access.py.

          - ServerAliveInterval=180; which causes SSH to ping connection every
          180 seconds. In conjunction with ServerAliveCountMax ensures
          that if the connection dies, Autotest will bail out quickly.

          - ServerAliveCountMax=3; consistency with remote_access.py.

          - ConnectAttempts=4; reduce flakiness in connection errors;
          consistency with remote_access.py.

          - UserKnownHostsFile=/dev/null; we don't care about the keys.

          - SSH protocol forced to 2; needed for ServerAliveInterval.

        @param user User name to use for the ssh connection.
        @param port Port on the target host to use for ssh connection.
        @param opts Additional options to the ssh command.
        @param hosts_file Ignored.
        @param connect_timeout Ignored.
        @param alive_interval Ignored.
        @param alive_count_max Ignored.
        @param connection_attempts Ignored.

        @returns: An ssh command with the requested settings.

        """
        options = ' '.join([opts, '-o Protocol=2'])
        return super(ServoHost, self).make_ssh_command(
            user=user, port=port, opts=options, hosts_file='/dev/null',
            connect_timeout=30, alive_interval=180, alive_count_max=3,
            connection_attempts=4)


    def _make_scp_cmd(self, sources, dest):
        """Format scp command.

        Given a list of source paths and a destination path, produces the
        appropriate scp command for encoding it. Remote paths must be
        pre-encoded. Overrides _make_scp_cmd in AbstractSSHHost
        to allow additional ssh options.

        @param sources: A list of source paths to copy from.
        @param dest: Destination path to copy to.

        @returns: An scp command that copies |sources| on local machine to
                  |dest| on the remote servo host.

        """
        command = ('scp -rq %s -o BatchMode=yes -o StrictHostKeyChecking=no '
                   '-o UserKnownHostsFile=/dev/null -P %d %s "%s"')
        return command % (self.master_ssh_option,
                          self.port, ' '.join(sources), dest)


    def run(self, command, timeout=3600, ignore_status=False,
            stdout_tee=utils.TEE_TO_LOGS, stderr_tee=utils.TEE_TO_LOGS,
            connect_timeout=30, ssh_failure_retry_ok=False,
            options='', stdin=None, verbose=True, args=()):
        """Run a command on the servo host.

        Extends method `run` in SSHHost. If the servo host is a remote device,
        it will call `run` in SSHost without changing anything.
        If the servo host is 'localhost', it will call utils.system_output.

        @param command: The command line string.
        @param timeout: Time limit in seconds before attempting to
                        kill the running process. The run() function
                        will take a few seconds longer than 'timeout'
                        to complete if it has to kill the process.
        @param ignore_status: Do not raise an exception, no matter
                              what the exit code of the command is.
        @param stdout_tee/stderr_tee: Where to tee the stdout/stderr.
        @param connect_timeout: SSH connection timeout (in seconds)
                                Ignored if host is 'localhost'.
        @param options: String with additional ssh command options
                        Ignored if host is 'localhost'.
        @param ssh_failure_retry_ok: when True and ssh connection failure is
                                     suspected, OK to retry command (but not
                                     compulsory, and likely not needed here)
        @param stdin: Stdin to pass (a string) to the executed command.
        @param verbose: Log the commands.
        @param args: Sequence of strings to pass as arguments to command by
                     quoting them in " and escaping their contents if necessary.

        @returns: A utils.CmdResult object.

        @raises AutoservRunError if the command failed.
        @raises AutoservSSHTimeout SSH connection has timed out. Only applies
                when servo host is not 'localhost'.

        """
        run_args = {'command': command, 'timeout': timeout,
                    'ignore_status': ignore_status, 'stdout_tee': stdout_tee,
                    'stderr_tee': stderr_tee, 'stdin': stdin,
                    'verbose': verbose, 'args': args}
        if self.is_localhost():
            if self._sudo_required:
                run_args['command'] = 'sudo -n sh -c "%s"' % utils.sh_escape(
                        command)
            try:
                return utils.run(**run_args)
            except error.CmdError as e:
                logging.error(e)
                raise error.AutoservRunError('command execution error',
                                             e.result_obj)
        else:
            run_args['connect_timeout'] = connect_timeout
            run_args['options'] = options
            return super(ServoHost, self).run(**run_args)


    def _get_release_version(self):
        """Get the value of attribute CHROMEOS_RELEASE_VERSION from lsb-release.

        @returns The version string in lsb-release, under attribute
                 CHROMEOS_RELEASE_VERSION.
        """
        lsb_release_content = self.run(
                    'cat "%s"' % client_constants.LSB_RELEASE).stdout.strip()
        return lsbrelease_utils.get_chromeos_release_version(
                    lsb_release_content=lsb_release_content)


    def get_attached_duts(self, afe):
        """Gather a list of duts that use this servo host.

        @param afe: afe instance.

        @returns list of duts.
        """
        return afe.get_hosts_by_attribute(
                attribute=SERVO_HOST_ATTR, value=self.hostname)


    def get_board(self):
        """Determine the board for this servo host.

        @returns a string representing this servo host's board.
        """
        return lsbrelease_utils.get_current_board(
                lsb_release_content=self.run('cat /etc/lsb-release').stdout)


    def _choose_dut_for_synchronized_reboot(self, dut_list, afe):
        """Choose which dut to schedule servo host reboot job.

        We'll want a semi-deterministic way of selecting which host should be
        scheduled for the servo host reboot job.  For now we'll sort the
        list with the expectation the dut list will stay consistent.
        From there we'll grab the first dut that is available so we
        don't schedule a job on a dut that will never run.

        @param dut_list:  List of the dut hostnames to choose from.
        @param afe:       Instance of the AFE.

        @return hostname of dut to schedule job on.
        """
        afe_hosts = afe.get_hosts(dut_list)
        afe_hosts.sort()
        for afe_host in afe_hosts:
            if afe_host.status not in host_states.UNAVAILABLE_STATES:
                return afe_host.hostname
        # If they're all unavailable, just return the first sorted dut.
        dut_list.sort()
        return dut_list[0]


    def _sync_job_scheduled_for_duts(self, dut_list, afe):
        """Checks if a synchronized reboot has been scheduled for these duts.

        Grab all the host queue entries that aren't completed for the duts and
        see if any of them have the expected job name.

        @param dut_list:  List of duts to check on.
        @param afe:       Instance of the AFE.

        @returns True if the job is scheduled, False otherwise.
        """
        afe_hosts = afe.get_hosts(dut_list)
        for afe_host in afe_hosts:
            hqes = afe.get_host_queue_entries(host=afe_host.id, complete=0)
            for hqe in hqes:
                job = afe.get_jobs(id=hqe.job.id)
                if job and job[0].name in (_SERVO_HOST_REBOOT_TEST_NAME,
                                           _SERVO_HOST_FORCE_REBOOT_TEST_NAME):
                    return True
        return False


    def schedule_synchronized_reboot(self, dut_list, afe, force_reboot=False):
        """Schedule a job to reboot the servo host.

        When we schedule a job, it will create a ServoHost object which will
        go through this entire flow of checking if a reboot is needed and
        trying to schedule it.  There is probably a better approach to setting
        up a synchronized reboot but I'm coming up short on better ideas so I
        apologize for this circus show.

        @param dut_list:      List of duts that need to be locked.
        @param afe:           Instance of afe.
        @param force_reboot:  Boolean to indicate if a forced reboot should be
                              scheduled or not.
        """
        # If we've already scheduled job on a dut, we're done here.
        if self._sync_job_scheduled_for_duts(dut_list, afe):
            return

        # Looks like we haven't scheduled a job yet.
        test = (_SERVO_HOST_REBOOT_TEST_NAME if not force_reboot
                else _SERVO_HOST_FORCE_REBOOT_TEST_NAME)
        dut = self._choose_dut_for_synchronized_reboot(dut_list, afe)
        getter = control_file_getter.FileSystemGetter([AUTOTEST_BASE])
        control_file = getter.get_control_file_contents_by_name(test)
        control_type = control_data.CONTROL_TYPE_NAMES.SERVER
        try:
            afe.create_job(control_file=control_file, name=test,
                           control_type=control_type, hosts=[dut])
        except Exception as e:
            # Sometimes creating the job will raise an exception. We'll log it
            # but we don't want to fail because of it.
            logging.exception('Scheduling reboot job failed due to Exception.')


    def reboot(self, *args, **dargs):
        """Reboot using special servo host reboot command."""
        super(ServoHost, self).reboot(reboot_cmd=self.REBOOT_CMD,
                                      *args, **dargs)


    def _check_for_reboot(self, updater):
        """Reboot this servo host if an upgrade is waiting.

        If the host has successfully downloaded and finalized a new
        build, reboot.

        @param updater: a ChromiumOSUpdater instance for checking
            whether reboot is needed.
        @return Return a (status, build) tuple reflecting the
            update_engine status and current build of the host
            at the end of the call.
        """
        current_build_number = self._get_release_version()
        status = updater.check_update_status()
        if status == autoupdater.UPDATER_NEED_REBOOT:
            # Check if we need to schedule an organized reboot.
            afe = frontend_wrappers.RetryingAFE(
                    timeout_min=5, delay_sec=10,
                    server=server_site_utils.get_global_afe_hostname())
            dut_list = self.get_attached_duts(afe)
            logging.info('servo host has the following duts: %s', dut_list)
            if len(dut_list) > 1:
                logging.info('servo host has multiple duts, scheduling '
                             'synchronized reboot')
                self.schedule_synchronized_reboot(dut_list, afe)
                return status, current_build_number

            logging.info('Rebooting servo host %s from build %s',
                         self.hostname, current_build_number)
            # Tell the reboot() call not to wait for completion.
            # Otherwise, the call will log reboot failure if servo does
            # not come back.  The logged reboot failure will lead to
            # test job failure.  If the test does not require servo, we
            # don't want servo failure to fail the test with error:
            # `Host did not return from reboot` in status.log.
            self.reboot(fastsync=True, wait=False)

            # We told the reboot() call not to wait, but we need to wait
            # for the reboot before we continue.  Alas.  The code from
            # here below is basically a copy of Host.wait_for_restart(),
            # with the logging bits ripped out, so that they can't cause
            # the failure logging problem described above.
            #
            # The black stain that this has left on my soul can never be
            # erased.
            old_boot_id = self.get_boot_id()
            if not self.wait_down(timeout=self.WAIT_DOWN_REBOOT_TIMEOUT,
                                  warning_timer=self.WAIT_DOWN_REBOOT_WARNING,
                                  old_boot_id=old_boot_id):
                raise error.AutoservHostError(
                        'servo host %s failed to shut down.' %
                        self.hostname)
            if self.wait_up(timeout=120):
                current_build_number = self._get_release_version()
                status = updater.check_update_status()
                logging.info('servo host %s back from reboot, with build %s',
                             self.hostname, current_build_number)
            else:
                raise error.AutoservHostError(
                        'servo host %s failed to come back from reboot.' %
                        self.hostname)
        return status, current_build_number


    def update_image(self, wait_for_update=False):
        """Update the image on the servo host, if needed.

        This method recognizes the following cases:
          * If the Host is not running Chrome OS, do nothing.
          * If a previously triggered update is now complete, reboot
            to the new version.
          * If the host is processing a previously triggered update,
            do nothing.
          * If the host is running a version of Chrome OS different
            from the default for servo Hosts, trigger an update, but
            don't wait for it to complete.

        @param wait_for_update If an update needs to be applied and
            this is true, then don't return until the update is
            downloaded and finalized, and the host rebooted.
        @raises dev_server.DevServerException: If all the devservers are down.
        @raises site_utils.ParseBuildNameException: If the devserver returns
            an invalid build name.
        @raises autoupdater.ChromiumOSError: If something goes wrong in the
            checking update engine client status or applying an update.
        @raises AutoservRunError: If the update_engine_client isn't present on
            the host, and the host is a cros_host.

        """
        # servod could be running in a Ubuntu workstation.
        if not self.is_cros_host():
            logging.info('Not attempting an update, either %s is not running '
                         'chromeos or we cannot find enough information about '
                         'the host.', self.hostname)
            return

        if lsbrelease_utils.is_moblab():
            logging.info('Not attempting an update, %s is running moblab.',
                         self.hostname)
            return

        target_build = afe_utils.get_stable_cros_image_name(self.get_board())
        target_build_number = server_site_utils.ParseBuildName(
                target_build)[3]
        # For servo image staging, we want it as more widely distributed as
        # possible, so that devservers' load can be evenly distributed. So use
        # hostname instead of target_build as hash.
        ds = dev_server.ImageServer.resolve(self.hostname,
                                            hostname=self.hostname)
        url = ds.get_update_url(target_build)

        updater = autoupdater.ChromiumOSUpdater(update_url=url, host=self)
        status, current_build_number = self._check_for_reboot(updater)
        update_pending = True
        if status in autoupdater.UPDATER_PROCESSING_UPDATE:
            logging.info('servo host %s already processing an update, update '
                         'engine client status=%s', self.hostname, status)
        elif status == autoupdater.UPDATER_NEED_REBOOT:
            return
        elif current_build_number != target_build_number:
            logging.info('Using devserver url: %s to trigger update on '
                         'servo host %s, from %s to %s', url, self.hostname,
                         current_build_number, target_build_number)
            try:
                ds.stage_artifacts(target_build,
                                   artifacts=['full_payload'])
            except Exception as e:
                logging.error('Staging artifacts failed: %s', str(e))
                logging.error('Abandoning update for this cycle.')
            else:
                try:
                    # TODO(jrbarnette): This 'touch' is a gross hack
                    # to get us past crbug.com/613603.  Once that
                    # bug is resolved, we should remove this code.
                    self.run('touch /home/chronos/.oobe_completed')
                    updater.trigger_update()
                except autoupdater.RootFSUpdateError as e:
                    trigger_download_status = 'failed with %s' % str(e)
                    metrics.Counter('chromeos/autotest/servo/'
                                    'rootfs_update_failed').increment()
                else:
                    trigger_download_status = 'passed'
                logging.info('Triggered download and update %s for %s, '
                             'update engine currently in status %s',
                             trigger_download_status, self.hostname,
                             updater.check_update_status())
        else:
            logging.info('servo host %s does not require an update.',
                         self.hostname)
            update_pending = False

        if update_pending and wait_for_update:
            logging.info('Waiting for servo update to complete.')
            self.run('update_engine_client --follow', ignore_status=True)


    def verify(self, silent=False):
        """Update the servo host and verify it's in a good state.

        @param silent   If true, suppress logging in `status.log`.
        """
        # TODO(jrbarnette) Old versions of beaglebone_servo include
        # the powerd package.  If you touch the .oobe_completed file
        # (as we do to work around an update_engine problem), then
        # powerd will eventually shut down the beaglebone for lack
        # of (apparent) activity.  Current versions of
        # beaglebone_servo don't have powerd, but until we can purge
        # the lab of the old images, we need to make sure powerd
        # isn't running.
        self.run('stop powerd', ignore_status=True)
        try:
            self._repair_strategy.verify(self, silent)
        except:
            self.disconnect_servo()
            raise


    def repair(self, silent=False):
        """Attempt to repair servo host.

        @param silent   If true, suppress logging in `status.log`.
        """
        try:
            self._repair_strategy.repair(self, silent)
        except:
            self.disconnect_servo()
            raise


    def has_power(self):
        """Return whether or not the servo host is powered by PoE."""
        # TODO(fdeng): See crbug.com/302791
        # For now, assume all servo hosts in the lab have power.
        return self.is_in_lab()


    def power_cycle(self):
        """Cycle power to this host via PoE if it is a lab device.

        @raises AutoservRepairError if it fails to power cycle the
                servo host.

        """
        if self.has_power():
            try:
                rpm_client.set_power(self.hostname, 'CYCLE')
            except (socket.error, xmlrpclib.Error,
                    httplib.BadStatusLine,
                    rpm_client.RemotePowerException) as e:
                raise hosts.AutoservRepairError(
                        'Power cycling %s failed: %s' % (self.hostname, e))
        else:
            logging.info('Skipping power cycling, not a lab device.')


    def get_servo(self):
        """Get the cached servo.Servo object.

        @return: a servo.Servo object.
        """
        return self._servo


def make_servo_hostname(dut_hostname):
    """Given a DUT's hostname, return the hostname of its servo.

    @param dut_hostname: hostname of a DUT.

    @return hostname of the DUT's servo.

    """
    host_parts = dut_hostname.split('.')
    host_parts[0] = host_parts[0] + '-servo'
    return '.'.join(host_parts)


def servo_host_is_up(servo_hostname):
    """Given a servo host name, return if it's up or not.

    @param servo_hostname: hostname of the servo host.

    @return True if it's up, False otherwise
    """
    # Technically, this duplicates the SSH ping done early in the servo
    # proxy initialization code.  However, this ping ends in a couple
    # seconds when if fails, rather than the 60 seconds it takes to decide
    # that an SSH ping has timed out.  Specifically, that timeout happens
    # when our servo DNS name resolves, but there is no host at that IP.
    logging.info('Pinging servo host at %s', servo_hostname)
    ping_config = ping_runner.PingConfig(
            servo_hostname, count=3,
            ignore_result=True, ignore_status=True)
    return ping_runner.PingRunner().ping(ping_config).received > 0


def _map_afe_board_to_servo_board(afe_board):
    """Map a board we get from the AFE to a servo appropriate value.

    Many boards are identical to other boards for servo's purposes.
    This function makes that mapping.

    @param afe_board string board name received from AFE.
    @return board we expect servo to have.

    """
    KNOWN_SUFFIXES = ['-freon', '_freon', '_moblab', '-cheets']
    BOARD_MAP = {'gizmo': 'panther'}
    mapped_board = afe_board
    if afe_board in BOARD_MAP:
        mapped_board = BOARD_MAP[afe_board]
    else:
        for suffix in KNOWN_SUFFIXES:
            if afe_board.endswith(suffix):
                mapped_board = afe_board[0:-len(suffix)]
                break
    if mapped_board != afe_board:
        logging.info('Mapping AFE board=%s to %s', afe_board, mapped_board)
    return mapped_board


def _get_standard_servo_args(dut_host):
    """Return servo data associated with a given DUT.

    This checks for the presence of servo host and port attached to the
    given `dut_host`.  This data should be stored in the
    `_afe_host.attributes` field in the provided `dut_host` parameter.

    @param dut_host   Instance of `Host` on which to find the servo
                      attributes.
    @return A tuple of `servo_args` dict with host and an option port,
            plus an `is_in_lab` flag indicating whether this in the CrOS
            test lab, or some different environment.
    """
    servo_args = None
    is_in_lab = False
    is_ssp_moblab = False
    if utils.is_in_container():
        is_moblab = _CONFIG.get_config_value(
                'SSP', 'is_moblab', type=bool, default=False)
        is_ssp_moblab = is_moblab
    else:
        is_moblab = utils.is_moblab()
    attrs = dut_host._afe_host.attributes
    if attrs and SERVO_HOST_ATTR in attrs:
        servo_host = attrs[SERVO_HOST_ATTR]
        if (is_ssp_moblab and servo_host in ['localhost', '127.0.0.1']):
            servo_host = _CONFIG.get_config_value(
                    'SSP', 'host_container_ip', type=str, default=None)
        servo_args = {SERVO_HOST_ATTR: servo_host}
        if SERVO_PORT_ATTR in attrs:
            try:
                servo_port = attrs[SERVO_PORT_ATTR]
                servo_args[SERVO_PORT_ATTR] = int(servo_port)
            except ValueError:
                logging.error('servo port is not an int: %s', servo_port)
                # Let's set the servo args to None since we're not creating
                # the ServoHost object with the proper port now.
                servo_args = None
        if SERVO_SERIAL_ATTR in attrs:
            servo_args[SERVO_SERIAL_ATTR] = attrs[SERVO_SERIAL_ATTR]
        is_in_lab = (not is_moblab
                     and utils.host_is_in_lab_zone(servo_host))

    # TODO(jrbarnette):  This test to use the default lab servo hostname
    # is a legacy that we need only until every host in the DB has
    # proper attributes.
    elif (not is_moblab and
            not dnsname_mangler.is_ip_address(dut_host.hostname)):
        servo_host = make_servo_hostname(dut_host.hostname)
        is_in_lab = utils.host_is_in_lab_zone(servo_host)
        if is_in_lab:
            servo_args = {SERVO_HOST_ATTR: servo_host}
    if servo_args is not None:
        info = dut_host.host_info_store.get()
        if info.board:
            servo_args[SERVO_BOARD_ATTR] = _map_afe_board_to_servo_board(
                    info.board)
    return servo_args, is_in_lab


def create_servo_host(dut, servo_args, try_lab_servo=False,
                      try_servo_repair=False):
    """Create a ServoHost object for a given DUT, if appropriate.

    This function attempts to create and verify or repair a `ServoHost`
    object for a servo connected to the given `dut`, subject to various
    constraints imposed by the parameters:
      * When the `servo_args` parameter is not `None`, a servo
        host must be created, and must be checked with `repair()`.
      * Otherwise, if a servo exists in the lab and `try_lab_servo` is
        true:
          * If `try_servo_repair` is true, then create a servo host and
            check it with `repair()`.
          * Otherwise, if the servo responds to `ping` then create a
            servo host and check it with `verify()`.

    In cases where `servo_args` was not `None`, repair failure
    exceptions are passed back to the caller; otherwise, exceptions
    are logged and then discarded.  Note that this only happens in cases
    where we're called from a test (not special task) control file that
    has an explicit dependency on servo.  In that case, we require that
    repair not write to `status.log`, so as to avoid polluting test
    results.

    TODO(jrbarnette):  The special handling for servo in test control
    files is a thorn in my flesh; I dearly hope to see it cut out before
    my retirement.

    Parameters for a servo host consist of a host name, port number, and
    DUT board, and are determined from one of these sources, in order of
    priority:
      * Servo attributes from the `dut` parameter take precedence over
        all other sources of information.
      * If a DNS entry for the servo based on the DUT hostname exists in
        the CrOS lab network, that hostname is used with the default
        port and the DUT's board.
      * If no other options are found, the parameters will be taken
        from the `servo_args` dict passed in from the caller.

    @param dut            An instance of `Host` from which to take
                          servo parameters (if available).
    @param servo_args     A dictionary with servo parameters to use if
                          they can't be found from `dut`.  If this
                          argument is supplied, unrepaired exceptions
                          from `verify()` will be passed back to the
                          caller.
    @param try_lab_servo  If not true, servo host creation will be
                          skipped unless otherwise required by the
                          caller.
    @param try_servo_repair  If true, check a servo host with
                          `repair()` instead of `verify()`.

    @returns: A ServoHost object or None. See comments above.

    """
    servo_dependency = servo_args is not None
    is_in_lab = False
    if dut is not None and (try_lab_servo or servo_dependency):
        servo_args_override, is_in_lab = _get_standard_servo_args(dut)
        if servo_args_override is not None:
            servo_args = servo_args_override
    if servo_args is None:
        return None
    if (not servo_dependency and not try_servo_repair and
            not servo_host_is_up(servo_args[SERVO_HOST_ATTR])):
        return None
    newhost = ServoHost(is_in_lab=is_in_lab, **servo_args)
    # Note that the logic of repair() includes everything done
    # by verify().  It's sufficient to call one or the other;
    # we don't need both.
    if servo_dependency:
        newhost.repair(silent=True)
    else:
        try:
            if try_servo_repair:
                newhost.repair()
            else:
                newhost.verify()
        except Exception:
            operation = 'repair' if try_servo_repair else 'verification'
            logging.exception('Servo %s failed for %s',
                              operation, newhost.hostname)
    return newhost
