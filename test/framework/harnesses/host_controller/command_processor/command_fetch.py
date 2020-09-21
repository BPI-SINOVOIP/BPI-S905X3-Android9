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

import logging

from host_controller import common
from host_controller.command_processor import base_command_processor


class CommandFetch(base_command_processor.BaseCommandProcessor):
    """Command processor for fetch command.

    Attributes:
        arg_parser: ConsoleArgumentParser object, argument parser.
        console: cmd.Cmd console object.
        command: string, command name which this processor will handle.
        command_detail: string, detailed explanation for the command.
    """

    command = "fetch"
    command_detail = "Fetch a build artifact."

    # @Override
    def SetUp(self):
        """Initializes the parser for fetch command."""
        self.arg_parser.add_argument(
            '--type',
            default='pab',
            choices=('local_fs', 'gcs', 'pab', 'ab'),
            help='Build provider type')
        self.arg_parser.add_argument(
            '--method',
            default='GET',
            choices=('GET', 'POST'),
            help='Method for fetching')
        self.arg_parser.add_argument(
            "--path",  # required for local_fs
            help="The path of a local directory which keeps the artifacts.")
        self.arg_parser.add_argument(
            "--branch",  # required for pab
            help="Branch to grab the artifact from.")
        self.arg_parser.add_argument(
            "--target",  # required for pab
            help="Target product to grab the artifact from.")
        # TODO(lejonathan): find a way to not specify this?
        self.arg_parser.add_argument(
            "--account_id",
            default=common._DEFAULT_ACCOUNT_ID,
            help="Partner Android Build account_id to use.")
        self.arg_parser.add_argument(
            '--build_id',
            default='latest',
            help='Build ID to use default latest.')
        self.arg_parser.add_argument(
            "--artifact_name",  # required for pab
            help=
            "Name of the artifact to be fetched. {id} replaced with build id.")
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
        """Makes the host download a build artifact from PAB."""
        args = self.arg_parser.ParseLine(arg_line)

        if args.type not in self.console._build_provider:
            print("ERROR: uninitialized fetch type %s" % args.type)
            return False

        provider = self.console._build_provider[args.type]
        if args.type == "pab":
            # do we want this somewhere else? No harm in doing multiple times
            provider.Authenticate(args.userinfo_file,
                                  args.noauth_local_webserver)
            (device_images, test_suites, fetch_environment,
             _) = provider.GetArtifact(
                 account_id=args.account_id,
                 branch=args.branch,
                 target=args.target,
                 artifact_name=args.artifact_name,
                 build_id=args.build_id,
                 method=args.method)
            self.console.fetch_info["build_id"] = fetch_environment["build_id"]
        elif args.type == "local_fs":
            device_images, test_suites = provider.Fetch(args.path)
            self.console.fetch_info["build_id"] = None
        elif args.type == "gcs":
            device_images, test_suites, tools = provider.Fetch(args.path)
            self.console.fetch_info["build_id"] = None
        elif args.type == "ab":
            device_images, test_suites, fetch_environment = provider.Fetch(
                branch=args.branch,
                target=args.target,
                artifact_name=args.artifact_name,
                build_id=args.build_id)
            self.console.fetch_info["build_id"] = fetch_environment["build_id"]
        else:
            print("ERROR: unknown fetch type %s" % args.type)
            return False

        self.console.fetch_info["branch"] = args.branch
        self.console.fetch_info["target"] = args.target

        self.console.device_image_info.update(device_images)
        self.console.test_suite_info.update(test_suites)
        self.console.tools_info.update(provider.GetAdditionalFile())

        if self.console.device_image_info:
            logging.info("device images:\n%s", "\n".join(
                image + ": " + path
                for image, path in self.console.device_image_info.iteritems()))
        if self.console.test_suite_info:
            logging.info("test suites:\n%s", "\n".join(
                suite + ": " + path
                for suite, path in self.console.test_suite_info.iteritems()))
        if self.console.tools_info:
            logging.info("additional files:\n%s", "\n".join(
                rel_path + ": " + full_path
                for rel_path, full_path in
                self.console.tools_info.iteritems()))
