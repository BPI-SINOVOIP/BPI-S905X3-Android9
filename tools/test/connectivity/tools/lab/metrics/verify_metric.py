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

from metrics.metric import Metric


class VerifyMetric(Metric):
    """Gathers the information of connected devices via ADB"""
    COMMAND = r"adb devices | sed '1d;$d'"
    UNAUTHORIZED = 'unauthorized'
    OFFLINE = 'offline'
    RECOVERY = 'recovery'
    QUESTION = 'question'
    DEVICE = 'device'
    TOTAL_UNHEALTHY = 'total_unhealthy'

    def gather_metric(self):
        """ Gathers device info based on adb output.

        Returns:
            A dictionary with the fields:
            unauthorized: list of phone sn's that are unauthorized
            offline: list of phone sn's that are offline
            recovery: list of phone sn's that are in recovery mode
            question: list of phone sn's in ??? mode
            device: list of phone sn's that are in device mode
            total: total number of offline, recovery, question or unauthorized
                devices
        """
        offline_list = list()
        unauth_list = list()
        recovery_list = list()
        question_list = list()
        device_list = list()

        # Delete first and last line of output of adb.
        output = self._shell.run(self.COMMAND).stdout

        # Example Line, Device Serial Num TAB Phone Status
        # 00bd977c7f504caf	offline
        if output:
            for line in output.split('\n'):
                spl_line = line.split('\t')
                # spl_line[0] is serial, [1] is status. See example line.
                phone_sn = spl_line[0]
                phone_state = spl_line[1]

                if phone_state == 'device':
                    device_list.append(phone_sn)
                elif phone_state == 'unauthorized':
                    unauth_list.append(phone_sn)
                elif phone_state == 'recovery':
                    recovery_list.append(phone_sn)
                elif '?' in phone_state:
                    question_list.append(phone_sn)
                elif phone_state == 'offline':
                    offline_list.append(phone_sn)

        return {
            self.UNAUTHORIZED:
            unauth_list,
            self.OFFLINE:
            offline_list,
            self.RECOVERY:
            recovery_list,
            self.QUESTION:
            question_list,
            self.DEVICE:
            device_list,
            self.TOTAL_UNHEALTHY:
            len(unauth_list) + len(offline_list) + len(recovery_list) +
            len(question_list)
        }
