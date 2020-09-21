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

"""Common code used by both acloud and acloud_kernel tools."""

DEFAULT_CONFIG_FILE = "acloud.config"


def AddCommonArguments(parser):
    """Adds arguments common to parsers.

    Args:
        parser: ArgumentParser object, used to parse flags.
    """
    parser.add_argument("--email",
                        type=str,
                        dest="email",
                        help="Email account to use for authentcation.")
    parser.add_argument(
        "--config_file",
        type=str,
        dest="config_file",
        default=DEFAULT_CONFIG_FILE,
        help="Path to the config file, default to acloud.config"
        "in the current working directory")
    parser.add_argument("--report_file",
                        type=str,
                        dest="report_file",
                        default=None,
                        help="Dump the report this file in json format. "
                        "If not specified, just log the report")
    parser.add_argument("--log_file",
                        dest="log_file",
                        type=str,
                        default=None,
                        help="Path to log file.")
    parser.add_argument("-v",
                        dest="verbose",
                        action="store_true",
                        default=False,
                        help="Verbose mode")
    parser.add_argument("-vv",
                        dest="very_verbose",
                        action="store_true",
                        default=False,
                        help="Very verbose mode")
