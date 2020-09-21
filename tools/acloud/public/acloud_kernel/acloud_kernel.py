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

"""Acloud Kernel Utility.

This CLI implements additional functionality to acloud CLI.
"""
import argparse
import os
import sys

from acloud.public import acloud_common
from acloud.public import config
from acloud.public.acloud_kernel import kernel_swapper

DEFAULT_CONFIG_FILE = "acloud.config"

# Commands
CMD_SWAP_KERNEL = "swap_kernel"


def _ParseArgs(args):
    """Parse args.

    Args:
        args: argument list passed from main.

    Returns:
        Parsed args.
    """
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()

    swap_kernel_parser = subparsers.add_parser(CMD_SWAP_KERNEL)
    swap_kernel_parser.required = False
    swap_kernel_parser.set_defaults(which=CMD_SWAP_KERNEL)
    swap_kernel_parser.add_argument(
        "--instance_name",
        type=str,
        dest="instance_name",
        required=True,
        help="The names of the instances that will have their kernels swapped, "
        "separated by spaces, e.g. --instance_names instance-1 instance-2")
    swap_kernel_parser.add_argument(
        "--local_kernel_image",
        type=str,
        dest="local_kernel_image",
        required=True,
        help="Path to a local disk image to use, e.g /tmp/bzImage")
    acloud_common.AddCommonArguments(swap_kernel_parser)

    return parser.parse_args(args)


def main(argv):
    """Main entry.

    Args:
        argv: list of system arguments.

    Returns:
        0 if success. Non-zero otherwise.
    """
    args = _ParseArgs(argv)
    config_mgr = config.AcloudConfigManager(args.config_file)
    cfg = config_mgr.Load()
    cfg.OverrideWithArgs(args)

    k_swapper = kernel_swapper.KernelSwapper(cfg, args.instance_name)
    report = k_swapper.SwapKernel(args.local_kernel_image)

    report.Dump(args.report_file)
    if report.errors:
        msg = "\n".join(report.errors)
        sys.stderr.write("Encountered the following errors:\n%s\n" % msg)
        return 1
    return 0
