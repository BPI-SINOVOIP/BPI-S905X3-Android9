#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import httplib2
import logging
import socket
import threading
import time

from googleapiclient import errors

from host_controller import common
from host_controller.command_processor import base_command_processor
from host_controller.console_argument_parser import ConsoleArgumentError
from host_controller.tradefed import remote_operation

from vts.utils.python.common import cmd_utils


class CommandDevice(base_command_processor.BaseCommandProcessor):
    """Command processor for Device command.

    Attributes:
        arg_parser: ConsoleArgumentParser object, argument parser.
        console: cmd.Cmd console object.
        command: string, command name which this processor will handle.
        command_detail: string, detailed explanation for the command.
        update_thread: threading.Thread that updates device state regularly.
    """

    command = "device"
    command_detail = "Selects device(s) under test."

    def UpdateDevice(self, server_type, host, lease):
        """Updates the device state of all devices on a given host.

        Args:
            server_type: string, the type of a test secheduling server.
            host: HostController object
            lease: boolean, True to lease and execute jobs.
        """
        if server_type == "vti":
            devices = []

            stdout, stderr, returncode = cmd_utils.ExecuteOneShellCommand(
                "adb devices")

            lines = stdout.split("\n")[1:]
            for line in lines:
                if len(line.strip()):
                    device = {}
                    device["serial"] = line.split()[0]
                    serial = device["serial"]

                    if (self.console.device_status[serial] !=
                            common._DEVICE_STATUS_DICT["use"]):
                        stdout, _, retcode = cmd_utils.ExecuteOneShellCommand(
                            "adb -s %s shell getprop ro.product.board" %
                            device["serial"])
                        if retcode == 0:
                            device["product"] = stdout.strip()
                        else:
                            device["product"] = "error"

                        self.console.device_status[
                            serial] = common._DEVICE_STATUS_DICT["online"]

                        device["status"] = self.console.device_status[serial]
                        devices.append(device)

            stdout, stderr, returncode = cmd_utils.ExecuteOneShellCommand(
                "fastboot devices")
            lines = stdout.split("\n")
            for line in lines:
                if len(line.strip()):
                    device = {}
                    device["serial"] = line.split()[0]
                    serial = device["serial"]

                    if (self.console.device_status[serial] !=
                            common._DEVICE_STATUS_DICT["use"]):
                        _, stderr, retcode = cmd_utils.ExecuteOneShellCommand(
                            "fastboot -s %s getvar product" % device["serial"])
                        if retcode == 0:
                            res = stderr.splitlines()[0].rstrip()
                            if ":" in res:
                                device["product"] = res.split(":")[1].strip()
                            else:
                                device["product"] = "error"
                        else:
                            device["product"] = "error"
                        self.console.device_status[
                            serial] = common._DEVICE_STATUS_DICT["fastboot"]

                        device["status"] = self.console.device_status[serial]
                        devices.append(device)

            self.console._vti_endpoint_client.UploadDeviceInfo(
                host.hostname, devices)

            if lease:
                self.console._job_in_queue.put("lease")
        elif server_type == "tfc":
            devices = host.ListDevices()
            for device in devices:
                device.Extend(['sim_state', 'sim_operator', 'mac_address'])
            snapshots = self.console._tfc_client.CreateDeviceSnapshot(
                host._cluster_ids[0], host.hostname, devices)
            self.console._tfc_client.SubmitHostEvents([snapshots])
        else:
            print "Error: unknown server_type %s for UpdateDevice" % server_type

    def UpdateDeviceRepeat(self, server_type, host, lease, update_interval):
        """Regularly updates the device state of devices on a given host.

        Args:
            server_type: string, the type of a test secheduling server.
            host: HostController object
            lease: boolean, True to lease and execute jobs.
            update_interval: int, number of seconds before repeating
        """
        thread = threading.currentThread()
        while getattr(thread, 'keep_running', True):
            try:
                self.UpdateDevice(server_type, host, lease)
            except (socket.error, remote_operation.RemoteOperationException,
                    httplib2.HttpLib2Error, errors.HttpError) as e:
                logging.exception(e)
            time.sleep(update_interval)

    # @Override
    def SetUp(self):
        """Initializes the parser for device command."""
        self.update_thread = None
        self.arg_parser.add_argument(
            "--set_serial",
            default="",
            help="Serial number for device. Can be a comma-separated list.")
        self.arg_parser.add_argument(
            "--update",
            choices=("single", "start", "stop"),
            default="start",
            help="Update device info on cloud scheduler")
        self.arg_parser.add_argument(
            "--interval",
            type=int,
            default=30,
            help="Interval (seconds) to repeat device update.")
        self.arg_parser.add_argument(
            "--host", type=int, help="The index of the host.")
        self.arg_parser.add_argument(
            "--server_type",
            choices=("vti", "tfc"),
            default="vti",
            help="The type of a cloud-based test scheduler server.")
        self.arg_parser.add_argument(
            "--lease",
            default=False,
            type=bool,
            help="Whether to lease jobs and execute them.")

    # @Override
    def Run(self, arg_line):
        """Sets device info such as serial number."""
        args = self.arg_parser.ParseLine(arg_line)
        if args.set_serial:
            self.console.SetSerials(args.set_serial.split(","))
            print("serials: %s" % self.console._serials)
        if args.update:
            if args.host is None:
                if len(self.console._hosts) > 1:
                    raise ConsoleArgumentError("More than one host.")
                args.host = 0
            host = self.console._hosts[args.host]
            if args.update == "single":
                self.UpdateDevice(args.server_type, host, args.lease)
            elif args.update == "start":
                if args.interval <= 0:
                    raise ConsoleArgumentError(
                        "update interval must be positive")
                # do not allow user to create new
                # thread if one is currently running
                if self.update_thread is not None and not hasattr(
                        self.update_thread, 'keep_running'):
                    print('device update already running. '
                          'run device --update stop first.')
                    return
                self.update_thread = threading.Thread(
                    target=self.UpdateDeviceRepeat,
                    args=(
                        args.server_type,
                        host,
                        args.lease,
                        args.interval,
                    ))
                self.update_thread.daemon = True
                self.update_thread.start()
            elif args.update == "stop":
                self.update_thread.keep_running = False
