#!/usr/bin/env python3.4
#
#   Copyright 2017 - The Android Open Source Project
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
"""
Starts iperf client on host machine.
"""
from acts import utils
import os
import subprocess


class IPerfClient():
    """Class that handles iperf3 client operations."""

    def __init__(self, port, ip_address, log_path):
        self.port = port
        self.log_path = os.path.join(log_path, "iPerf{}".format(self.port))
        self.iperf_cmd = "iperf3 -c {} -i1".format(ip_address)
        self.iperf_process = None
        self.log_files = []
        self.started = False

    def start(self, extra_args="", tag=""):
        """Starts iperf client on specified port on host machine.

        Args:
            extra_args: A string representing extra arguments to start iperf
            client with.
            tag: Appended to log file name to identify logs from different
            iperf runs.

        Returns:
            full_out_path: Iperf result path.
        """
        if self.started:
            return
        utils.create_dir(self.log_path)
        if tag:
            tag = tag + ','
        out_file_name = "IPerfClient,{},{}{}.log".format(
            self.port, tag, len(self.log_files))
        full_out_path = os.path.join(self.log_path, out_file_name)
        cmd = "{} {}".format(self.iperf_cmd, extra_args)
        cmd = cmd.split()
        with open(full_out_path, "w") as f:
            subprocess.call(cmd, stdout=f)
        self.log_files.append(full_out_path)
        self.started = True
        return full_out_path
