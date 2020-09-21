#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging

from vts.proto import AndroidSystemControlMessage_pb2 as ASysCtrlMsg
from vts.runners.host import errors
from vts.runners.host.tcp_client import vts_tcp_client
from vts.runners.host.tcp_server import callback_server
from vts.utils.python.mirror import hal_mirror
from vts.utils.python.mirror import lib_mirror
from vts.utils.python.mirror import shell_mirror

_DEFAULT_TARGET_BASE_PATHS = ["/system/lib64/hw"]
_DEFAULT_HWBINDER_SERVICE = "default"
_DEFAULT_SHELL_NAME = "_default"


class MirrorTracker(object):
    """The class tracks all mirror objects on the host side.

    Attributes:
        _host_command_port: int, the host-side port for command-response
                            sessions.
        _host_callback_port: int, the host-side port for callback sessions.
        _adb: An AdbProxy object used for interacting with the device via adb.
        _registered_mirrors: dict, key is mirror handler name, value is the
                             mirror object.
        _callback_server: VtsTcpServer, the server that receives and handles
                          callback messages from target side.
    """

    def __init__(self,
                 host_command_port,
                 host_callback_port=None,
                 start_callback_server=False,
                 adb = None):
        self._host_command_port = host_command_port
        self._host_callback_port = host_callback_port
        self._adb = adb
        self._registered_mirrors = {}
        self._callback_server = None
        if start_callback_server:
            self._StartCallbackServer()

    def __del__(self):
        self.CleanUp()

    def CleanUp(self):
        """Shutdown services and release resources held by the registered mirrors.
        """
        for mirror in self._registered_mirrors.values():
            mirror.CleanUp()
        self._registered_mirrors = {}
        if self._callback_server:
            self._callback_server.Stop()
            self._callback_server = None

    def RemoveMirror(self, mirror_name):
        self._registered_mirrors[mirror_name].CleanUp()
        self._registered_mirrors.pop(mirror_name)

    def _StartCallbackServer(self):
        """Starts the callback server.

        Raises:
            errors.ComponentLoadingError is raised if the callback server fails
            to start.
        """
        self._callback_server = callback_server.CallbackServer()
        _, port = self._callback_server.Start(self._host_callback_port)
        if port != self._host_callback_port:
            raise errors.ComponentLoadingError(
                "Failed to start a callback TcpServer at port %s" %
                self._host_callback_port)

    def InitHidlHal(self,
                    target_type,
                    target_version,
                    target_package=None,
                    target_component_name=None,
                    target_basepaths=_DEFAULT_TARGET_BASE_PATHS,
                    handler_name=None,
                    hw_binder_service_name=_DEFAULT_HWBINDER_SERVICE,
                    bits=64):
        """Initiates a handler for a particular HIDL HAL.

        This will initiate a driver service for a HAL on the target side, create
        a mirror object for a HAL, and register it in the tracker.

        Args:
            target_type: string, the target type name (e.g., light, camera).
            target_version: float, the target component version (e.g., 1.0).
            target_package: string, the package name of a target HIDL HAL.
            target_basepaths: list of strings, the paths to look for target
                              files in. Default is _DEFAULT_TARGET_BASE_PATHS.
            handler_name: string, the name of the handler. target_type is used
                          by default.
            hw_binder_service_name: string, the name of a HW binder service.
            bits: integer, processor architecture indicator: 32 or 64.
        """

        if not handler_name:
            handler_name = target_type
        client = vts_tcp_client.VtsTcpClient()
        client.Connect(
            command_port=self._host_command_port,
            callback_port=self._host_callback_port)
        mirror = hal_mirror.HalMirror(client, self._callback_server)
        mirror.InitHalDriver(target_type, target_version, target_package,
                             target_component_name, hw_binder_service_name,
                             handler_name, bits)
        self._registered_mirrors[target_type] = mirror

    def InitSharedLib(self,
                      target_type,
                      target_version,
                      target_basepaths=_DEFAULT_TARGET_BASE_PATHS,
                      target_package="",
                      target_filename=None,
                      handler_name=None,
                      bits=64):
        """Initiates a handler for a particular lib.

        This will initiate a driver service for a lib on the target side, create
        a mirror object for a lib, and register it in the tracker.

        Args:
            target_type: string, the target type name (e.g., light, camera).
            target_version: float, the target component version (e.g., 1.0).
            target_basepaths: list of strings, the paths to look for target
                             files in. Default is _DEFAULT_TARGET_BASE_PATHS.
            target_package: . separated string (e.g., a.b.c) to denote the
                            package name of target component.
            target_filename: string, the target file name (e.g., libm.so).
            handler_name: string, the name of the handler. target_type is used
                          by default.
            bits: integer, processor architecture indicator: 32 or 64.
        """
        if not handler_name:
            handler_name = target_type
        client = vts_tcp_client.VtsTcpClient()
        client.Connect(command_port=self._host_command_port)
        mirror = lib_mirror.LibMirror(client)
        mirror.InitLibDriver(target_type, target_version, target_package,
                             target_filename, target_basepaths, handler_name,
                             bits)
        self._registered_mirrors[handler_name] = mirror

    def InvokeTerminal(self, instance_name, bits=32):
        """Initiates a handler for a particular shell terminal.

        This will initiate a driver service for a shell on the target side,
        create a mirror object for the shell, and register it in the tracker.

        Args:
            instance_name: string, the shell terminal instance name.
            bits: integer, processor architecture indicator: 32 or 64.
        """
        if not instance_name:
            raise error.ComponentLoadingError("instance_name is None")
        if bits not in [32, 64]:
            raise error.ComponentLoadingError("Invalid value for bits: %s" %
                                              bits)

        client = vts_tcp_client.VtsTcpClient()
        client.Connect(command_port=self._host_command_port)

        logging.info("Init the driver service for shell, %s", instance_name)
        launched = client.LaunchDriverService(
            driver_type=ASysCtrlMsg.VTS_DRIVER_TYPE_SHELL,
            service_name="shell_" + instance_name,
            bits=bits)

        if not launched:
            raise errors.ComponentLoadingError(
                "Failed to launch shell driver service %s" % instance_name)

        mirror = shell_mirror.ShellMirror(client, self._adb)
        self._registered_mirrors[instance_name] = mirror

    def DisableShell(self):
        """Disables all registered shell mirrors."""
        for mirror in self._registered_mirrors.values():
            if not isinstance(mirror, shell_mirror.ShellMirror):
                logging.error("mirror object is not a shell mirror")
                continue
            mirror.enabled = False

    def Execute(self, command, no_except=False):
        """Execute a shell command with default shell terminal."""
        if _DEFAULT_SHELL_NAME not in self._registered_mirrors:
            self.InvokeTerminal(_DEFAULT_SHELL_NAME)
        return getattr(self, _DEFAULT_SHELL_NAME).Execute(command, no_except)

    def SetConnTimeout(self, timeout):
        """Set remove shell connection timeout for default shell terminal.

        Args:
            timeout: int, TCP connection timeout in seconds.
        """
        if _DEFAULT_SHELL_NAME not in self._registered_mirrors:
            self.InvokeTerminal(_DEFAULT_SHELL_NAME)
        getattr(self, _DEFAULT_SHELL_NAME).SetConnTimeout(timeout)

    def __getattr__(self, name):
        if name in self._registered_mirrors:
            return self._registered_mirrors[name]
        else:
            logging.error("No mirror found with name: %s", name)
            return None
