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
import os
import socket
import threading
import time

from googleapiclient import errors
from google.protobuf import text_format

from host_controller import common
from host_controller.command_processor import base_command_processor
from host_controller.console_argument_parser import ConsoleArgumentError
from host_controller.tradefed import remote_operation

from vti.test_serving.proto import TestLabConfigMessage_pb2 as LabCfgMsg
from vti.test_serving.proto import TestScheduleConfigMessage_pb2 as SchedCfgMsg


class CommandConfig(base_command_processor.BaseCommandProcessor):
    """Command processor for config command.

    Attributes:
        arg_parser: ConsoleArgumentParser object, argument parser.
        console: cmd.Cmd console object.
        command: string, command name which this processor will handle.
        command_detail: string, detailed explanation for the command.
        schedule_thread: dict containing threading.Thread instances(s) that
                         update schedule info regularly.
    """

    command = "config"
    command_detail = "Specifies a global config type to monitor."

    def UpdateConfig(self, account_id, branch, targets, config_type, method):
        """Updates the global configuration data.

        Args:
            account_id: string, Partner Android Build account_id to use.
            branch: string, branch to grab the artifact from.
            targets: string, a comma-separate list of build target product(s).
            config_type: string, config type (`prod` or `test').
            method: string, HTTP method for fetching.
        """

        self.console._build_provider["pab"].Authenticate()
        for target in targets.split(","):
            listed_builds = self.console._build_provider["pab"].GetBuildList(
                account_id=account_id,
                branch=branch,
                target=target,
                page_token="",
                max_results=1,
                method="GET")

            if listed_builds and len(listed_builds) > 0:
                listed_build = listed_builds[0]
                if listed_build["successful"]:
                    device_images, test_suites, artifacts, configs = self.console._build_provider[
                        "pab"].GetArtifact(
                            account_id=account_id,
                            branch=branch,
                            target=target,
                            artifact_name=(
                                "vti-global-config-%s.zip" % config_type),
                            build_id=listed_build["build_id"],
                            method=method)
                    base_path = os.path.dirname(configs[config_type])
                    schedules_pbs = []
                    lab_pbs = []
                    for root, dirs, files in os.walk(base_path):
                        for config_file in files:
                            full_path = os.path.join(root, config_file)
                            try:
                                if config_file.endswith(".schedule_config"):
                                    with open(full_path, "r") as fd:
                                        context = fd.read()
                                        sched_cfg_msg = SchedCfgMsg.ScheduleConfigMessage(
                                        )
                                        text_format.Merge(
                                            context, sched_cfg_msg)
                                        schedules_pbs.append(sched_cfg_msg)
                                        print sched_cfg_msg.manifest_branch
                                elif config_file.endswith(".lab_config"):
                                    with open(full_path, "r") as fd:
                                        context = fd.read()
                                        lab_cfg_msg = LabCfgMsg.LabConfigMessage(
                                        )
                                        text_format.Merge(context, lab_cfg_msg)
                                        lab_pbs.append(lab_cfg_msg)
                            except text_format.ParseError as e:
                                print("ERROR: Config parsing error %s" % e)
                    self.console._vti_endpoint_client.UploadScheduleInfo(
                        schedules_pbs)
                    self.console._vti_endpoint_client.UploadLabInfo(lab_pbs)

    def UpdateConfigLoop(self, account_id, branch, target, config_type, method,
                         update_interval):
        """Regularly updates the global configuration.

        Args:
            account_id: string, Partner Android Build account_id to use.
            branch: string, branch to grab the artifact from.
            targets: string, a comma-separate list of build target product(s).
            config_type: string, config type (`prod` or `test').
            method: string, HTTP method for fetching.
            update_interval: int, number of seconds before repeating
        """
        thread = threading.currentThread()
        while getattr(thread, 'keep_running', True):
            try:
                self.UpdateConfig(account_id, branch, target, config_type,
                                  method)
            except (socket.error, remote_operation.RemoteOperationException,
                    httplib2.HttpLib2Error, errors.HttpError) as e:
                logging.exception(e)
            time.sleep(update_interval)

    # @Override
    def SetUp(self):
        """Initializes the parser for config command."""
        self.schedule_thread = {}
        self.arg_parser.add_argument(
            "--update",
            choices=("single", "start", "stop", "list"),
            default="start",
            help="Update build info")
        self.arg_parser.add_argument(
            "--id",
            default=None,
            help="session ID only required for 'stop' update command")
        self.arg_parser.add_argument(
            "--interval",
            type=int,
            default=60,
            help="Interval (seconds) to repeat build update.")
        self.arg_parser.add_argument(
            "--config-type",
            choices=("prod", "test"),
            default="prod",
            help="Whether it's for prod")
        self.arg_parser.add_argument(
            "--branch",
            required=True,
            help="Branch to grab the artifact from.")
        self.arg_parser.add_argument(
            "--target",
            required=True,
            help="a comma-separate list of build target product(s).")
        self.arg_parser.add_argument(
            "--account_id",
            default=common._DEFAULT_ACCOUNT_ID,
            help="Partner Android Build account_id to use.")
        self.arg_parser.add_argument(
            '--method',
            default='GET',
            choices=('GET', 'POST'),
            help='Method for fetching')

    # @Override
    def Run(self, arg_line):
        """Updates global config."""
        args = self.arg_parser.ParseLine(arg_line)
        if args.update == "single":
            self.UpdateConfig(args.account_id, args.branch, args.target,
                              args.config_type, args.method)
        elif args.update == "list":
            print("Running config update sessions:")
            for id in self.schedule_thread:
                print("  ID %d", id)
        elif args.update == "start":
            if args.interval <= 0:
                raise ConsoleArgumentError("update interval must be positive")
            # do not allow user to create new
            # thread if one is currently running
            if args.id is None:
                if not self.schedule_thread:
                    args.id = 1
                else:
                    args.id = max(self.schedule_thread) + 1
            else:
                args.id = int(args.id)
            if args.id in self.schedule_thread and not hasattr(
                    self.schedule_thread[args.id], 'keep_running'):
                print('config update already running. '
                      'run config --update=stop --id=%s first.' % args.id)
                return
            self.schedule_thread[args.id] = threading.Thread(
                target=self.UpdateConfigLoop,
                args=(
                    args.account_id,
                    args.branch,
                    args.target,
                    args.config_type,
                    args.method,
                    args.interval,
                ))
            self.schedule_thread[args.id].daemon = True
            self.schedule_thread[args.id].start()
        elif args.update == "stop":
            if args.id is None:
                print("--id must be set for stop")
            else:
                self.schedule_thread[int(args.id)].keep_running = False

    def Help(self):
        base_command_processor.BaseCommandProcessor.Help(self)
        print("Sample: schedule --target=aosp_sailfish-userdebug "
              "--branch=git_oc-release")
