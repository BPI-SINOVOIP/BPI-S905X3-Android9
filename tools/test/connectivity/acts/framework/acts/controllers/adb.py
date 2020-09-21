#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from builtins import str

import logging
import re
import shellescape

from acts.libs.proc import job

DEFAULT_ADB_TIMEOUT = 60
DEFAULT_ADB_PULL_TIMEOUT = 180
# Uses a regex to be backwards compatible with previous versions of ADB
# (N and above add the serial to the error msg).
DEVICE_NOT_FOUND_REGEX = re.compile('^error: device (?:\'.*?\' )?not found')
DEVICE_OFFLINE_REGEX = re.compile('^error: device offline')
ROOT_USER_ID = '0'
SHELL_USER_ID = '2000'


def parsing_parcel_output(output):
    """Parsing the adb output in Parcel format.

    Parsing the adb output in format:
      Result: Parcel(
        0x00000000: 00000000 00000014 00390038 00340031 '........8.9.1.4.'
        0x00000010: 00300038 00300030 00300030 00340032 '8.0.0.0.0.0.2.4.'
        0x00000020: 00350034 00330035 00320038 00310033 '4.5.5.3.8.2.3.1.'
        0x00000030: 00000000                            '....            ')
    """
    output = ''.join(re.findall(r"'(.*)'", output))
    return re.sub(r'[.\s]', '', output)


class AdbError(Exception):
    """Raised when there is an error in adb operations."""

    def __init__(self, cmd, stdout, stderr, ret_code):
        self.cmd = cmd
        self.stdout = stdout
        self.stderr = stderr
        self.ret_code = ret_code

    def __str__(self):
        return ("Error executing adb cmd '%s'. ret: %d, stdout: %s, stderr: %s"
                ) % (self.cmd, self.ret_code, self.stdout, self.stderr)


class AdbProxy(object):
    """Proxy class for ADB.

    For syntactic reasons, the '-' in adb commands need to be replaced with
    '_'. Can directly execute adb commands on an object:
    >> adb = AdbProxy(<serial>)
    >> adb.start_server()
    >> adb.devices() # will return the console output of "adb devices".
    """

    _SERVER_LOCAL_PORT = None

    def __init__(self, serial="", ssh_connection=None):
        """Construct an instance of AdbProxy.

        Args:
            serial: str serial number of Android device from `adb devices`
            ssh_connection: SshConnection instance if the Android device is
                            connected to a remote host that we can reach via SSH.
        """
        self.serial = serial
        adb_path = self._exec_cmd("which adb")
        adb_cmd = [adb_path]
        if serial:
            adb_cmd.append("-s %s" % serial)
        if ssh_connection is not None and not AdbProxy._SERVER_LOCAL_PORT:
            # Kill all existing adb processes on the remote host (if any)
            # Note that if there are none, then pkill exits with non-zero status
            ssh_connection.run("pkill adb", ignore_status=True)
            # Copy over the adb binary to a temp dir
            temp_dir = ssh_connection.run("mktemp -d").stdout.strip()
            ssh_connection.send_file(adb_path, temp_dir)
            # Start up a new adb server running as root from the copied binary.
            remote_adb_cmd = "%s/adb %s root" % (temp_dir, "-s %s" % serial
                                                 if serial else "")
            ssh_connection.run(remote_adb_cmd)
            # Proxy a local port to the adb server port
            local_port = ssh_connection.create_ssh_tunnel(5037)
            AdbProxy._SERVER_LOCAL_PORT = local_port

        if AdbProxy._SERVER_LOCAL_PORT:
            adb_cmd.append("-P %d" % local_port)
        self.adb_str = " ".join(adb_cmd)
        self._ssh_connection = ssh_connection

    def get_user_id(self):
        """Returns the adb user. Either 2000 (shell) or 0 (root)."""
        return self.shell('id -u')

    def is_root(self, user_id=None):
        """Checks if the user is root.

        Args:
            user_id: if supplied, the id to check against.
        Returns:
            True if the user is root. False otherwise.
        """
        if not user_id:
            user_id = self.get_user_id()
        return user_id == ROOT_USER_ID

    def ensure_root(self):
        """Ensures the user is root after making this call.

        Note that this will still fail if the device is a user build, as root
        is not accessible from a user build.

        Returns:
            False if the device is a user build. True otherwise.
        """
        self.ensure_user(ROOT_USER_ID)
        return self.is_root()

    def ensure_user(self, user_id=SHELL_USER_ID):
        """Ensures the user is set to the given user.

        Args:
            user_id: The id of the user.
        """
        if self.is_root(user_id):
            self.root()
        else:
            self.unroot()
        self.wait_for_device()
        return self.get_user_id() == user_id

    def _exec_cmd(self, cmd, ignore_status=False, timeout=DEFAULT_ADB_TIMEOUT):
        """Executes adb commands in a new shell.

        This is specific to executing adb commands.

        Args:
            cmd: A string that is the adb command to execute.

        Returns:
            The stdout of the adb command.

        Raises:
            AdbError is raised if adb cannot find the device.
        """
        result = job.run(cmd, ignore_status=True, timeout=timeout)
        ret, out, err = result.exit_status, result.stdout, result.stderr

        if DEVICE_OFFLINE_REGEX.match(err):
            raise AdbError(cmd=cmd, stdout=out, stderr=err, ret_code=ret)
        if "Result: Parcel" in out:
            return parsing_parcel_output(out)
        if ignore_status:
            return out or err
        if ret == 1 and DEVICE_NOT_FOUND_REGEX.match(err):
            raise AdbError(cmd=cmd, stdout=out, stderr=err, ret_code=ret)
        else:
            return out

    def _exec_adb_cmd(self, name, arg_str, **kwargs):
        return self._exec_cmd(' '.join((self.adb_str, name, arg_str)),
                              **kwargs)

    def _exec_cmd_nb(self, cmd, **kwargs):
        """Executes adb commands in a new shell, non blocking.

        Args:
            cmds: A string that is the adb command to execute.

        """
        return job.run_async(cmd, **kwargs)

    def _exec_adb_cmd_nb(self, name, arg_str, **kwargs):
        return self._exec_cmd_nb(' '.join((self.adb_str, name, arg_str)),
                                 **kwargs)

    def tcp_forward(self, host_port, device_port):
        """Starts tcp forwarding from localhost to this android device.

        Args:
            host_port: Port number to use on localhost
            device_port: Port number to use on the android device.

        Returns:
            The command output for the forward command.
        """
        if self._ssh_connection:
            # We have to hop through a remote host first.
            #  1) Find some free port on the remote host's localhost
            #  2) Setup forwarding between that remote port and the requested
            #     device port
            remote_port = self._ssh_connection.find_free_port()
            self._ssh_connection.create_ssh_tunnel(
                remote_port, local_port=host_port)
            host_port = remote_port
        return self.forward("tcp:%d tcp:%d" % (host_port, device_port))

    def remove_tcp_forward(self, host_port):
        """Stop tcp forwarding a port from localhost to this android device.

        Args:
            host_port: Port number to use on localhost
        """
        if self._ssh_connection:
            remote_port = self._ssh_connection.close_ssh_tunnel(host_port)
            if remote_port is None:
                logging.warning("Cannot close unknown forwarded tcp port: %d",
                                host_port)
                return
            # The actual port we need to disable via adb is on the remote host.
            host_port = remote_port
        self.forward("--remove tcp:%d" % host_port)

    def getprop(self, prop_name):
        """Get a property of the device.

        This is a convenience wrapper for "adb shell getprop xxx".

        Args:
            prop_name: A string that is the name of the property to get.

        Returns:
            A string that is the value of the property, or None if the property
            doesn't exist.
        """
        return self.shell("getprop %s" % prop_name)

    # TODO: This should be abstracted out into an object like the other shell
    # command.
    def shell(self, command, ignore_status=False, timeout=DEFAULT_ADB_TIMEOUT):
        return self._exec_adb_cmd(
            'shell',
            shellescape.quote(command),
            ignore_status=ignore_status,
            timeout=timeout)

    def shell_nb(self, command):
        return self._exec_adb_cmd_nb('shell', shellescape.quote(command))

    def pull(self,
             command,
             ignore_status=False,
             timeout=DEFAULT_ADB_PULL_TIMEOUT):
        return self._exec_adb_cmd(
            'pull', command, ignore_status=ignore_status, timeout=timeout)

    def __getattr__(self, name):
        def adb_call(*args, **kwargs):
            clean_name = name.replace('_', '-')
            arg_str = ' '.join(str(elem) for elem in args)
            return self._exec_adb_cmd(clean_name, arg_str, **kwargs)

        return adb_call
