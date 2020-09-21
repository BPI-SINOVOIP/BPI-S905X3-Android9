#!/usr/bin/env python
#
# Copyright 2016 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

r"""Cloud Android Driver.

This CLI manages google compute engine project for android devices.

- Prerequisites:
  See: go/acloud-manual

- Configuration:
  The script takes a required configuration file, which should look like
     <Start of the file>
     # If using service account
     service_account_name: "your_account@developer.gserviceaccount.com"
     service_account_private_key_path: "/path/to/your-project.p12"

     # If using OAuth2 authentication flow
     client_id: <client id created in the project>
     client_secret: <client secret for the client id>

     # Optional
     ssh_private_key_path: "~/.ssh/acloud_rsa"
     ssh_public_key_path: "~/.ssh/acloud_rsa.pub"
     orientation: "portrait"
     resolution: "800x1280x32x213"
     network: "default"
     machine_type: "n1-standard-1"
     extra_data_disk_size_gb: 10  # 4G or 10G

     # Required
     project: "your-project"
     zone: "us-central1-f"
     storage_bucket_name: "your_google_storage_bucket_name"
     <End of the file>

  Save it at /path/to/acloud.config

- Example calls:
  - Create two instances:
  $ acloud.par create
    --build_target gce_x86_phone-userdebug_fastbuild3c_linux \
    --build_id 3744001 --num 2 --config_file /path/to/acloud.config \
    --report_file /tmp/acloud_report.json --log_file /tmp/acloud.log

  - Delete two instances:
  $ acloud.par delete --instance_names
    ins-b638cdba-3744001-gce-x86-phone-userdebug-fastbuild3c-linux
    --config_file /path/to/acloud.config
    --report_file /tmp/acloud_report.json --log_file /tmp/acloud.log
"""
import argparse
import getpass
import logging
import os
import sys

from acloud.internal import constants
from acloud.public import acloud_common
from acloud.public import config
from acloud.public import device_driver
from acloud.public import errors

LOGGING_FMT = "%(asctime)s |%(levelname)s| %(module)s:%(lineno)s| %(message)s"
LOGGER_NAME = "acloud_main"

# Commands
CMD_CREATE = "create"
CMD_DELETE = "delete"
CMD_CLEANUP = "cleanup"
CMD_SSHKEY = "project_sshkey"


def _ParseArgs(args):
    """Parse args.

    Args:
        args: Argument list passed from main.

    Returns:
        Parsed args.
    """
    usage = ",".join([CMD_CREATE, CMD_DELETE, CMD_CLEANUP, CMD_SSHKEY])
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        usage="%(prog)s {" + usage + "} ...")
    subparsers = parser.add_subparsers()
    subparser_list = []

    # Command "create"
    create_parser = subparsers.add_parser(CMD_CREATE)
    create_parser.required = False
    create_parser.set_defaults(which=CMD_CREATE)
    create_parser.add_argument(
        "--build_target",
        type=str,
        dest="build_target",
        help="Android build target, e.g. gce_x86-userdebug, "
        "or short names: phone, tablet, or tablet_mobile.")
    create_parser.add_argument(
        "--branch",
        type=str,
        dest="branch",
        help="Android branch, e.g. mnc-dev or git_mnc-dev")
    # TODO(fdeng): Support HEAD (the latest build)
    create_parser.add_argument("--build_id",
                               type=str,
                               dest="build_id",
                               help="Android build id, e.g. 2145099, P2804227")
    create_parser.add_argument(
        "--spec",
        type=str,
        dest="spec",
        required=False,
        help="The name of a pre-configured device spec that we are "
        "going to use. Choose from: %s" % ", ".join(constants.SPEC_NAMES))
    create_parser.add_argument("--num",
                               type=int,
                               dest="num",
                               required=False,
                               default=1,
                               help="Number of instances to create.")
    create_parser.add_argument(
        "--gce_image",
        type=str,
        dest="gce_image",
        required=False,
        help="Name of an existing compute engine image to reuse.")
    create_parser.add_argument("--local_disk_image",
                               type=str,
                               dest="local_disk_image",
                               required=False,
                               help="Path to a local disk image to use, "
                               "e.g /tmp/avd-system.tar.gz")
    create_parser.add_argument(
        "--no_cleanup",
        dest="no_cleanup",
        default=False,
        action="store_true",
        help="Do not clean up temporary disk image and compute engine image. "
        "For debugging purposes.")
    create_parser.add_argument(
        "--serial_log_file",
        type=str,
        dest="serial_log_file",
        required=False,
        help="Path to a *tar.gz file where serial logs will be saved "
        "when a device fails on boot.")
    create_parser.add_argument(
        "--logcat_file",
        type=str,
        dest="logcat_file",
        required=False,
        help="Path to a *tar.gz file where logcat logs will be saved "
        "when a device fails on boot.")

    subparser_list.append(create_parser)

    # Command "Delete"
    delete_parser = subparsers.add_parser(CMD_DELETE)
    delete_parser.required = False
    delete_parser.set_defaults(which=CMD_DELETE)
    delete_parser.add_argument(
        "--instance_names",
        dest="instance_names",
        nargs="+",
        required=True,
        help="The names of the instances that need to delete, "
        "separated by spaces, e.g. --instance_names instance-1 instance-2")
    subparser_list.append(delete_parser)

    # Command "cleanup"
    cleanup_parser = subparsers.add_parser(CMD_CLEANUP)
    cleanup_parser.required = False
    cleanup_parser.set_defaults(which=CMD_CLEANUP)
    cleanup_parser.add_argument(
        "--expiration_mins",
        type=int,
        dest="expiration_mins",
        required=True,
        help="Garbage collect all gce instances, gce images, cached disk "
        "images that are older than |expiration_mins|.")
    subparser_list.append(cleanup_parser)

    # Command "project_sshkey"
    sshkey_parser = subparsers.add_parser(CMD_SSHKEY)
    sshkey_parser.required = False
    sshkey_parser.set_defaults(which=CMD_SSHKEY)
    sshkey_parser.add_argument(
        "--user",
        type=str,
        dest="user",
        default=getpass.getuser(),
        help="The user name which the sshkey belongs to, default to: %s." %
        getpass.getuser())
    sshkey_parser.add_argument(
        "--ssh_rsa_path",
        type=str,
        dest="ssh_rsa_path",
        required=True,
        help="Absolute path to the file that contains the public rsa key "
             "that will be added as project-wide ssh key.")
    subparser_list.append(sshkey_parser)

    # Add common arguments.
    for p in subparser_list:
        acloud_common.AddCommonArguments(p)

    return parser.parse_args(args)


def _TranslateAlias(parsed_args):
    """Translate alias to Launch Control compatible values.

    This method translates alias to Launch Control compatible values.
     - branch: "git_" prefix will be added if branch name doesn't have it.
     - build_target: For example, "phone" will be translated to full target
                          name "git_x86_phone-userdebug",

    Args:
        parsed_args: Parsed args.

    Returns:
        Parsed args with its values being translated.
    """
    if parsed_args.which == CMD_CREATE:
        if (parsed_args.branch and
                not parsed_args.branch.startswith(constants.BRANCH_PREFIX)):
            parsed_args.branch = constants.BRANCH_PREFIX + parsed_args.branch
        parsed_args.build_target = constants.BUILD_TARGET_MAPPING.get(
            parsed_args.build_target, parsed_args.build_target)
    return parsed_args


def _VerifyArgs(parsed_args):
    """Verify args.

    Args:
        parsed_args: Parsed args.

    Raises:
        errors.CommandArgError: If args are invalid.
    """
    if parsed_args.which == CMD_CREATE:
        if (parsed_args.spec and parsed_args.spec not in constants.SPEC_NAMES):
            raise errors.CommandArgError(
                "%s is not valid. Choose from: %s" %
                (parsed_args.spec, ", ".join(constants.SPEC_NAMES)))
        if not ((parsed_args.build_id and parsed_args.build_target) or
                parsed_args.gce_image or parsed_args.local_disk_image):
            raise errors.CommandArgError(
                "At least one of the following should be specified: "
                "--build_id and --build_target, or --gce_image, or "
                "--local_disk_image.")
        if bool(parsed_args.build_id) != bool(parsed_args.build_target):
            raise errors.CommandArgError(
                "Must specify --build_id and --build_target at the same time.")
        if (parsed_args.serial_log_file and
                not parsed_args.serial_log_file.endswith(".tar.gz")):
            raise errors.CommandArgError(
                "--serial_log_file must ends with .tar.gz")
        if (parsed_args.logcat_file and
                not parsed_args.logcat_file.endswith(".tar.gz")):
            raise errors.CommandArgError(
                "--logcat_file must ends with .tar.gz")


def _SetupLogging(log_file, verbose, very_verbose):
    """Setup logging.

    Args:
        log_file: path to log file.
        verbose: If True, log at DEBUG level, otherwise log at INFO level.
        very_verbose: If True, log at DEBUG level and turn on logging on
                      all libraries. Take take precedence over |verbose|.
    """
    if very_verbose:
        logger = logging.getLogger()
    else:
        logger = logging.getLogger(LOGGER_NAME)

    logging_level = logging.DEBUG if verbose or very_verbose else logging.INFO
    logger.setLevel(logging_level)

    if not log_file:
        handler = logging.StreamHandler()
    else:
        handler = logging.FileHandler(filename=log_file)
    log_formatter = logging.Formatter(LOGGING_FMT)
    handler.setFormatter(log_formatter)
    logger.addHandler(handler)


def main(argv):
    """Main entry.

    Args:
        argv: A list of system arguments.

    Returns:
        0 if success. None-zero if fails.
    """
    args = _ParseArgs(argv)
    _SetupLogging(args.log_file, args.verbose, args.very_verbose)
    args = _TranslateAlias(args)
    _VerifyArgs(args)

    config_mgr = config.AcloudConfigManager(args.config_file)
    cfg = config_mgr.Load()
    cfg.OverrideWithArgs(args)

    # Check access.
    device_driver.CheckAccess(cfg)

    if args.which == CMD_CREATE:
        report = device_driver.CreateAndroidVirtualDevices(
            cfg,
            args.build_target,
            args.build_id,
            args.num,
            args.gce_image,
            args.local_disk_image,
            cleanup=not args.no_cleanup,
            serial_log_file=args.serial_log_file,
            logcat_file=args.logcat_file)
    elif args.which == CMD_DELETE:
        report = device_driver.DeleteAndroidVirtualDevices(cfg,
                                                           args.instance_names)
    elif args.which == CMD_CLEANUP:
        report = device_driver.Cleanup(cfg, args.expiration_mins)
    elif args.which == CMD_SSHKEY:
        report = device_driver.AddSshRsa(cfg, args.user, args.ssh_rsa_path)
    else:
        sys.stderr.write("Invalid command %s" % args.which)
        return 2

    report.Dump(args.report_file)
    if report.errors:
        msg = "\n".join(report.errors)
        sys.stderr.write("Encountered the following errors:\n%s\n" % msg)
        return 1
    return 0


if __name__ == "__main__":
    main(sys.argv[1:])
