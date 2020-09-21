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
import random
import socket
import subprocess
import time

from vts.runners.host import const
from vts.utils.python.common import cmd_utils


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


SL4A_LAUNCH_CMD = (
    "am start -a com.googlecode.android_scripting.action.LAUNCH_SERVER "
    "--ei com.googlecode.android_scripting.extra.USE_SERVICE_PORT {} "
    "com.googlecode.android_scripting/.activity.ScriptingLayerServiceLauncher")


def get_available_host_port():
    """Gets a host port number available for adb forward.

    Returns:
        An integer representing a port number on the host available for adb
        forward.
    """
    while True:
        port = random.randint(1024, 9900)
        if is_port_available(port):
            return port


def is_port_available(port):
    """Checks if a given port number is available on the system.

    Args:
        port: An integer which is the port number to check.

    Returns:
        True if the port is available; False otherwise.
    """
    # Make sure adb is not using this port so we don't accidentally interrupt
    # ongoing runs by trying to bind to the port.
    if port in list_occupied_adb_ports():
        return False
    s = None
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(('localhost', port))
        return True
    except socket.error:
        return False
    finally:
        if s:
            s.close()


def list_occupied_adb_ports():
    """Lists all the host ports occupied by adb forward.

    This is useful because adb will silently override the binding if an attempt
    to bind to a port already used by adb was made, instead of throwing binding
    error. So one should always check what ports adb is using before trying to
    bind to a port with adb.

    Returns:
        A list of integers representing occupied host ports.
    """
    out = AdbProxy().forward("--list")
    clean_lines = str(out, 'utf-8').strip().split('\n')
    used_ports = []
    for line in clean_lines:
        tokens = line.split(" tcp:")
        if len(tokens) != 3:
            continue
        used_ports.append(int(tokens[1]))
    return used_ports


class AdbProxy():
    """Proxy class for ADB.

    For syntactic reasons, the '-' in adb commands need to be replaced with
    '_'. Can directly execute adb commands on an object:
    >> adb = AdbProxy(<serial>)
    >> adb.start_server()
    >> adb.devices() # will return the console output of "adb devices".
    """

    def __init__(self, serial="", log=None):
        self.serial = serial
        if serial:
            self.adb_str = "adb -s {}".format(serial)
        else:
            self.adb_str = "adb"
        self.log = log

    def _exec_cmd(self, cmd, no_except=False, timeout=None):
        """Executes adb commands in a new shell.

        This is specific to executing adb binary because stderr is not a good
        indicator of cmd execution status.

        Args:
            cmd: string, the adb command to execute.
            no_except: bool, controls whether exception can be thrown.
            timeout: float, timeout in seconds. If the command times out, the
                     exit code is not 0.

        Returns:
            The output of the adb command run if the exit code is 0 and if
            exceptions are allowed. Otherwise, returns a dictionary containing
            stdout, stderr, and exit code.

        Raises:
            AdbError if the adb command exit code is not 0 and exceptions are
            allowed.
        """
        out, err, ret = cmd_utils.ExecuteOneShellCommand(cmd, timeout)
        logging.debug("cmd: %s, stdout: %s, stderr: %s, ret: %s", cmd, out,
                      err, ret)
        if no_except:
            return {
                const.STDOUT: out,
                const.STDERR: err,
                const.EXIT_CODE: ret,
            }
        else:
            if ret == 0:
                return out
            else:
                raise AdbError(cmd=cmd, stdout=out, stderr=err, ret_code=ret)

    def tcp_forward(self, host_port, device_port):
        """Starts TCP forwarding.

        Args:
            host_port: Port number to use on the computer.
            device_port: Port number to use on the android device.
        """
        self.forward("tcp:{} tcp:{}".format(host_port, device_port))

    def reverse_tcp_forward(self, device_port, host_port):
        """Starts reverse TCP forwarding.

        Args:
            device_port: Port number to use on the android device.
            host_port: Port number to use on the computer.
        """
        self.reverse("tcp:{} tcp:{}".format(device_port, host_port))

    def __getattr__(self, name):

        def adb_call(*args, **kwargs):
            clean_name = name.replace('_', '-')
            arg_str = ' '.join(str(elem) for elem in args)
            return self._exec_cmd(' '.join((self.adb_str, clean_name, arg_str)),
                                  **kwargs)

        return adb_call
