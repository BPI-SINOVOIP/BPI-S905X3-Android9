# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import errno
import functools
import logging
import os
import re
import signal
import stat
import sys
import time

import common

from autotest_lib.client.bin import utils as client_utils
from autotest_lib.client.common_lib import android_utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.server import autoserv_parser
from autotest_lib.server import constants as server_constants
from autotest_lib.server import utils
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import tools
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.hosts import abstract_ssh
from autotest_lib.server.hosts import adb_label
from autotest_lib.server.hosts import base_label
from autotest_lib.server.hosts import host_info
from autotest_lib.server.hosts import teststation_host


CONFIG = global_config.global_config

ADB_CMD = 'adb'
FASTBOOT_CMD = 'fastboot'
SHELL_CMD = 'shell'
# Some devices have no serial, then `adb serial` has output such as:
# (no serial number)  device
# ??????????          device
DEVICE_NO_SERIAL_MSG = '(no serial number)'
DEVICE_NO_SERIAL_TAG = '<NO_SERIAL>'
# Regex to find an adb device. Examples:
# 0146B5580B01801B    device
# 018e0ecb20c97a62    device
# 172.22.75.141:5555  device
# localhost:22        device
DEVICE_FINDER_REGEX = (r'^(?P<SERIAL>([\w-]+)|((tcp:)?' +
                       '\d{1,3}.\d{1,3}.\d{1,3}.\d{1,3}([:]5555)?)|' +
                       '((tcp:)?localhost([:]22)?)|' +
                       re.escape(DEVICE_NO_SERIAL_MSG) +
                       r')[ \t]+(?:device|fastboot)')
CMD_OUTPUT_PREFIX = 'ADB_CMD_OUTPUT'
CMD_OUTPUT_REGEX = ('(?P<OUTPUT>[\s\S]*)%s:(?P<EXIT_CODE>\d{1,3})' %
                    CMD_OUTPUT_PREFIX)
RELEASE_FILE = 'ro.build.version.release'
BOARD_FILE = 'ro.product.device'
SDK_FILE = 'ro.build.version.sdk'
LOGCAT_FILE_FMT = 'logcat_%s.log'
TMP_DIR = '/data/local/tmp'
# Regex to pull out file type, perms and symlink. Example:
# lrwxrwx--- 1 6 root system 2015-09-12 19:21 blah_link -> ./blah
FILE_INFO_REGEX = '^(?P<TYPE>[dl-])(?P<PERMS>[rwx-]{9})'
FILE_SYMLINK_REGEX = '^.*-> (?P<SYMLINK>.+)'
# List of the perm stats indexed by the order they are listed in the example
# supplied above.
FILE_PERMS_FLAGS = [stat.S_IRUSR, stat.S_IWUSR, stat.S_IXUSR,
                    stat.S_IRGRP, stat.S_IWGRP, stat.S_IXGRP,
                    stat.S_IROTH, stat.S_IWOTH, stat.S_IXOTH]

# Default maximum number of seconds to wait for a device to be down.
DEFAULT_WAIT_DOWN_TIME_SECONDS = 10
# Default maximum number of seconds to wait for a device to be up.
DEFAULT_WAIT_UP_TIME_SECONDS = 300
# Maximum number of seconds to wait for a device to be up after it's wiped.
WAIT_UP_AFTER_WIPE_TIME_SECONDS = 1200

# Default timeout for retrying adb/fastboot command.
DEFAULT_COMMAND_RETRY_TIMEOUT_SECONDS = 10

OS_TYPE_ANDROID = 'android'
OS_TYPE_BRILLO = 'brillo'

# Regex to parse build name to get the detailed build information.
BUILD_REGEX = ('(?P<BRANCH>([^/]+))/(?P<BUILD_TARGET>([^/]+))-'
               '(?P<BUILD_TYPE>([^/]+))/(?P<BUILD_ID>([^/]+))')
# Regex to parse devserver url to get the detailed build information. Sample
# url: http://$devserver:8080/static/branch/target/build_id
DEVSERVER_URL_REGEX = '.*/%s/*' % BUILD_REGEX

ANDROID_IMAGE_FILE_FMT = '%(build_target)s-img-%(build_id)s.zip'

BRILLO_VENDOR_PARTITIONS_FILE_FMT = (
        '%(build_target)s-vendor_partitions-%(build_id)s.zip')
AUTOTEST_SERVER_PACKAGE_FILE_FMT = (
        '%(build_target)s-autotest_server_package-%(build_id)s.tar.bz2')
ADB_DEVICE_PREFIXES = ['product:', 'model:', 'device:']

# Command to provision a Brillo device.
# os_image_dir: The full path of the directory that contains all the Android image
# files (from the image zip file).
# vendor_partition_dir: The full path of the directory that contains all the
# Brillo vendor partitions, and provision-device script.
BRILLO_PROVISION_CMD = (
        'sudo ANDROID_PROVISION_OS_PARTITIONS=%(os_image_dir)s '
        'ANDROID_PROVISION_VENDOR_PARTITIONS=%(vendor_partition_dir)s '
        '%(vendor_partition_dir)s/provision-device')

# Default timeout in minutes for fastboot commands.
DEFAULT_FASTBOOT_RETRY_TIMEOUT_MIN = 10

# Default permissions for files/dirs copied from the device.
_DEFAULT_FILE_PERMS = 0o600
_DEFAULT_DIR_PERMS = 0o700

# Constants for getprop return value for a given property.
PROPERTY_VALUE_TRUE = '1'

# Timeout used for retrying installing apk. After reinstall apk failed, we try
# to reboot the device and try again.
APK_INSTALL_TIMEOUT_MIN = 5

# The amount of time to wait for package verification to be turned off.
DISABLE_PACKAGE_VERIFICATION_TIMEOUT_MIN = 1

# Directory where (non-Brillo) Android stores tombstone crash logs.
ANDROID_TOMBSTONE_CRASH_LOG_DIR = '/data/tombstones'
# Directory where Brillo stores crash logs for native (non-Java) crashes.
BRILLO_NATIVE_CRASH_LOG_DIR = '/data/misc/crash_reporter/crash'

# A specific string value to return when a timeout has occurred.
TIMEOUT_MSG = 'TIMEOUT_OCCURRED'

class AndroidInstallError(error.InstallError):
    """Generic error for Android installation related exceptions."""


class ADBHost(abstract_ssh.AbstractSSHHost):
    """This class represents a host running an ADB server."""

    VERSION_PREFIX = provision.ANDROID_BUILD_VERSION_PREFIX
    _LABEL_FUNCTIONS = []
    _DETECTABLE_LABELS = []
    label_decorator = functools.partial(utils.add_label_detector,
                                        _LABEL_FUNCTIONS,
                                        _DETECTABLE_LABELS)

    _parser = autoserv_parser.autoserv_parser

    # Minimum build id that supports server side packaging. Older builds may
    # not have server side package built or with Autotest code change to support
    # server-side packaging.
    MIN_VERSION_SUPPORT_SSP = CONFIG.get_config_value(
            'AUTOSERV', 'min_launch_control_build_id_support_ssp', type=int)

    @staticmethod
    def check_host(host, timeout=10):
        """
        Check if the given host is an adb host.

        If SSH connectivity can't be established, check_host will try to use
        user 'adb' as well. If SSH connectivity still can't be established
        then the original SSH user is restored.

        @param host: An ssh host representing a device.
        @param timeout: The timeout for the run command.


        @return: True if the host device has adb.

        @raises AutoservRunError: If the command failed.
        @raises AutoservSSHTimeout: Ssh connection has timed out.
        """
        # host object may not have user attribute if it's a LocalHost object.
        current_user = host.user if hasattr(host, 'user') else None
        try:
            if not (host.hostname == 'localhost' or
                    host.verify_ssh_user_access()):
                host.user = 'adb'
            result = host.run(
                    'test -f %s' % server_constants.ANDROID_TESTER_FILEFLAG,
                    timeout=timeout)
        except (error.GenericHostRunError, error.AutoservSSHTimeout):
            if current_user is not None:
                host.user = current_user
            return False
        return result.exit_status == 0


    def _initialize(self, hostname='localhost', serials=None,
                    adb_serial=None, fastboot_serial=None,
                    teststation=None, *args, **dargs):
        """Initialize an ADB Host.

        This will create an ADB Host. Hostname should always refer to the
        test station connected to an Android DUT. This will be the DUT
        to test with.  If there are multiple, serial must be specified or an
        exception will be raised.

        @param hostname: Hostname of the machine running ADB.
        @param serials: DEPRECATED (to be removed)
        @param adb_serial: An ADB device serial. If None, assume a single
                           device is attached (and fail otherwise).
        @param fastboot_serial: A fastboot device serial. If None, defaults to
                                the ADB serial (or assumes a single device if
                                the latter is None).
        @param teststation: The teststation object ADBHost should use.
        """
        # Sets up the is_client_install_supported field.
        super(ADBHost, self)._initialize(hostname=hostname,
                                         is_client_install_supported=False,
                                         *args, **dargs)

        self.tmp_dirs = []
        self.labels = base_label.LabelRetriever(adb_label.ADB_LABELS)
        adb_serial = adb_serial or self._afe_host.attributes.get('serials')
        fastboot_serial = (fastboot_serial or
                self._afe_host.attributes.get('fastboot_serial'))

        self.adb_serial = adb_serial
        if adb_serial:
            adb_prefix = any(adb_serial.startswith(p)
                             for p in ADB_DEVICE_PREFIXES)
            self.fastboot_serial = (fastboot_serial or
                    ('tcp:%s' % adb_serial.split(':')[0] if
                    ':' in adb_serial and not adb_prefix else adb_serial))
            self._use_tcpip = ':' in adb_serial and not adb_prefix
        else:
            self.fastboot_serial = fastboot_serial or adb_serial
            self._use_tcpip = False
        self.teststation = (teststation if teststation
                else teststation_host.create_teststationhost(
                        hostname=hostname,
                        user=self.user,
                        password=self.password,
                        port=self.port
                ))

        msg ='Initializing ADB device on host: %s' % hostname
        if self.adb_serial:
            msg += ', ADB serial: %s' % self.adb_serial
        if self.fastboot_serial:
            msg += ', fastboot serial: %s' % self.fastboot_serial
        logging.debug(msg)

        self._os_type = None


    def _connect_over_tcpip_as_needed(self):
        """Connect to the ADB device over TCP/IP if so configured."""
        if not self._use_tcpip:
            return
        logging.debug('Connecting to device over TCP/IP')
        self.adb_run('connect %s' % self.adb_serial)


    def _restart_adbd_with_root_permissions(self):
        """Restarts the adb daemon with root permissions."""
        @retry.retry(error.GenericHostRunError, timeout_min=20/60.0,
                     delay_sec=1)
        def run_adb_root():
            """Run command `adb root`."""
            self.adb_run('root')

        # adb command may flake with error "device not found". Retry the root
        # command to reduce the chance of flake.
        run_adb_root()
        # TODO(ralphnathan): Remove this sleep once b/19749057 is resolved.
        time.sleep(1)
        self._connect_over_tcpip_as_needed()
        self.adb_run('wait-for-device')


    def _set_tcp_port(self):
        """Ensure the device remains in tcp/ip mode after a reboot."""
        if not self._use_tcpip:
            return
        port = self.adb_serial.split(':')[-1]
        self.run('setprop persist.adb.tcp.port %s' % port)


    def _reset_adbd_connection(self):
        """Resets adbd connection to the device after a reboot/initialization"""
        self._connect_over_tcpip_as_needed()
        self._restart_adbd_with_root_permissions()
        self._set_tcp_port()


    # pylint: disable=missing-docstring
    def adb_run(self, command, **kwargs):
        """Runs an adb command.

        This command will launch on the test station.

        Refer to _device_run method for docstring for parameters.
        """
        # Suppresses 'adb devices' from printing to the logs, which often
        # causes large log files.
        if command == "devices":
            kwargs['verbose'] = False
        return self._device_run(ADB_CMD, command, **kwargs)


    # pylint: disable=missing-docstring
    def fastboot_run(self, command, **kwargs):
        """Runs an fastboot command.

        This command will launch on the test station.

        Refer to _device_run method for docstring for parameters.
        """
        return self._device_run(FASTBOOT_CMD, command, **kwargs)


    # pylint: disable=missing-docstring
    @retry.retry(error.GenericHostRunError,
                 timeout_min=DEFAULT_FASTBOOT_RETRY_TIMEOUT_MIN)
    def _fastboot_run_with_retry(self, command, **kwargs):
        """Runs an fastboot command with retry.

        This command will launch on the test station.

        Refer to _device_run method for docstring for parameters.
        """
        return self.fastboot_run(command, **kwargs)


    def _log_adb_pid(self):
        """Log the pid of adb server.

        adb's server is known to have bugs and randomly restart. BY logging
        the server's pid it will allow us to better debug random adb failures.
        """
        adb_pid = self.teststation.run('pgrep -f "adb.*server"')
        logging.debug('ADB Server PID: %s', adb_pid.stdout)


    def _device_run(self, function, command, shell=False,
                    timeout=3600, ignore_status=False, ignore_timeout=False,
                    stdout=utils.TEE_TO_LOGS, stderr=utils.TEE_TO_LOGS,
                    connect_timeout=30, options='', stdin=None, verbose=True,
                    require_sudo=False, args=()):
        """Runs a command named `function` on the test station.

        This command will launch on the test station.

        @param command: Command to run.
        @param shell: If true the command runs in the adb shell otherwise if
                      False it will be passed directly to adb. For example
                      reboot with shell=False will call 'adb reboot'. This
                      option only applies to function adb.
        @param timeout: Time limit in seconds before attempting to
                        kill the running process. The run() function
                        will take a few seconds longer than 'timeout'
                        to complete if it has to kill the process.
        @param ignore_status: Do not raise an exception, no matter
                              what the exit code of the command is.
        @param ignore_timeout: Bool True if command timeouts should be
                               ignored.  Will return None on command timeout.
        @param stdout: Redirect stdout.
        @param stderr: Redirect stderr.
        @param connect_timeout: Connection timeout (in seconds)
        @param options: String with additional ssh command options
        @param stdin: Stdin to pass (a string) to the executed command
        @param require_sudo: True to require sudo to run the command. Default is
                             False.
        @param args: Sequence of strings to pass as arguments to command by
                     quoting them in " and escaping their contents if
                     necessary.

        @returns a CMDResult object.
        """
        if function == ADB_CMD:
            serial = self.adb_serial
        elif function == FASTBOOT_CMD:
            serial = self.fastboot_serial
        else:
            raise NotImplementedError('Mode %s is not supported' % function)

        if function != ADB_CMD and shell:
            raise error.CmdError('shell option is only applicable to `adb`.')

        client_side_cmd = 'timeout --signal=%d %d %s' % (signal.SIGKILL,
                                                         timeout + 1, function)
        cmd = '%s%s ' % ('sudo -n ' if require_sudo else '', client_side_cmd)

        if serial:
            cmd += '-s %s ' % serial

        if shell:
            cmd += '%s ' % SHELL_CMD
        cmd += command

        self._log_adb_pid()

        if verbose:
            logging.debug('Command: %s', cmd)

        return self.teststation.run(cmd, timeout=timeout,
                ignore_status=ignore_status,
                ignore_timeout=ignore_timeout, stdout_tee=stdout,
                stderr_tee=stderr, options=options, stdin=stdin,
                connect_timeout=connect_timeout, args=args)


    def _run_output_with_retry(self, cmd):
        """Call run_output method for the given command with retry.

        adb command can be flaky some time, and the command may fail or return
        empty string. It may take several retries until a value can be returned.

        @param cmd: The command to run.

        @return: Return value from the command after retry.
        """
        try:
            return client_utils.poll_for_condition(
                    lambda: self.run_output(cmd, ignore_status=True),
                    timeout=DEFAULT_COMMAND_RETRY_TIMEOUT_SECONDS,
                    sleep_interval=0.5,
                    desc='Get return value for command `%s`' % cmd)
        except client_utils.TimeoutError:
            return ''


    def get_device_aliases(self):
        """Get all aliases for this device."""
        product = self.get_product_name()
        return android_utils.AndroidAliases.get_product_aliases(product)

    def get_product_name(self):
        """Get the product name of the device, eg., shamu, bat"""
        return self.run_output('getprop %s' % BOARD_FILE)

    def get_board_name(self):
        """Get the name of the board, e.g., shamu, bat_land etc.
        """
        product = self.get_product_name()
        return android_utils.AndroidAliases.get_board_name(product)


    @label_decorator()
    def get_board(self):
        """Determine the correct board label for the device.

        @returns a string representing this device's board.
        """
        board = self.get_board_name()
        board_os = self.get_os_type()
        return constants.BOARD_PREFIX + '-'.join([board_os, board])


    def job_start(self):
        """Overload of parent which intentionally doesn't log certain files.

        The parent implementation attempts to log certain Linux files, such as
        /var/log, which do not exist on Android, thus there is no call to the
        parent's job_start().  The sync call is made so that logcat logs can be
        approximately matched to server logs.
        """
        # Try resetting the ADB daemon on the device, however if we are
        # creating the host to do a repair job, the device maybe inaccesible
        # via ADB.
        try:
            self._reset_adbd_connection()
        except error.GenericHostRunError as e:
            logging.error('Unable to reset the device adb daemon connection: '
                          '%s.', e)

        if self.is_up():
            self._sync_time()
            self._enable_native_crash_logging()


    def run(self, command, timeout=3600, ignore_status=False,
            ignore_timeout=False, stdout_tee=utils.TEE_TO_LOGS,
            stderr_tee=utils.TEE_TO_LOGS, connect_timeout=30, options='',
            stdin=None, verbose=True, args=()):
        """Run a command on the adb device.

        The command given will be ran directly on the adb device; for example
        'ls' will be ran as: 'abd shell ls'

        @param command: The command line string.
        @param timeout: Time limit in seconds before attempting to
                        kill the running process. The run() function
                        will take a few seconds longer than 'timeout'
                        to complete if it has to kill the process.
        @param ignore_status: Do not raise an exception, no matter
                              what the exit code of the command is.
        @param ignore_timeout: Bool True if command timeouts should be
                               ignored.  Will return None on command timeout.
        @param stdout_tee: Redirect stdout.
        @param stderr_tee: Redirect stderr.
        @param connect_timeout: Connection timeout (in seconds).
        @param options: String with additional ssh command options.
        @param stdin: Stdin to pass (a string) to the executed command
        @param args: Sequence of strings to pass as arguments to command by
                     quoting them in " and escaping their contents if
                     necessary.

        @returns A CMDResult object or None if the call timed out and
                 ignore_timeout is True.

        @raises AutoservRunError: If the command failed.
        @raises AutoservSSHTimeout: Ssh connection has timed out.
        """
        command = ('"%s; echo %s:\$?"' %
                   (utils.sh_escape(command), CMD_OUTPUT_PREFIX))

        def _run():
            """Run the command and try to parse the exit code.
            """
            result = self.adb_run(
                    command, shell=True, timeout=timeout,
                    ignore_status=ignore_status, ignore_timeout=ignore_timeout,
                    stdout=stdout_tee, stderr=stderr_tee,
                    connect_timeout=connect_timeout, options=options,
                    stdin=stdin, verbose=verbose, args=args)
            if not result:
                # In case of timeouts. Set the return to a specific string
                # value. That way the caller of poll_for_condition knows
                # a timeout occurs and should return None. Return None here will
                # lead to the command to be retried.
                return TIMEOUT_MSG
            parse_output = re.match(CMD_OUTPUT_REGEX, result.stdout)
            if not parse_output and not ignore_status:
                logging.error('Failed to parse the exit code for command: `%s`.'
                              ' result: `%s`', command, result.stdout)
                return None
            elif parse_output:
                result.stdout = parse_output.group('OUTPUT')
                result.exit_status = int(parse_output.group('EXIT_CODE'))
                if result.exit_status != 0 and not ignore_status:
                    raise error.AutoservRunError(command, result)
            return result

        result = client_utils.poll_for_condition(
                lambda: _run(),
                timeout=DEFAULT_COMMAND_RETRY_TIMEOUT_SECONDS,
                sleep_interval=0.5,
                desc='Run command `%s`' % command)
        return None if result == TIMEOUT_MSG else result


    def check_boot_to_adb_complete(self, exception_type=error.TimeoutException):
        """Check if the device has finished booting and accessible by adb.

        @param exception_type: Type of exception to raise. Default is set to
                error.TimeoutException for retry.

        @raise exception_type: If the device has not finished booting yet, raise
                an exception of type `exception_type`.
        """
        bootcomplete = self._run_output_with_retry('getprop dev.bootcomplete')
        if bootcomplete != PROPERTY_VALUE_TRUE:
            raise exception_type('dev.bootcomplete is %s.' % bootcomplete)
        if self.get_os_type() == OS_TYPE_ANDROID:
            boot_completed = self._run_output_with_retry(
                    'getprop sys.boot_completed')
            if boot_completed != PROPERTY_VALUE_TRUE:
                raise exception_type('sys.boot_completed is %s.' %
                                     boot_completed)


    def wait_up(self, timeout=DEFAULT_WAIT_UP_TIME_SECONDS, command=ADB_CMD):
        """Wait until the remote host is up or the timeout expires.

        Overrides wait_down from AbstractSSHHost.

        @param timeout: Time limit in seconds before returning even if the host
                is not up.
        @param command: The command used to test if a device is up, i.e.,
                accessible by the given command. Default is set to `adb`.

        @returns True if the host was found to be up before the timeout expires,
                 False otherwise.
        """
        @retry.retry(error.TimeoutException, timeout_min=timeout/60.0,
                     delay_sec=1)
        def _wait_up():
            if not self.is_up(command=command):
                raise error.TimeoutException('Device is still down.')
            if command == ADB_CMD:
                self.check_boot_to_adb_complete()
            return True

        try:
            _wait_up()
            logging.debug('Host %s is now up, and can be accessed by %s.',
                          self.hostname, command)
            return True
        except error.TimeoutException:
            logging.debug('Host %s is still down after waiting %d seconds',
                          self.hostname, timeout)
            return False


    def wait_down(self, timeout=DEFAULT_WAIT_DOWN_TIME_SECONDS,
                  warning_timer=None, old_boot_id=None, command=ADB_CMD,
                  boot_id=None):
        """Wait till the host goes down.

        Return when the host is down (not accessible via the command) OR when
        the device's boot_id changes (if a boot_id was provided).

        Overrides wait_down from AbstractSSHHost.

        @param timeout: Time in seconds to wait for the host to go down.
        @param warning_timer: Time limit in seconds that will generate
                              a warning if the host is not down yet.
                              Currently ignored.
        @param old_boot_id: Not applicable for adb_host.
        @param command: `adb`, test if the device can be accessed by adb
                command, or `fastboot`, test if the device can be accessed by
                fastboot command. Default is set to `adb`.
        @param boot_id: UUID of previous boot (consider the device down when the
                        boot_id changes from this value). Ignored if None.

        @returns True if the device goes down before the timeout, False
                 otherwise.
        """
        @retry.retry(error.TimeoutException, timeout_min=timeout/60.0,
                     delay_sec=1)
        def _wait_down():
            up = self.is_up(command=command)
            if not up:
                return True
            if boot_id:
                try:
                    new_boot_id = self.get_boot_id()
                    if new_boot_id != boot_id:
                        return True
                except error.GenericHostRunError:
                    pass
            raise error.TimeoutException('Device is still up.')

        try:
            _wait_down()
            logging.debug('Host %s is now down', self.hostname)
            return True
        except error.TimeoutException:
            logging.debug('Host %s is still up after waiting %d seconds',
                          self.hostname, timeout)
            return False


    def reboot(self):
        """Reboot the android device via adb.

        @raises AutoservRebootError if reboot failed.
        """
        # Not calling super.reboot() as we want to reboot the ADB device not
        # the test station we are running ADB on.
        boot_id = self.get_boot_id()
        self.adb_run('reboot', timeout=10, ignore_timeout=True)
        if not self.wait_down(boot_id=boot_id):
            raise error.AutoservRebootError(
                    'ADB Device %s is still up after reboot' % self.adb_serial)
        if not self.wait_up():
            raise error.AutoservRebootError(
                    'ADB Device %s failed to return from reboot.' %
                    self.adb_serial)
        self._reset_adbd_connection()


    def fastboot_reboot(self):
        """Do a fastboot reboot to go back to adb.

        @raises AutoservRebootError if reboot failed.
        """
        self.fastboot_run('reboot')
        if not self.wait_down(command=FASTBOOT_CMD):
            raise error.AutoservRebootError(
                    'Device %s is still in fastboot mode after reboot' %
                    self.fastboot_serial)
        if not self.wait_up():
            raise error.AutoservRebootError(
                    'Device %s failed to boot to adb after fastboot reboot.' %
                    self.adb_serial)
        self._reset_adbd_connection()


    def remount(self):
        """Remounts paritions on the device read-write.

        Specifically, the /system, /vendor (if present) and /oem (if present)
        partitions on the device are remounted read-write.
        """
        self.adb_run('remount')


    @staticmethod
    def parse_device_serials(devices_output):
        """Return a list of parsed serials from the output.

        @param devices_output: Output from either an adb or fastboot command.

        @returns List of device serials
        """
        devices = []
        for line in devices_output.splitlines():
            match = re.search(DEVICE_FINDER_REGEX, line)
            if match:
                serial = match.group('SERIAL')
                if serial == DEVICE_NO_SERIAL_MSG or re.match(r'^\?+$', serial):
                    serial = DEVICE_NO_SERIAL_TAG
                logging.debug('Found Device: %s', serial)
                devices.append(serial)
        return devices


    def _get_devices(self, use_adb):
        """Get a list of devices currently attached to the test station.

        @params use_adb: True to get adb accessible devices. Set to False to
                         get fastboot accessible devices.

        @returns a list of devices attached to the test station.
        """
        if use_adb:
            result = self.adb_run('devices').stdout
            if self.adb_serial and self.adb_serial not in result:
                self._connect_over_tcpip_as_needed()
        else:
            result = self.fastboot_run('devices').stdout
            if (self.fastboot_serial and
                self.fastboot_serial not in result):
                # fastboot devices won't list the devices using TCP
                try:
                    if 'product' in self.fastboot_run('getvar product',
                                                      timeout=2).stderr:
                        result += '\n%s\tfastboot' % self.fastboot_serial
                # The main reason we do a general Exception catch here instead
                # of setting ignore_timeout/status to True is because even when
                # the fastboot process has been nuked, it still stays around and
                # so bgjob wants to warn us of this and tries to read the
                # /proc/<pid>/stack file which then promptly returns an
                # 'Operation not permitted' error since we're running as moblab
                # and we don't have permission to read those files.
                except Exception:
                    pass
        return self.parse_device_serials(result)


    def adb_devices(self):
        """Get a list of devices currently attached to the test station and
        accessible with the adb command."""
        devices = self._get_devices(use_adb=True)
        if self.adb_serial is None and len(devices) > 1:
            raise error.AutoservError(
                    'Not given ADB serial but multiple devices detected')
        return devices


    def fastboot_devices(self):
        """Get a list of devices currently attached to the test station and
        accessible by fastboot command.
        """
        devices = self._get_devices(use_adb=False)
        if self.fastboot_serial is None and len(devices) > 1:
            raise error.AutoservError(
                    'Not given fastboot serial but multiple devices detected')
        return devices


    def is_up(self, timeout=0, command=ADB_CMD):
        """Determine if the specified adb device is up with expected mode.

        @param timeout: Not currently used.
        @param command: `adb`, the device can be accessed by adb command,
                or `fastboot`, the device can be accessed by fastboot command.
                Default is set to `adb`.

        @returns True if the device is detectable by given command, False
                 otherwise.

        """
        if command == ADB_CMD:
            devices = self.adb_devices()
            serial = self.adb_serial
            # ADB has a device state, if the device is not online, no
            # subsequent ADB command will complete.
            # DUT with single device connected may not have adb_serial set.
            # Therefore, skip checking if serial is in the list of adb devices
            # if self.adb_serial is not set.
            if (serial and serial not in devices) or not self.is_device_ready():
                logging.debug('Waiting for device to enter the ready state.')
                return False
        elif command == FASTBOOT_CMD:
            devices = self.fastboot_devices()
            serial = self.fastboot_serial
        else:
            raise NotImplementedError('Mode %s is not supported' % command)

        return bool(devices and (not serial or serial in devices))


    def stop_loggers(self):
        """Inherited stop_loggers function.

        Calls parent function and captures logcat, since the end of the run
        is logically the end/stop of the logcat log.
        """
        super(ADBHost, self).stop_loggers()

        # When called from atest and tools like it there will be no job.
        if not self.job:
            return

        # Record logcat log to a temporary file on the teststation.
        tmp_dir = self.teststation.get_tmp_dir()
        logcat_filename = LOGCAT_FILE_FMT % self.adb_serial
        teststation_filename = os.path.join(tmp_dir, logcat_filename)
        try:
            self.adb_run('logcat -v time -d > "%s"' % (teststation_filename),
                         timeout=20)
        except (error.GenericHostRunError, error.AutoservSSHTimeout,
                error.CmdTimeoutError):
            return
        # Copy-back the log to the drone's results directory.
        results_logcat_filename = os.path.join(self.job.resultdir,
                                               logcat_filename)
        self.teststation.get_file(teststation_filename,
                                  results_logcat_filename)
        try:
            self.teststation.run('rm -rf %s' % tmp_dir)
        except (error.GenericHostRunError, error.AutoservSSHTimeout) as e:
            logging.warn('failed to remove dir %s: %s', tmp_dir, e)

        self._collect_crash_logs()


    def close(self):
        """Close the ADBHost object.

        Called as the test ends. Will return the device to USB mode and kill
        the ADB server.
        """
        super(ADBHost, self).close()
        self.teststation.close()


    def syslog(self, message, tag='autotest'):
        """Logs a message to syslog on the device.

        @param message String message to log into syslog
        @param tag String tag prefix for syslog

        """
        self.run('log -t "%s" "%s"' % (tag, message))


    def get_autodir(self):
        """Return the directory to install autotest for client side tests."""
        return '/data/autotest'


    def is_device_ready(self):
        """Return the if the device is ready for ADB commands."""
        try:
            # Retry to avoid possible flakes.
            is_ready = client_utils.poll_for_condition(
                lambda: self.adb_run('get-state').stdout.strip() == 'device',
                timeout=DEFAULT_COMMAND_RETRY_TIMEOUT_SECONDS, sleep_interval=1,
                desc='Waiting for device state to be `device`')
        except client_utils.TimeoutError:
            is_ready = False

        logging.debug('Device state is %sready', '' if is_ready else 'NOT ')
        return is_ready


    def verify_connectivity(self):
        """Verify we can connect to the device."""
        if not self.is_device_ready():
            raise error.AutoservHostError('device state is not in the '
                                          '\'device\' state.')


    def verify_software(self):
        """Verify working software on an adb_host.

        """
        # Check if adb and fastboot are present.
        self.teststation.run('which adb')
        self.teststation.run('which fastboot')
        self.teststation.run('which unzip')

        # Apply checks only for Android device.
        if self.get_os_type() == OS_TYPE_ANDROID:
            # Make sure ro.boot.hardware and ro.build.product match.
            hardware = self._run_output_with_retry('getprop ro.boot.hardware')
            product = self._run_output_with_retry('getprop ro.build.product')
            if hardware != product:
                raise error.AutoservHostError('ro.boot.hardware: %s does not '
                                              'match to ro.build.product: %s' %
                                              (hardware, product))


    def verify_job_repo_url(self, tag=''):
        """Make sure job_repo_url of this host is valid.

        TODO (crbug.com/532223): Actually implement this method.

        @param tag: The tag from the server job, in the format
                    <job_id>-<user>/<hostname>, or <hostless> for a server job.
        """
        return


    def repair(self, board=None, os=None):
        """Attempt to get the DUT to pass `self.verify()`.

        @param board: Board name of the device. For host created in testbed,
                      it does not have host labels and attributes. Therefore,
                      the board name needs to be passed in from the testbed
                      repair call.
        @param os: OS of the device. For host created in testbed, it does not
                   have host labels and attributes. Therefore, the OS needs to
                   be passed in from the testbed repair call.
        """
        if self.is_up():
            logging.debug('The device is up and accessible by adb. No need to '
                          'repair.')
            return
        # Force to do a reinstall in repair first. The reason is that it
        # requires manual action to put the device into fastboot mode.
        # If repair tries to switch the device back to adb mode, one will
        # have to change it back to fastboot mode manually again.
        logging.debug('Verifying the device is accessible via fastboot.')
        self.ensure_bootloader_mode()
        subdir_tag = self.adb_serial if board else None
        if not self.job.run_test(
                'provision_AndroidUpdate', host=self, value=None, force=True,
                repair=True, board=board, os=os, subdir_tag=subdir_tag):
            raise error.AutoservRepairTotalFailure(
                    'Unable to repair the device.')


    def send_file(self, source, dest, delete_dest=False,
                  preserve_symlinks=False, excludes=None):
        """Copy files from the drone to the device.

        Just a note, there is the possibility the test station is localhost
        which makes some of these steps redundant (e.g. creating tmp dir) but
        that scenario will undoubtedly be a development scenario (test station
        is also the moblab) and not the typical live test running scenario so
        the redundancy I think is harmless.

        @param source: The file/directory on the drone to send to the device.
        @param dest: The destination path on the device to copy to.
        @param delete_dest: A flag set to choose whether or not to delete
                            dest on the device if it exists.
        @param preserve_symlinks: Controls if symlinks on the source will be
                                  copied as such on the destination or
                                  transformed into the referenced
                                  file/directory.
        @param excludes: A list of file pattern that matches files not to be
                         sent. `send_file` will fail if exclude is set, since
                         local copy does not support --exclude, e.g., when
                         using scp to copy file.
        """
        # If we need to preserve symlinks, let's check if the source is a
        # symlink itself and if so, just create it on the device.
        if preserve_symlinks:
            symlink_target = None
            try:
                symlink_target = os.readlink(source)
            except OSError:
                # Guess it's not a symlink.
                pass

            if symlink_target is not None:
                # Once we create the symlink, let's get out of here.
                self.run('ln -s %s %s' % (symlink_target, dest))
                return

        # Stage the files on the test station.
        tmp_dir = self.teststation.get_tmp_dir()
        src_path = os.path.join(tmp_dir, os.path.basename(dest))
        # Now copy the file over to the test station so you can reference the
        # file in the push command.
        self.teststation.send_file(
                source, src_path, preserve_symlinks=preserve_symlinks,
                excludes=excludes)

        if delete_dest:
            self.run('rm -rf %s' % dest)

        self.adb_run('push %s %s' % (src_path, dest))

        # Cleanup the test station.
        try:
            self.teststation.run('rm -rf %s' % tmp_dir)
        except (error.GenericHostRunError, error.AutoservSSHTimeout) as e:
            logging.warn('failed to remove dir %s: %s', tmp_dir, e)


    def _get_file_info(self, dest):
        """Get permission and possible symlink info about file on the device.

        These files are on the device so we only have shell commands (via adb)
        to get the info we want.  We'll use 'ls' to get it all.

        @param dest: File to get info about.

        @returns a dict of the file permissions and symlink.
        """
        # Grab file info.
        file_info = self.run_output('ls -ld %s' % dest)
        symlink = None
        perms = 0
        match = re.match(FILE_INFO_REGEX, file_info)
        if match:
            # Check if it's a symlink and grab the linked dest if it is.
            if match.group('TYPE') == 'l':
                symlink_match = re.match(FILE_SYMLINK_REGEX, file_info)
                if symlink_match:
                    symlink = symlink_match.group('SYMLINK')

            # Set the perms.
            for perm, perm_flag in zip(match.group('PERMS'), FILE_PERMS_FLAGS):
                if perm != '-':
                    perms |= perm_flag

        return {'perms': perms,
                'symlink': symlink}


    def get_file(self, source, dest, delete_dest=False, preserve_perm=True,
                 preserve_symlinks=False):
        """Copy files from the device to the drone.

        Just a note, there is the possibility the test station is localhost
        which makes some of these steps redundant (e.g. creating tmp dir) but
        that scenario will undoubtedly be a development scenario (test station
        is also the moblab) and not the typical live test running scenario so
        the redundancy I think is harmless.

        @param source: The file/directory on the device to copy back to the
                       drone.
        @param dest: The destination path on the drone to copy to.
        @param delete_dest: A flag set to choose whether or not to delete
                            dest on the drone if it exists.
        @param preserve_perm: Tells get_file() to try to preserve the sources
                              permissions on files and dirs.
        @param preserve_symlinks: Try to preserve symlinks instead of
                                  transforming them into files/dirs on copy.
        """
        # Stage the files on the test station under teststation_temp_dir.
        teststation_temp_dir = self.teststation.get_tmp_dir()
        teststation_dest = os.path.join(teststation_temp_dir,
                                        os.path.basename(source))

        source_info = {}
        if preserve_symlinks or preserve_perm:
            source_info = self._get_file_info(source)

        # If we want to preserve symlinks, just create it here, otherwise pull
        # the file off the device.
        #
        # TODO(sadmac): Directories containing symlinks won't behave as
        # expected.
        if preserve_symlinks and source_info['symlink']:
            os.symlink(source_info['symlink'], dest)
        else:
            self.adb_run('pull %s %s' % (source, teststation_temp_dir))

            # Copy over the file from the test station and clean up.
            self.teststation.get_file(teststation_dest, dest,
                                      delete_dest=delete_dest)
            try:
                self.teststation.run('rm -rf %s' % teststation_temp_dir)
            except (error.GenericHostRunError, error.AutoservSSHTimeout) as e:
                logging.warn('failed to remove dir %s: %s',
                             teststation_temp_dir, e)

            # Source will be copied under dest if either:
            #  1. Source is a directory and doesn't end with /.
            #  2. Source is a file and dest is a directory.
            command = '[ -d %s ]' % source
            source_is_dir = self.run(command,
                                     ignore_status=True).exit_status == 0
            logging.debug('%s on the device %s a directory', source,
                          'is' if source_is_dir else 'is not')

            if ((source_is_dir and not source.endswith(os.sep)) or
                (not source_is_dir and os.path.isdir(dest))):
                receive_path = os.path.join(dest, os.path.basename(source))
            else:
                receive_path = dest

            if not os.path.exists(receive_path):
                logging.warning('Expected file %s does not exist; skipping'
                                ' permissions copy', receive_path)
                return

            # Set the permissions of the received file/dirs.
            if os.path.isdir(receive_path):
                for root, _dirs, files in os.walk(receive_path):
                    def process(rel_path, default_perm):
                        info = self._get_file_info(os.path.join(source,
                                                                rel_path))
                        if info['perms'] != 0:
                            target = os.path.join(receive_path, rel_path)
                            if preserve_perm:
                                os.chmod(target, info['perms'])
                            else:
                                os.chmod(target, default_perm)

                    rel_root = os.path.relpath(root, receive_path)
                    process(rel_root, _DEFAULT_DIR_PERMS)
                    for f in files:
                        process(os.path.join(rel_root, f), _DEFAULT_FILE_PERMS)
            elif preserve_perm:
                os.chmod(receive_path, source_info['perms'])
            else:
                os.chmod(receive_path, _DEFAULT_FILE_PERMS)


    def get_release_version(self):
        """Get the release version from the RELEASE_FILE on the device.

        @returns The release string in the RELEASE_FILE.

        """
        return self.run_output('getprop %s' % RELEASE_FILE)


    def get_tmp_dir(self, parent=''):
        """Return a suitable temporary directory on the device.

        We ensure this is a subdirectory of /data/local/tmp.

        @param parent: Parent directory of the returned tmp dir.

        @returns a path to the temp directory on the host.
        """
        # TODO(kevcheng): Refactor the cleanup of tmp dir to be inherited
        #                 from the parent.
        if not parent.startswith(TMP_DIR):
            parent = os.path.join(TMP_DIR, parent.lstrip(os.path.sep))
        self.run('mkdir -p %s' % parent)
        tmp_dir = self.run_output('mktemp -d -p %s' % parent)
        self.tmp_dirs.append(tmp_dir)
        return tmp_dir


    def get_platform(self):
        """Determine the correct platform label for this host.

        @returns a string representing this host's platform.
        """
        return 'adb'


    def get_os_type(self):
        """Get the OS type of the DUT, e.g., android or brillo.
        """
        if not self._os_type:
            if self.run_output('getprop ro.product.brand') == 'Brillo':
                self._os_type = OS_TYPE_BRILLO
            else:
                self._os_type = OS_TYPE_ANDROID

        return self._os_type


    def _forward(self, reverse, args):
        """Execute a forwarding command.

        @param reverse: Whether this is reverse forwarding (Boolean).
        @param args: List of command arguments.
        """
        cmd = '%s %s' % ('reverse' if reverse else 'forward', ' '.join(args))
        self.adb_run(cmd)


    def add_forwarding(self, src, dst, reverse=False, rebind=True):
        """Forward a port between the ADB host and device.

        Port specifications are any strings accepted as such by ADB, for
        example 'tcp:8080'.

        @param src: Port specification to forward from.
        @param dst: Port specification to forward to.
        @param reverse: Do reverse forwarding from device to host (Boolean).
        @param rebind: Allow rebinding an already bound port (Boolean).
        """
        args = []
        if not rebind:
            args.append('--no-rebind')
        args += [src, dst]
        self._forward(reverse, args)


    def remove_forwarding(self, src=None, reverse=False):
        """Removes forwarding on port.

        @param src: Port specification, or None to remove all forwarding.
        @param reverse: Whether this is reverse forwarding (Boolean).
        """
        args = []
        if src is None:
            args.append('--remove-all')
        else:
            args += ['--remove', src]
        self._forward(reverse, args)


    def create_ssh_tunnel(self, port, local_port):
        """
        Forwards a port securely through a tunnel process from the server
        to the DUT for RPC server connection.
        Add a 'ADB forward' rule to forward the RPC packets from the AdbHost
        to the DUT.

        @param port: remote port on the DUT.
        @param local_port: local forwarding port.

        @return: the tunnel process.
        """
        self.add_forwarding('tcp:%s' % port, 'tcp:%s' % port)
        return super(ADBHost, self).create_ssh_tunnel(port, local_port)


    def disconnect_ssh_tunnel(self, tunnel_proc, port):
        """
        Disconnects a previously forwarded port from the server to the DUT for
        RPC server connection.
        Remove the previously added 'ADB forward' rule to forward the RPC
        packets from the AdbHost to the DUT.

        @param tunnel_proc: the original tunnel process returned from
                            |create_ssh_tunnel|.
        @param port: remote port on the DUT.

        """
        self.remove_forwarding('tcp:%s' % port)
        super(ADBHost, self).disconnect_ssh_tunnel(tunnel_proc, port)


    def ensure_bootloader_mode(self):
        """Ensure the device is in bootloader mode.

        @raise: error.AutoservError if the device failed to reboot into
                bootloader mode.
        """
        if self.is_up(command=FASTBOOT_CMD):
            return
        self.adb_run('reboot bootloader')
        if not self.wait_up(command=FASTBOOT_CMD):
            raise error.AutoservError(
                    'Device %s failed to reboot into bootloader mode.' %
                    self.fastboot_serial)


    def ensure_adb_mode(self, timeout=DEFAULT_WAIT_UP_TIME_SECONDS):
        """Ensure the device is up and can be accessed by adb command.

        @param timeout: Time limit in seconds before returning even if the host
                        is not up.

        @raise: error.AutoservError if the device failed to reboot into
                adb mode.
        """
        if self.is_up():
            return
        # Ignore timeout error to allow `fastboot reboot` to fail quietly and
        # check if the device is in adb mode.
        self.fastboot_run('reboot', timeout=timeout, ignore_timeout=True)
        if not self.wait_up(timeout=timeout):
            raise error.AutoservError(
                    'Device %s failed to reboot into adb mode.' %
                    self.adb_serial)
        self._reset_adbd_connection()


    @classmethod
    def get_build_info_from_build_url(cls, build_url):
        """Get the Android build information from the build url.

        @param build_url: The url to use for downloading Android artifacts.
                pattern: http://$devserver:###/static/branch/target/build_id

        @return: A dictionary of build information, including keys:
                 build_target, branch, target, build_id.
        @raise AndroidInstallError: If failed to parse build_url.
        """
        if not build_url:
            raise AndroidInstallError('Need build_url to download image files.')

        try:
            match = re.match(DEVSERVER_URL_REGEX, build_url)
            return {'build_target': match.group('BUILD_TARGET'),
                    'branch': match.group('BRANCH'),
                    'target': ('%s-%s' % (match.group('BUILD_TARGET'),
                                          match.group('BUILD_TYPE'))),
                    'build_id': match.group('BUILD_ID')}
        except (AttributeError, IndexError, ValueError) as e:
            raise AndroidInstallError(
                    'Failed to parse build url: %s\nError: %s' % (build_url, e))


    @retry.retry(error.GenericHostRunError, timeout_min=10)
    def download_file(self, build_url, file, dest_dir, unzip=False,
                      unzip_dest=None):
        """Download the given file from the build url.

        @param build_url: The url to use for downloading Android artifacts.
                pattern: http://$devserver:###/static/branch/target/build_id
        @param file: Name of the file to be downloaded, e.g., boot.img.
        @param dest_dir: Destination folder for the file to be downloaded to.
        @param unzip: If True, unzip the downloaded file.
        @param unzip_dest: Location to unzip the downloaded file to. If not
                           provided, dest_dir is used.
        """
        # Append the file name to the url if build_url is linked to the folder
        # containing the file.
        if not build_url.endswith('/%s' % file):
            src_url = os.path.join(build_url, file)
        else:
            src_url = build_url
        dest_file = os.path.join(dest_dir, file)
        try:
            self.teststation.run('wget -q -O "%s" "%s"' % (dest_file, src_url))
            if unzip:
                unzip_dest = unzip_dest or dest_dir
                self.teststation.run('unzip "%s/%s" -x -d "%s"' %
                                     (dest_dir, file, unzip_dest))
        except:
            # Delete the destination file if download failed.
            self.teststation.run('rm -f "%s"' % dest_file)
            raise


    def stage_android_image_files(self, build_url):
        """Download required image files from the given build_url to a local
        directory in the machine runs fastboot command.

        @param build_url: The url to use for downloading Android artifacts.
                pattern: http://$devserver:###/static/branch/target/build_id

        @return: Path to the directory contains image files.
        """
        build_info = self.get_build_info_from_build_url(build_url)

        zipped_image_file = ANDROID_IMAGE_FILE_FMT % build_info
        image_dir = self.teststation.get_tmp_dir()

        try:
            self.download_file(build_url, zipped_image_file, image_dir,
                               unzip=True)
            images = android_utils.AndroidImageFiles.get_standalone_images(
                    build_info['build_target'])
            for image_file in images:
                self.download_file(build_url, image_file, image_dir)

            return image_dir
        except:
            self.teststation.run('rm -rf %s' % image_dir)
            raise


    def stage_brillo_image_files(self, build_url):
        """Download required brillo image files from the given build_url to a
        local directory in the machine runs fastboot command.

        @param build_url: The url to use for downloading Android artifacts.
                pattern: http://$devserver:###/static/branch/target/build_id

        @return: Path to the directory contains image files.
        """
        build_info = self.get_build_info_from_build_url(build_url)

        zipped_image_file = ANDROID_IMAGE_FILE_FMT % build_info
        vendor_partitions_file = BRILLO_VENDOR_PARTITIONS_FILE_FMT % build_info
        image_dir = self.teststation.get_tmp_dir()

        try:
            self.download_file(build_url, zipped_image_file, image_dir,
                               unzip=True)
            self.download_file(build_url, vendor_partitions_file, image_dir,
                               unzip=True,
                               unzip_dest=os.path.join(image_dir, 'vendor'))
            return image_dir
        except:
            self.teststation.run('rm -rf %s' % image_dir)
            raise


    def stage_build_for_install(self, build_name, os_type=None):
        """Stage a build on a devserver and return the build_url and devserver.

        @param build_name: a name like git-master/shamu-userdebug/2040953

        @returns a tuple with an update URL like:
            http://172.22.50.122:8080/git-master/shamu-userdebug/2040953
            and the devserver instance.
        """
        os_type = os_type or self.get_os_type()
        logging.info('Staging build for installation: %s', build_name)
        devserver = dev_server.AndroidBuildServer.resolve(build_name,
                                                          self.hostname)
        build_name = devserver.translate(build_name)
        branch, target, build_id = utils.parse_launch_control_build(build_name)
        devserver.trigger_download(target, build_id, branch,
                                   os=os_type, synchronous=False)
        return '%s/static/%s' % (devserver.url(), build_name), devserver


    def install_android(self, build_url, build_local_path=None, wipe=True,
                        flash_all=False, disable_package_verification=True,
                        skip_setup_wizard=True):
        """Install the Android DUT.

        Following are the steps used here to provision an android device:
        1. If build_local_path is not set, download the image zip file, e.g.,
           shamu-img-2284311.zip, unzip it.
        2. Run fastboot to install following artifacts:
           bootloader, radio, boot, system, vendor(only if exists)

        Repair is not supported for Android devices yet.

        @param build_url: The url to use for downloading Android artifacts.
                pattern: http://$devserver:###/static/$build
        @param build_local_path: The path to a local folder that contains the
                image files needed to provision the device. Note that the folder
                is in the machine running adb command, rather than the drone.
        @param wipe: If true, userdata will be wiped before flashing.
        @param flash_all: If True, all img files found in img_path will be
                flashed. Otherwise, only boot and system are flashed.

        @raises AndroidInstallError if any error occurs.
        """
        # If the build is not staged in local server yet, clean up the temp
        # folder used to store image files after the provision is completed.
        delete_build_folder = bool(not build_local_path)

        try:
            # Download image files needed for provision to a local directory.
            if not build_local_path:
                build_local_path = self.stage_android_image_files(build_url)

            # Device needs to be in bootloader mode for flashing.
            self.ensure_bootloader_mode()

            if wipe:
                self._fastboot_run_with_retry('-w')

            # Get all *.img file in the build_local_path.
            list_file_cmd = 'ls -d %s' % os.path.join(build_local_path, '*.img')
            image_files = self.teststation.run(
                    list_file_cmd).stdout.strip().split()
            images = dict([(os.path.basename(f), f) for f in image_files])
            build_info = self.get_build_info_from_build_url(build_url)
            board = build_info['build_target']
            all_images = (
                    android_utils.AndroidImageFiles.get_standalone_images(board)
                    + android_utils.AndroidImageFiles.get_zipped_images(board))

            # Sort images to be flashed, bootloader needs to be the first one.
            bootloader = android_utils.AndroidImageFiles.BOOTLOADER
            sorted_images = sorted(
                    images.items(),
                    key=lambda pair: 0 if pair[0] == bootloader else 1)
            for image, image_file in sorted_images:
                if image not in all_images:
                    continue
                logging.info('Flashing %s...', image_file)
                self._fastboot_run_with_retry('-S 256M flash %s %s' %
                                              (image[:-4], image_file))
                if image == android_utils.AndroidImageFiles.BOOTLOADER:
                    self.fastboot_run('reboot-bootloader')
                    self.wait_up(command=FASTBOOT_CMD)
        except Exception as e:
            logging.error('Install Android build failed with error: %s', e)
            # Re-raise the exception with type of AndroidInstallError.
            raise AndroidInstallError, sys.exc_info()[1], sys.exc_info()[2]
        finally:
            if delete_build_folder:
                self.teststation.run('rm -rf %s' % build_local_path)
            timeout = (WAIT_UP_AFTER_WIPE_TIME_SECONDS if wipe else
                       DEFAULT_WAIT_UP_TIME_SECONDS)
            self.ensure_adb_mode(timeout=timeout)
            if disable_package_verification:
                # TODO: Use a whitelist of devices to do this for rather than
                # doing it by default.
                self.disable_package_verification()
            if skip_setup_wizard:
                try:
                    self.skip_setup_wizard()
                except error.GenericHostRunError:
                    logging.error('Could not skip setup wizard.')
        logging.info('Successfully installed Android build staged at %s.',
                     build_url)


    def install_brillo(self, build_url, build_local_path=None):
        """Install the Brillo DUT.

        Following are the steps used here to provision an android device:
        1. If build_local_path is not set, download the image zip file, e.g.,
           dragonboard-img-123456.zip, unzip it. And download the vendor
           partition zip file, e.g., dragonboard-vendor_partitions-123456.zip,
           unzip it to vendor folder.
        2. Run provision_device script to install OS images and vendor
           partitions.

        @param build_url: The url to use for downloading Android artifacts.
                pattern: http://$devserver:###/static/$build
        @param build_local_path: The path to a local folder that contains the
                image files needed to provision the device. Note that the folder
                is in the machine running adb command, rather than the drone.

        @raises AndroidInstallError if any error occurs.
        """
        # If the build is not staged in local server yet, clean up the temp
        # folder used to store image files after the provision is completed.
        delete_build_folder = bool(not build_local_path)

        try:
            # Download image files needed for provision to a local directory.
            if not build_local_path:
                build_local_path = self.stage_brillo_image_files(build_url)

            # Device needs to be in bootloader mode for flashing.
            self.ensure_bootloader_mode()

            # Run provision_device command to install image files and vendor
            # partitions.
            vendor_partition_dir = os.path.join(build_local_path, 'vendor')
            cmd = (BRILLO_PROVISION_CMD %
                   {'os_image_dir': build_local_path,
                    'vendor_partition_dir': vendor_partition_dir})
            if self.fastboot_serial:
                cmd += ' -s %s ' % self.fastboot_serial
            self.teststation.run(cmd)
        except Exception as e:
            logging.error('Install Brillo build failed with error: %s', e)
            # Re-raise the exception with type of AndroidInstallError.
            raise AndroidInstallError, sys.exc_info()[1], sys.exc_info()[2]
        finally:
            if delete_build_folder:
                self.teststation.run('rm -rf %s' % build_local_path)
            self.ensure_adb_mode()
        logging.info('Successfully installed Android build staged at %s.',
                     build_url)


    @property
    def job_repo_url_attribute(self):
        """Get the host attribute name for job_repo_url, which should append the
        adb serial.
        """
        return '%s_%s' % (constants.JOB_REPO_URL, self.adb_serial)


    def machine_install(self, build_url=None, build_local_path=None, wipe=True,
                        flash_all=False, os_type=None):
        """Install the DUT.

        @param build_url: The url to use for downloading Android artifacts.
                pattern: http://$devserver:###/static/$build. If build_url is
                set to None, the code may try _parser.options.image to do the
                installation. If none of them is set, machine_install will fail.
        @param build_local_path: The path to a local directory that contains the
                image files needed to provision the device.
        @param wipe: If true, userdata will be wiped before flashing.
        @param flash_all: If True, all img files found in img_path will be
                flashed. Otherwise, only boot and system are flashed.

        @returns A tuple of (image_name, host_attributes).
                image_name is the name of image installed, e.g.,
                git_mnc-release/shamu-userdebug/1234
                host_attributes is a dictionary of (attribute, value), which
                can be saved to afe_host_attributes table in database. This
                method returns a dictionary with a single entry of
                `job_repo_url_[adb_serial]`: devserver_url, where devserver_url
                is a url to the build staged on devserver.
        """
        os_type = os_type or self.get_os_type()
        if not build_url and self._parser.options.image:
            build_url, _ = self.stage_build_for_install(
                    self._parser.options.image, os_type=os_type)
        if os_type == OS_TYPE_ANDROID:
            self.install_android(
                    build_url=build_url, build_local_path=build_local_path,
                    wipe=wipe, flash_all=flash_all)
        elif os_type == OS_TYPE_BRILLO:
            self.install_brillo(
                    build_url=build_url, build_local_path=build_local_path)
        else:
            raise error.InstallError(
                    'Installation of os type %s is not supported.' %
                    self.get_os_type())
        return (build_url.split('static/')[-1],
                {self.job_repo_url_attribute: build_url})


    def list_files_glob(self, path_glob):
        """Get a list of files on the device given glob pattern path.

        @param path_glob: The path glob that we want to return the list of
                files that match the glob.  Relative paths will not work as
                expected.  Supply an absolute path to get the list of files
                you're hoping for.

        @returns List of files that match the path_glob.
        """
        # This is just in case path_glob has no path separator.
        base_path = os.path.dirname(path_glob) or '.'
        result = self.run('find %s -path \'%s\' -print' %
                          (base_path, path_glob), ignore_status=True)
        if result.exit_status != 0:
            return []
        return result.stdout.splitlines()


    @retry.retry(error.GenericHostRunError,
                 timeout_min=DISABLE_PACKAGE_VERIFICATION_TIMEOUT_MIN)
    def disable_package_verification(self):
        """Disables package verification on an android device.

        Disables the package verificatoin manager allowing any package to be
        installed without checking
        """
        logging.info('Disabling package verification on %s.', self.adb_serial)
        self.check_boot_to_adb_complete()
        self.run('am broadcast -a '
                 'com.google.gservices.intent.action.GSERVICES_OVERRIDE -e '
                 'global:package_verifier_enable 0')


    @retry.retry(error.GenericHostRunError, timeout_min=APK_INSTALL_TIMEOUT_MIN)
    def install_apk(self, apk, force_reinstall=True):
        """Install the specified apk.

        This will install the apk and override it if it's already installed and
        will also allow for downgraded apks.

        @param apk: The path to apk file.
        @param force_reinstall: True to reinstall the apk even if it's already
                installed. Default is set to True.

        @returns a CMDResult object.
        """
        try:
            client_utils.poll_for_condition(
                    lambda: self.run('pm list packages',
                                     ignore_status=True).exit_status == 0,
                    timeout=120)
            client_utils.poll_for_condition(
                    lambda: self.run('service list | grep mount',
                                     ignore_status=True).exit_status == 0,
                    timeout=120)
            return self.adb_run('install %s -d %s' %
                                ('-r' if force_reinstall else '', apk))
        except error.GenericHostRunError:
            self.reboot()
            raise


    def uninstall_package(self, package):
        """Remove the specified package.

        @param package: Android package name.

        @raises GenericHostRunError: uninstall failed
        """
        result = self.adb_run('uninstall %s' % package)

        if self.is_apk_installed(package):
            raise error.GenericHostRunError('Uninstall of "%s" failed.'
                                            % package, result)

    def save_info(self, results_dir, include_build_info=True):
        """Save info about this device.

        @param results_dir: The local directory to store the info in.
        @param include_build_info: If true this will include the build info
                                   artifact.
        """
        if include_build_info:
            teststation_temp_dir = self.teststation.get_tmp_dir()

            try:
                info = self.host_info_store.get()
            except host_info.StoreError:
                logging.warning(
                    'Device %s could not get repo url for build info.',
                    self.adb_serial)
                return

            job_repo_url = info.attributes.get(self.job_repo_url_attribute, '')
            if not job_repo_url:
                logging.warning(
                    'Device %s could not get repo url for build info.',
                    self.adb_serial)
                return

            build_info = ADBHost.get_build_info_from_build_url(job_repo_url)

            target = build_info['target']
            branch = build_info['branch']
            build_id = build_info['build_id']

            devserver_url = dev_server.AndroidBuildServer.get_server_url(
                    job_repo_url)
            ds = dev_server.AndroidBuildServer(devserver_url)

            ds.trigger_download(target, build_id, branch, files='BUILD_INFO',
                                synchronous=True)

            pull_base_url = ds.get_pull_url(target, build_id, branch)

            source_path = os.path.join(teststation_temp_dir, 'BUILD_INFO')

            self.download_file(pull_base_url, 'BUILD_INFO',
                               teststation_temp_dir)

            destination_path = os.path.join(
                    results_dir, 'BUILD_INFO-%s' % self.adb_serial)
            self.teststation.get_file(source_path, destination_path)



    @retry.retry(error.GenericHostRunError, timeout_min=0.2)
    def _confirm_apk_installed(self, package_name):
        """Confirm if apk is already installed with the given name.

        `pm list packages` command is not reliable some time. The retry helps to
        reduce the chance of false negative.

        @param package_name: Name of the package, e.g., com.android.phone.

        @raise AutoservRunError: If the package is not found or pm list command
                failed for any reason.
        """
        name = 'package:%s' % package_name
        self.adb_run('shell pm list packages | grep -w "%s"' % name)


    def is_apk_installed(self, package_name):
        """Check if apk is already installed with the given name.

        @param package_name: Name of the package, e.g., com.android.phone.

        @return: True if package is installed. False otherwise.
        """
        try:
            self._confirm_apk_installed(package_name)
            return True
        except:
            return False

    @retry.retry(error.GenericHostRunError, timeout_min=1)
    def skip_setup_wizard(self):
        """Skip the setup wizard.

        Skip the starting setup wizard that normally shows up on android.
        """
        logging.info('Skipping setup wizard on %s.', self.adb_serial)
        self.check_boot_to_adb_complete()
        result = self.run('am start -n com.google.android.setupwizard/'
                          '.SetupWizardExitActivity')

        if result.exit_status != 0:
            if result.stdout.startswith('ADB_CMD_OUTPUT:255'):
                # If the result returns ADB_CMD_OUTPUT:255, then run the above
                # as root.
                logging.debug('Need root access to bypass setup wizard.')
                self._restart_adbd_with_root_permissions()
                result = self.run('am start -n com.google.android.setupwizard/'
                                  '.SetupWizardExitActivity')

            if result.stdout == 'ADB_CMD_OUTPUT:0':
                # If the result returns ADB_CMD_OUTPUT:0, Error type 3, then the
                # setup wizard does not exist, so we do not have to bypass it.
                if result.stderr and not \
                        result.stderr.startswith('Error type 3\n'):
                    logging.error('Unrecoverable skip setup wizard failure:'
                                  ' %s', result.stderr)
                    raise error.TestError()
                logging.debug('Skip setup wizard received harmless error: '
                              'No setup to bypass.')

        logging.debug('Bypass setup wizard was successful.')


    def get_attributes_to_clear_before_provision(self):
        """Get a list of attributes to be cleared before machine_install starts.
        """
        return [self.job_repo_url_attribute]


    def get_labels(self):
        """Return a list of the labels gathered from the devices connected.

        @return: A list of strings that denote the labels from all the devices
                 connected.
        """
        return self.labels.get_labels(self)


    def update_labels(self):
        """Update the labels for this testbed."""
        self.labels.update_labels(self)


    def stage_server_side_package(self, image=None):
        """Stage autotest server-side package on devserver.

        @param image: A build name, e.g., git_mnc_dev/shamu-eng/123

        @return: A url to the autotest server-side package.

        @raise: error.AutoservError if fail to locate the build to test with, or
                fail to stage server-side package.
        """
        # If enable_drone_in_restricted_subnet is False, do not set hostname
        # in devserver.resolve call, so a devserver in non-restricted subnet
        # is picked to stage autotest server package for drone to download.
        hostname = self.hostname
        if not utils.ENABLE_DRONE_IN_RESTRICTED_SUBNET:
            hostname = None
        if image:
            ds = dev_server.AndroidBuildServer.resolve(image, hostname)
        else:
            info = self.host_info_store.get()
            job_repo_url = info.attributes.get(self.job_repo_url_attribute)
            if job_repo_url is not None:
                devserver_url, image = (
                        tools.get_devserver_build_from_package_url(
                                job_repo_url, True))
                # If enable_drone_in_restricted_subnet is True, use the
                # existing devserver. Otherwise, resolve a new one in
                # non-restricted subnet.
                if utils.ENABLE_DRONE_IN_RESTRICTED_SUBNET:
                    ds = dev_server.AndroidBuildServer(devserver_url)
                else:
                    ds = dev_server.AndroidBuildServer.resolve(image)
            elif info.build is not None:
                ds = dev_server.AndroidBuildServer.resolve(info.build, hostname)
            else:
                raise error.AutoservError(
                        'Failed to stage server-side package. The host has '
                        'no job_report_url attribute or version label.')

        branch, target, build_id = utils.parse_launch_control_build(image)
        build_target, _ = utils.parse_launch_control_target(target)

        # For any build older than MIN_VERSION_SUPPORT_SSP, server side
        # packaging is not supported.
        try:
            # Some build ids may have special character before the actual
            # number, skip such characters.
            actual_build_id = build_id
            if build_id.startswith('P'):
                actual_build_id = build_id[1:]
            if int(actual_build_id) < self.MIN_VERSION_SUPPORT_SSP:
                raise error.AutoservError(
                        'Build %s is older than %s. Server side packaging is '
                        'disabled.' % (image, self.MIN_VERSION_SUPPORT_SSP))
        except ValueError:
            raise error.AutoservError(
                    'Failed to compare build id in %s with the minimum '
                    'version that supports server side packaging. Server '
                    'side packaging is disabled.' % image)

        ds.stage_artifacts(target, build_id, branch,
                           artifacts=['autotest_server_package'])
        autotest_server_package_name = (AUTOTEST_SERVER_PACKAGE_FILE_FMT %
                                        {'build_target': build_target,
                                         'build_id': build_id})
        return '%s/static/%s/%s' % (ds.url(), image,
                                    autotest_server_package_name)


    def _sync_time(self):
        """Approximate synchronization of time between host and ADB device.

        This sets the ADB/Android device's clock to approximately the same
        time as the Autotest host for the purposes of comparing Android system
        logs such as logcat to logs from the Autotest host system.
        """
        command = 'date '
        sdk_version = int(self.run('getprop %s' % SDK_FILE).stdout)
        if sdk_version < 23:
            # Android L and earlier use this format: date -s (format).
            command += ('-s %s' %
                        datetime.datetime.now().strftime('%Y%m%d.%H%M%S'))
        else:
            # Android M and later use this format: date -u (format).
            command += ('-u %s' %
                        datetime.datetime.utcnow().strftime('%m%d%H%M%Y.%S'))
        self.run(command, timeout=DEFAULT_COMMAND_RETRY_TIMEOUT_SECONDS,
                 ignore_timeout=True)


    def _enable_native_crash_logging(self):
        """Enable native (non-Java) crash logging.
        """
        if self.get_os_type() == OS_TYPE_ANDROID:
            self._enable_android_native_crash_logging()


    def _enable_brillo_native_crash_logging(self):
        """Enables native crash logging for a Brillo DUT.
        """
        try:
            self.run('touch /data/misc/metrics/enabled',
                     timeout=DEFAULT_COMMAND_RETRY_TIMEOUT_SECONDS,
                     ignore_timeout=True)
            # If running, crash_sender will delete crash files every hour.
            self.run('stop crash_sender',
                     timeout=DEFAULT_COMMAND_RETRY_TIMEOUT_SECONDS,
                     ignore_timeout=True)
        except error.GenericHostRunError as e:
            logging.warn(e)
            logging.warn('Failed to enable Brillo native crash logging.')


    def _enable_android_native_crash_logging(self):
        """Enables native crash logging for an Android DUT.
        """
        # debuggerd should be enabled by default on Android.
        result = self.run('pgrep debuggerd',
                          timeout=DEFAULT_COMMAND_RETRY_TIMEOUT_SECONDS,
                          ignore_timeout=True, ignore_status=True)
        if not result or result.exit_status != 0:
            logging.debug('Unable to confirm that debuggerd is running.')


    def _collect_crash_logs(self):
        """Copies crash log files from the DUT to the drone.
        """
        if self.get_os_type() == OS_TYPE_BRILLO:
            self._collect_crash_logs_dut(BRILLO_NATIVE_CRASH_LOG_DIR)
        elif self.get_os_type() == OS_TYPE_ANDROID:
            self._collect_crash_logs_dut(ANDROID_TOMBSTONE_CRASH_LOG_DIR)


    def _collect_crash_logs_dut(self, log_directory):
        """Copies native crash logs from the Android/Brillo DUT to the drone.

        @param log_directory: absolute path of the directory on the DUT where
                log files are stored.
        """
        files = None
        try:
            result = self.run('find %s -maxdepth 1 -type f' % log_directory,
                              timeout=DEFAULT_COMMAND_RETRY_TIMEOUT_SECONDS)
            files = result.stdout.strip().split()
        except (error.GenericHostRunError, error.AutoservSSHTimeout,
                error.CmdTimeoutError):
            logging.debug('Unable to call find %s, unable to find crash logs',
                          log_directory)
        if not files:
            logging.debug('There are no crash logs on the DUT.')
            return

        crash_dir = os.path.join(self.job.resultdir, 'crash')
        try:
            os.mkdir(crash_dir)
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise e

        for f in files:
            logging.debug('DUT native crash file produced: %s', f)
            dest = os.path.join(crash_dir, os.path.basename(f))
            # We've had cases where the crash file on the DUT has permissions
            # "000". Let's override permissions to make them sane for the user
            # collecting the crashes.
            self.get_file(source=f, dest=dest, preserve_perm=False)
