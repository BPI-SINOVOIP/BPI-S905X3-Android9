#!/usr/bin/env python
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

import itertools
from metrics.metric import Metric


class ProcessTimeMetric(Metric):
    TIME_COMMAND = 'ps -p %s -o etimes,command | sed 1d'
    # Number of seconds in 24 hours
    MIN_TIME = 86400
    # Fields for response dictionary
    ADB_PROCESSES = 'adb_processes'
    NUM_ADB_PROCESSES = 'num_adb_processes'
    FASTBOOT_PROCESSES = 'fastboot_processes'
    NUM_FASTBOOT_PROCESSES = 'num_fastboot_processes'

    def gather_metric(self):
        """Returns ADB and Fastboot processes and their time elapsed

        Returns:
            A dictionary with adb/fastboot_processes as a list of serial nums or
            NONE if number wasn't in command. num_adb/fastboot_processes as
            the number of serials in list.
        """
        # Get the process ids
        pids = self.get_adb_fastboot_pids()

        # Get elapsed time for selected pids
        adb_processes, fastboot_processes = [], []
        for pid in pids:
            # Sample output:
            # 232893 fastboot -s FA6BM0305019 -w

            output = self._shell.run(self.TIME_COMMAND % pid).stdout
            spl_ln = output.split()

            # There is a potential race condition between getting pids, and the
            # pid then dying, so we must check that there is output.
            if spl_ln:
                # pull out time in seconds
                time = int(spl_ln[0])
            else:
                continue

            # We only care about processes older than the min time
            if time > self.MIN_TIME:
                # ignore fork-server command, because it's not a problematic process
                if 'fork-server' not in output:
                    # get serial number, which defaults to none
                    serial_number = None
                    if '-s' in spl_ln:
                        sn_index = spl_ln.index('-s')
                        # avoid indexing out of range
                        if sn_index + 1 < len(spl_ln):
                            serial_number = spl_ln[sn_index + 1]
                    # append to proper list
                    if 'fastboot' in output:
                        fastboot_processes.append(serial_number)
                    elif 'adb' in output:
                        adb_processes.append(serial_number)

        # Create response dictionary
        response = {
            self.ADB_PROCESSES: adb_processes,
            self.NUM_ADB_PROCESSES: len(adb_processes),
            self.FASTBOOT_PROCESSES: fastboot_processes,
            self.NUM_FASTBOOT_PROCESSES: len(fastboot_processes)
        }
        return response

    def get_adb_fastboot_pids(self):
        """Finds a list of ADB and Fastboot process ids.

        Returns:
          A list of PID strings
        """
        # Get ids of processes with 'adb' or 'fastboot' in name
        adb_result = self._shell.get_command_pids('adb')
        fastboot_result = self._shell.get_command_pids('fastboot')
        # concatenate two generator objects, return as list
        return list(itertools.chain(adb_result, fastboot_result))
