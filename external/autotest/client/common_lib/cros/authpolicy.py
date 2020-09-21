# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Wrapper for D-Bus calls ot the AuthPolicy daemon.
"""

import logging
import os
import sys

import dbus

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils


class AuthPolicy(object):
    """
    Wrapper for D-Bus calls ot the AuthPolicy daemon.

    The AuthPolicy daemon handles Active Directory domain join, user
    authentication and policy fetch. This class is a wrapper around the D-Bus
    interface to the daemon.

    """

    # Log file written by authpolicyd.
    _LOG_FILE = '/var/log/authpolicy.log'

    # Number of log lines to include in error logs.
    _LOG_LINE_LIMIT = 50

    # The usual system log file (minijail logs there!).
    _SYSLOG_FILE = '/var/log/messages'

    # Authpolicy daemon D-Bus parameters.
    _DBUS_SERVICE_NAME = 'org.chromium.AuthPolicy'
    _DBUS_SERVICE_PATH = '/org/chromium/AuthPolicy'
    _DBUS_INTERFACE_NAME = 'org.chromium.AuthPolicy'

    # Default timeout in seconds for D-Bus calls.
    _DEFAULT_TIMEOUT = 120

    # Chronos user ID.
    _CHRONOS_UID = 1000

    def __init__(self, bus_loop, proto_binding_location):
        """
        Constructor

        Creates and returns a D-Bus connection to authpolicyd. The daemon must
        be running.

        @param bus_loop: glib main loop object.
        @param proto_binding_location: the location of generated python bindings
                                       for authpolicy protobufs.
        """

        # Pull in protobuf bindings.
        sys.path.append(proto_binding_location)

        try:
            # Get the interface as Chronos since only they are allowed to send
            # D-Bus messages to authpolicyd.
            os.setresuid(self._CHRONOS_UID, self._CHRONOS_UID, 0)
            bus = dbus.SystemBus(bus_loop)

            proxy = bus.get_object(self._DBUS_SERVICE_NAME,
                                   self._DBUS_SERVICE_PATH)
            self._authpolicyd = dbus.Interface(proxy, self._DBUS_INTERFACE_NAME)
        finally:
            os.setresuid(0, 0, 0)

    def __del__(self):
        """
        Destructor

        Turns debug logs off.

        """

        self.set_default_log_level(0)

    def join_ad_domain(self,
                       user_principal_name,
                       password,
                       machine_name,
                       machine_domain=None,
                       machine_ou=None):
        """
        Joins a machine (=device) to an Active Directory domain.

        @param user_principal_name: Logon name of the user (with @realm) who
            joins the machine to the domain.
        @param password: Password corresponding to user_principal_name.
        @param machine_name: Netbios computer (aka machine) name for the joining
            device.
        @param machine_domain: Domain (realm) the machine should be joined to.
            If not specified, the machine is joined to the user's realm.
        @param machine_ou: Array of organizational units (OUs) from leaf to
            root. The machine is put into the leaf OU. If not specified, the
            machine account is created in the default 'Computers' OU.

        @return A tuple with the ErrorType and the joined domain returned by the
            D-Bus call.

        """

        from active_directory_info_pb2 import JoinDomainRequest

        request = JoinDomainRequest()
        request.user_principal_name = user_principal_name
        request.machine_name = machine_name
        if machine_ou:
            request.machine_ou.extend(machine_ou)
        if machine_domain:
            request.machine_domain = machine_domain

        with self.PasswordFd(password) as password_fd:
            return self._authpolicyd.JoinADDomain(
                dbus.ByteArray(request.SerializeToString()),
                dbus.types.UnixFd(password_fd),
                timeout=self._DEFAULT_TIMEOUT,
                byte_arrays=True)

    def authenticate_user(self, user_principal_name, account_id, password):
        """
        Authenticates a user with an Active Directory domain.

        @param user_principal_name: User logon name (user@example.com) for the
            Active Directory domain.
        #param account_id: User account id (aka objectGUID). May be empty.
        @param password: Password corresponding to user_principal_name.

        @return A tuple with the ErrorType and an ActiveDirectoryAccountInfo
                blob string returned by the D-Bus call.

        """

        from active_directory_info_pb2 import ActiveDirectoryAccountInfo
        from active_directory_info_pb2 import AuthenticateUserRequest
        from active_directory_info_pb2 import ERROR_NONE

        request = AuthenticateUserRequest()
        request.user_principal_name = user_principal_name
        if account_id:
            request.account_id = account_id

        with self.PasswordFd(password) as password_fd:
            error_value, account_info_blob = self._authpolicyd.AuthenticateUser(
                dbus.ByteArray(request.SerializeToString()),
                dbus.types.UnixFd(password_fd),
                timeout=self._DEFAULT_TIMEOUT,
                byte_arrays=True)
            account_info = ActiveDirectoryAccountInfo()
            if error_value == ERROR_NONE:
                account_info.ParseFromString(account_info_blob)
            return error_value, account_info

    def refresh_user_policy(self, account_id):
        """
        Fetches user policy and sends it to Session Manager.

        @param account_id: User account ID (aka objectGUID).

        @return ErrorType from the D-Bus call.

        """

        return self._authpolicyd.RefreshUserPolicy(
            dbus.String(account_id),
            timeout=self._DEFAULT_TIMEOUT,
            byte_arrays=True)

    def refresh_device_policy(self):
        """
        Fetches device policy and sends it to Session Manager.

        @return ErrorType from the D-Bus call.

        """

        return self._authpolicyd.RefreshDevicePolicy(
            timeout=self._DEFAULT_TIMEOUT, byte_arrays=True)

    def set_default_log_level(self, level):
        """
        Fetches device policy and sends it to Session Manager.

        @param level: Log level, 0=quiet, 1=taciturn, 2=chatty, 3=verbose.

        @return error_message: Error message, empty if no error occurred.

        """

        return self._authpolicyd.SetDefaultLogLevel(level, byte_arrays=True)

    def print_log_tail(self):
        """
        Prints out authpolicyd log tail. Catches and prints out errors.

        """

        try:
            cmd = 'tail -n %s %s' % (self._LOG_LINE_LIMIT, self._LOG_FILE)
            log_tail = utils.run(cmd).stdout
            logging.info('Tail of %s:\n%s', self._LOG_FILE, log_tail)
        except error.CmdError as ex:
            logging.error('Failed to print authpolicyd log tail: %s', ex)

    def print_seccomp_failure_info(self):
        """
        Detects seccomp failures and prints out the failing syscall.

        """

        # Exit code 253 is minijail's marker for seccomp failures.
        cmd = 'grep -q "failed: exit code 253" %s' % self._LOG_FILE
        if utils.run(cmd, ignore_status=True).exit_status == 0:
            logging.error('Seccomp failure detected!')
            cmd = 'grep -oE "blocked syscall: \\w+" %s | tail -1' % \
                    self._SYSLOG_FILE
            try:
                logging.error(utils.run(cmd).stdout)
                logging.error(
                    'This can happen if you changed a dependency of '
                    'authpolicyd. Consider whitelisting this syscall in '
                    'the appropriate -seccomp.policy file in authpolicyd.'
                    '\n')
            except error.CmdError as ex:
                logging.error(
                    'Failed to determine reason for seccomp issue: %s', ex)

    def clear_log(self):
        """
        Clears the authpolicy daemon's log file.

        """

        try:
            utils.run('echo "" > %s' % self._LOG_FILE)
        except error.CmdError as ex:
            logging.error('Failed to clear authpolicyd log file: %s', ex)

    class PasswordFd(object):
        """
        Writes password into a file descriptor.

        Use in a 'with' statement to automatically close the returned file
        descriptor.

        @param password: Plaintext password string.

        @return A file descriptor (pipe) containing the password.

        """

        def __init__(self, password):
            self._password = password
            self._read_fd = None

        def __enter__(self):
            """Creates the password file descriptor."""
            self._read_fd, write_fd = os.pipe()
            os.write(write_fd, self._password)
            os.close(write_fd)
            return self._read_fd

        def __exit__(self, mytype, value, traceback):
            """Closes the password file descriptor again."""
            if self._read_fd:
                os.close(self._read_fd)
