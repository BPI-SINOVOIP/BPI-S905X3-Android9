#
# Copyright (C) 2016 The Android Open Source Project
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


class RequirementState(object):
    """Enum for test case requirement check state.

    Attributes:
        UNCHECKED: test case requirement has not been checked
        SATISFIED: all the requirements are satisfied
        UNSATISFIED: some of the requirements are not satisfied. Test case will
                     not be executed
    """
    UNCHECKED = 0
    SATISFIED = 2
    UNSATISFIED = 3


class ConfigKeys(object):
    RUN_STAGING = "run_staging"
    RUN_32BIT = "run_32bit"
    RUN_64BIT = "run_64bit"
    LTP_NUMBER_OF_THREADS = "ltp_number_of_threads"


class ShellEnvKeys(object):
    """Shell env keys to run LTP (Linux Test Project) testcases.

    Contains constant strings starting with prefix "_KEY_ENV_" are used as dict
        key in environment variable dictionary
    """
    TMP = 'TMP'
    TMPBASE = 'TMPBASE'
    LTPTMP = 'LTPTMP'
    TMPDIR = 'TMPDIR'
    LTP_DEV_FS_TYPE = 'LTP_DEV_FS_TYPE'
    LTPROOT = 'LTPROOT'
    PATH = 'PATH'


class Delimiters(object):
    TESTCASE_FILTER = ','
    TESTCASE_DEFINITION = '\t'


class TestExitCode(object):
    """Exit return codes of LTP test case binary

    Attributes:
        TPASS: int, exit_code for Test pass
        TCONF: int, the test case is not for current configuration of kernel
        TBROK: int, test assumption failed, such as missing system binary and
               and permission issue not explicitly checked by the test case.
        SEGFAULT: int, test case results in segmentation fault
    """
    TPASS = 0
    TCONF = 32
    TBROK = 2
    SEGFAULT = 139


class Requirements(object):
    """Enum for all ltp requirements"""
    LOOP_DEVICE_SUPPORT = 1
    LTP_TMP_DIR = 2
    BIN_IN_PATH_LDD = 3
