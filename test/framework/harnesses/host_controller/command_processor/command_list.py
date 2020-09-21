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

from host_controller.command_processor import base_command_processor


class CommandList(base_command_processor.BaseCommandProcessor):
    """Command processor for list command.

    Attributes:
        arg_parser: ConsoleArgumentParser object, argument parser.
        console: cmd.Cmd console object.
        command: string, command name which this processor will handle.
        command_detail: string, detailed explanation for the command.
    """

    command = "list"
    command_detail = "Show information about the hosts."

    def _PrintHosts(self, hosts):
        """Shows a list of host controllers.

        Args:
            hosts: A list of HostController objects.
        """
        self.console._Print("index  name")
        for ind, host in enumerate(hosts):
            self.console._Print("[%3d]  %s" % (ind, host.hostname))

    def _PrintDevices(self, devices):
        """Shows a list of devices.

        Args:
            devices: A list of DeviceInfo objects.
        """
        attr_names = ("device_serial", "state", "run_target", "build_id",
                      "sdk_version", "stub")
        self.console._PrintObjects(devices, attr_names)

    # @Override
    def SetUp(self):
        """Initializes the parser for list command."""
        self.arg_parser.add_argument(
            "--host", type=int, help="The index of the host.")
        self.arg_parser.add_argument(
            "type",
            choices=("hosts", "devices"),
            help="The type of the shown objects.")

    # @Override
    def Run(self, arg_line):
        """Shows information about the hosts."""
        args = self.arg_parser.ParseLine(arg_line)
        if args.host is None:
            hosts = enumerate(self.console._hosts)
        else:
            hosts = [(args.host, self.console._hosts[args.host])]
        if args.type == "hosts":
            self._PrintHosts(self.console._hosts)
        elif args.type == "devices":
            for ind, host in hosts:
                devices = host.ListDevices()
                self.console._Print("[%3d]  %s" % (ind, host.hostname))
                self._PrintDevices(devices)

    def Help(self):
        base_command_processor.BaseCommandProcessor.Help(self)
        print("Sample: build --target=aosp_sailfish-userdebug "
              "--branch=<branch name> --artifact-type=device")