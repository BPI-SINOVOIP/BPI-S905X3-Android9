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


class CommandBuild(base_command_processor.BaseCommandProcessor):
    """Command processor for build command.

    Attributes:
        arg_parser: ConsoleArgumentParser object, argument parser.
        build_thread: dict containing threading.Thread instances(s) that
                      update build info regularly.
        console: cmd.Cmd console object.
        command: string, command name which this processor will handle.
        command_detail: string, detailed explanation for the command.
    """

    command = "build"
    command_detail = "Specifies branches and targets to monitor."

    def UpdateBuild(self, account_id, branch, targets, artifact_type, method,
                    userinfo_file, noauth_local_webserver):
        """Updates the build state.

        Args:
            account_id: string, Partner Android Build account_id to use.
            branch: string, branch to grab the artifact from.
            targets: string, a comma-separate list of build target product(s).
            artifact_type: string, artifact type (`device`, 'gsi' or `test').
            method: string,  method for getting build information.
            userinfo_file: string, the path of a file containing email and
                           password (if method == POST).
            noauth_local_webserver: boolean, True to not use a local websever.
        """
        builds = []

        self.console._build_provider["pab"].Authenticate(
            userinfo_file=userinfo_file,
            noauth_local_webserver=noauth_local_webserver)
        for target in targets.split(","):
            listed_builds = self.console._build_provider["pab"].GetBuildList(
                account_id=account_id,
                branch=branch,
                target=target,
                page_token="",
                max_results=100,
                method=method)

            for listed_build in listed_builds:
                if method == "GET":
                    if "successful" in listed_build:
                        if listed_build["successful"]:
                            build = {}
                            build["manifest_branch"] = branch
                            build["build_id"] = listed_build["build_id"]
                            if "-" in target:
                                build["build_target"], build[
                                    "build_type"] = target.split("-")
                            else:
                                build["build_target"] = target
                                build["build_type"] = ""
                            build["artifact_type"] = artifact_type
                            build["artifacts"] = []
                            builds.append(build)
                    else:
                        print("Error: listed_build %s" % listed_build)
                else:  # POST
                    build = {}
                    build["manifest_branch"] = branch
                    build["build_id"] = listed_build[u"1"]
                    if "-" in target:
                        (build["build_target"],
                         build["build_type"]) = target.split("-")
                    else:
                        build["build_target"] = target
                        build["build_type"] = ""
                    build["artifact_type"] = artifact_type
                    build["artifacts"] = []
                    builds.append(build)
        self.console._vti_endpoint_client.UploadBuildInfo(builds)

    def UpdateBuildLoop(self, account_id, branch, target, artifact_type,
                        method, userinfo_file, noauth_local_webserver,
                        update_interval):
        """Regularly updates the build information.

        Args:
            account_id: string, Partner Android Build account_id to use.
            branch: string, branch to grab the artifact from.
            targets: string, a comma-separate list of build target product(s).
            artifact_type: string, artifcat type (`device`, 'gsi' or `test).
            method: string,  method for getting build information.
            userinfo_file: string, the path of a file containing email and
                           password (if method == POST).
            noauth_local_webserver: boolean, True to not use a local websever.
            update_interval: int, number of seconds before repeating
        """
        thread = threading.currentThread()
        while getattr(thread, 'keep_running', True):
            try:
                self.UpdateBuild(account_id, branch, target, artifact_type,
                                 method, userinfo_file, noauth_local_webserver)
            except (socket.error, remote_operation.RemoteOperationException,
                    httplib2.HttpLib2Error, errors.HttpError) as e:
                logging.exception(e)
            time.sleep(update_interval)

    # @Override
    def SetUp(self):
        """Initializes the parser for build command."""
        self.build_thread = {}
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
            default=30,
            help="Interval (seconds) to repeat build update.")
        self.arg_parser.add_argument(
            "--artifact-type",
            choices=("device", "gsi", "test"),
            default="device",
            help="The type of an artifact to update")
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
            "--method",
            default="GET",
            choices=("GET", "POST"),
            help="Method for getting build information")
        self.arg_parser.add_argument(
            "--userinfo-file",
            help=
            "Location of file containing email and password, if using POST.")
        self.arg_parser.add_argument(
            "--noauth_local_webserver",
            default=False,
            type=bool,
            help="True to not use a local webserver for authentication.")

    # @Override
    def Run(self, arg_line):
        """Updates build info."""
        args = self.arg_parser.ParseLine(arg_line)
        if args.update == "single":
            self.UpdateBuild(args.account_id, args.branch, args.target,
                             args.artifact_type, args.method,
                             args.userinfo_file, args.noauth_local_webserver)
        elif args.update == "list":
            print("Running build update sessions:")
            for id in self.build_thread:
                print("  ID %d", id)
        elif args.update == "start":
            if args.interval <= 0:
                raise ConsoleArgumentError("update interval must be positive")
            # do not allow user to create new
            # thread if one is currently running
            if args.id is None:
                if not self.build_thread:
                    args.id = 1
                else:
                    args.id = max(self.build_thread) + 1
            else:
                args.id = int(args.id)
            if args.id in self.build_thread and not hasattr(
                    self.build_thread[args.id], 'keep_running'):
                print('build update (session ID: %s) already running. '
                      'run build --update stop first.' % args.id)
                return
            self.build_thread[args.id] = threading.Thread(
                target=self.UpdateBuildLoop,
                args=(
                    args.account_id,
                    args.branch,
                    args.target,
                    args.artifact_type,
                    args.method,
                    args.userinfo_file,
                    args.noauth_local_webserver,
                    args.interval,
                ))
            self.build_thread[args.id].daemon = True
            self.build_thread[args.id].start()
        elif args.update == "stop":
            if args.id is None:
                print("--id must be set for stop")
            else:
                self.build_thread[int(args.id)].keep_running = False
