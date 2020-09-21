#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
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

import json
import logging
import math
import os
from acts import utils
from acts.controllers.utils_lib.ssh import connection
from acts.controllers.utils_lib.ssh import settings

ACTS_CONTROLLER_CONFIG_NAME = "IPerfServer"
ACTS_CONTROLLER_REFERENCE_NAME = "iperf_servers"


def create(configs):
    """ Factory method for iperf servers.

    The function creates iperf servers based on at least one config.
    If configs only specify a port number, a regular local IPerfServer object
    will be created. If configs contains ssh settings for a remote host,
    a RemoteIPerfServer object will be instantiated

    Args:
        config: config parameters for the iperf server
    """
    results = []
    for c in configs:
        try:
            results.append(IPerfServer(c, logging.log_path))
        except:
            pass
    return results


def destroy(objs):
    for ipf in objs:
        try:
            ipf.stop()
        except:
            pass


class IPerfResult(object):
    def __init__(self, result_path):
        """ Loads iperf result from file.

        Loads iperf result from JSON formatted server log. File can be accessed
        before or after server is stopped. Note that only the first JSON object
        will be loaded and this funtion is not intended to be used with files
        containing multiple iperf client runs.
        """
        try:
            with open(result_path, 'r') as f:
                iperf_output = f.readlines()
                if "}\n" in iperf_output:
                    iperf_output = iperf_output[0:
                                                iperf_output.index("}\n") + 1]
                iperf_string = ''.join(iperf_output)
                iperf_string = iperf_string.replace("-nan", '0')
                self.result = json.loads(iperf_string)
        except ValueError:
            with open(result_path, 'r') as f:
                # Possibly a result from interrupted iperf run, skip first line
                # and try again.
                lines = f.readlines()[1:]
                self.result = json.loads(''.join(lines))

    def _has_data(self):
        """Checks if the iperf result has valid throughput data.

        Returns:
            True if the result contains throughput data. False otherwise.
        """
        return ('end' in self.result) and ('sum_received' in self.result["end"]
                                           or 'sum' in self.result["end"])

    def get_json(self):
        """
        Returns:
            The raw json output from iPerf.
        """
        return self.result

    @property
    def error(self):
        if 'error' not in self.result:
            return None
        return self.result['error']

    @property
    def avg_rate(self):
        """Average UDP rate in MB/s over the entire run.

        This is the average UDP rate observed at the terminal the iperf result
        is pulled from. According to iperf3 documentation this is calculated
        based on bytes sent and thus is not a good representation of the
        quality of the link. If the result is not from a success run, this
        property is None.
        """
        if not self._has_data() or 'sum' not in self.result['end']:
            return None
        bps = self.result['end']['sum']['bits_per_second']
        return bps / 8 / 1024 / 1024

    @property
    def avg_receive_rate(self):
        """Average receiving rate in MB/s over the entire run.

        This data may not exist if iperf was interrupted. If the result is not
        from a success run, this property is None.
        """
        if not self._has_data() or 'sum_received' not in self.result['end']:
            return None
        bps = self.result['end']['sum_received']['bits_per_second']
        return bps / 8 / 1024 / 1024

    @property
    def avg_send_rate(self):
        """Average sending rate in MB/s over the entire run.

        This data may not exist if iperf was interrupted. If the result is not
        from a success run, this property is None.
        """
        if not self._has_data() or 'sum_sent' not in self.result['end']:
            return None
        bps = self.result['end']['sum_sent']['bits_per_second']
        return bps / 8 / 1024 / 1024

    @property
    def instantaneous_rates(self):
        """Instantaneous received rate in MB/s over entire run.

        This data may not exist if iperf was interrupted. If the result is not
        from a success run, this property is None.
        """
        if not self._has_data():
            return None
        intervals = [
            interval["sum"]["bits_per_second"] / 8 / 1024 / 1024
            for interval in self.result["intervals"]
        ]
        return intervals

    @property
    def std_deviation(self):
        """Standard deviation of rates in MB/s over entire run.

        This data may not exist if iperf was interrupted. If the result is not
        from a success run, this property is None.
        """
        return self.get_std_deviation(0)

    def get_std_deviation(self, iperf_ignored_interval):
        """Standard deviation of rates in MB/s over entire run.

        This data may not exist if iperf was interrupted. If the result is not
        from a success run, this property is None. A configurable number of
        beginning (and the single last) intervals are ignored in the
        calculation as they are inaccurate (e.g. the last is from a very small
        interval)

        Args:
            iperf_ignored_interval: number of iperf interval to ignored in
            calculating standard deviation
        """
        if not self._has_data():
            return None
        instantaneous_rates = self.instantaneous_rates[iperf_ignored_interval:
                                                       -1]
        avg_rate = math.fsum(instantaneous_rates) / len(instantaneous_rates)
        sqd_deviations = [(rate - avg_rate)**2 for rate in instantaneous_rates]
        std_dev = math.sqrt(
            math.fsum(sqd_deviations) / (len(sqd_deviations) - 1))
        return std_dev


class IPerfServer():
    """Class that handles iperf3 operations.

    Class handles both local and remote iperf servers. Type of server is set
    at runtime based on the configuration input (whether or not ssh settings
    were passed to the constructor).
    """

    def __init__(self, config, log_path):
        if type(config) is dict and "ssh_config" in config:
            self.server_type = "remote"
            self.ssh_settings = settings.from_config(config["ssh_config"])
            self.ssh_session = connection.SshConnection(self.ssh_settings)
            self.port = config["port"]
        else:
            self.server_type = "local"
            self.port = config
        self.log_path = os.path.join(log_path, "iPerf{}".format(self.port))
        utils.create_dir(self.log_path)
        #self.iperf_str = "iperf3 -s -J -p {}".format(self.port)
        self.iperf_str = "iperf3 -s -J"
        self.log_files = []
        self.started = False

    def start(self, extra_args="", tag=""):
        """Starts iperf server on specified machine and port.

        Args:
            extra_args: A string representing extra arguments to start iperf
                server with.
            tag: Appended to log file name to identify logs from different
                iperf runs.
        """
        if self.started:
            return
        if tag:
            tag = tag + ','
        out_file_name = "IPerfServer,{},{}{}.log".format(
            self.port, tag, len(self.log_files))
        self.full_out_path = os.path.join(self.log_path, out_file_name)
        if self.server_type == "local":
            cmd = "{} {} > {}".format(self.iperf_str, extra_args,
                                      self.full_out_path)
            print(cmd)
            self.iperf_process = utils.start_standing_subprocess(cmd)
        else:
            cmd = "{} {} > {}".format(
                self.iperf_str, extra_args,
                "iperf_server_port{}.log".format(self.port))
            job_result = self.ssh_session.run_async(cmd)
            self.iperf_process = job_result.stdout
        self.log_files.append(self.full_out_path)
        self.started = True

    def stop(self):
        """ Stops iperf server running and get output in case of remote server.

        """
        if not self.started:
            return
        if self.server_type == "local":
            utils.stop_standing_subprocess(self.iperf_process)
            self.started = False
        if self.server_type == "remote":
            self.ssh_session.run_async(
                "kill {}".format(str(self.iperf_process)))
            iperf_result = self.ssh_session.run(
                "cat iperf_server_port{}.log".format(self.port))
            with open(self.full_out_path, 'w') as f:
                f.write(iperf_result.stdout)
            self.ssh_session.run_async(
                "rm iperf_server_port{}.log".format(self.port))
            self.started = False
