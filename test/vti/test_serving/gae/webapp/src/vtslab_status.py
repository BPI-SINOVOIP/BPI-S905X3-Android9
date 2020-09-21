# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Status dict updated from HC.
DEVICE_STATUS_DICT = {
    # default state, currently not in use.
    "unknown": 0,
    # for devices detected via "fastboot devices" shell command.
    "fastboot": 1,
    # for devices detected via "adb devices" shell command.
    "online": 2,
    # currently not in use.
    "ready": 3,
    # currently not in use.
    "use": 4,
    # for devices in error state.
    "error": 5,
    # for devices which timed out (not detected either via fastboot or adb).
    "no-response": 6
}

# Scheduling status dict based on the status of each jobs in job queue.
DEVICE_SCHEDULING_STATUS_DICT = {
    # for devices detected but not scheduled.
    "free": 0,
    # for devices scheduled but not running.
    "reserved": 1,
    # for devices scheduled for currently leased job(s).
    "use": 2
}

# Job status dict
JOB_STATUS_DICT = {
    # scheduled but not leased yet
    "ready": 0,
    # scheduled and in running
    "leased": 1,
    # completed job
    "complete": 2,
    # unexpected error during running
    "infra-err": 3,
    # never leased within schedule period
    "expired": 4
}

JOB_PRIORITY_DICT = {
    "top": 0,
    "high": 1,
    "medium": 2,
    "low": 3,
    "other": 4
}


STORAGE_TYPE_DICT = {
    "unknown": 0,
    "PAB": 1,
    "GCS": 2
}


def PrioritySortHelper(priority):
    """Helper function to sort jobs based on priority.

    Args:
        priority: string, the job priority.

    Returns:
        int, priority order (the lower, the higher)
    """
    priority = priority.lower()
    if priority in JOB_PRIORITY_DICT:
        return JOB_PRIORITY_DICT[priority]
    return 4
