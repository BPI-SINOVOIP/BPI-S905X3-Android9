#
# Copyright (C) 2016 The Android Open Source Project
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

import os

from vts.utils.python.os import path_utils

from vts.testcases.kernel.ltp import ltp_enums

VTS_LTP_OUTPUT = os.path.join('DATA', 'nativetest', 'ltp')
LTP_RUNTEST_DIR = os.path.join(VTS_LTP_OUTPUT, 'runtest')
LTP_DISABLED_BUILD_TESTS_CONFIG_PATH = os.path.join(VTS_LTP_OUTPUT,
                                                    'disabled_tests.txt')

# Environment paths for ltp test cases
# string, ltp build root directory on target
LTPDIR = '/data/local/tmp/ltp'
# Directory for environment variable 'TMP' needed by some test cases
TMP = path_utils.JoinTargetPath(LTPDIR, 'tmp')
# Directory for environment variable 'TMPBASE' needed by some test cases
TMPBASE = path_utils.JoinTargetPath(TMP, 'tmpbase')
# Directory for environment variable 'LTPTMP' needed by some test cases
LTPTMP = path_utils.JoinTargetPath(TMP, 'ltptemp')
# Directory for environment variable 'TMPDIR' needed by some test cases
TMPDIR = path_utils.JoinTargetPath(TMP, 'tmpdir')
# Path where ltp test binary exists
LTPBINPATH = path_utils.JoinTargetPath(LTPDIR, 'testcases', 'bin')
# Add LTP's binary path to PATH
PATH = '/system/bin:%s' % LTPBINPATH

# Default number of threads to run LTP tests. Zero means matching to number
# of CPU threads
DEFAULT_NUMBER_OF_THREADS = 0

# File system type for loop device
LTP_DEV_FS_TYPE = 'ext4'

# File name suffix for low memory scenario group scripts
LOW_MEMORY_SCENARIO_GROUP_SUFFIX = '_low_mem'

# Binaries required by LTP test cases that should exist in PATH
INTERNAL_BINS = [
    'mktemp',
    'cp',
    'chmod',
    'chown',
    'ls',
    'mkfifo',
    'ldd',
]

# Internal shell command required by some LTP test cases
INTERNAL_SHELL_COMMANDS = [
    'export',
    'cd',
]

# Requirement to testcase dictionary.
REQUIREMENTS_TO_TESTCASE = {
    ltp_enums.Requirements.LOOP_DEVICE_SUPPORT: [
        'syscalls.mount01',
        'syscalls.fchmod06',
        'syscalls.ftruncate04',
        'syscalls.ftruncate04_64',
        'syscalls.inotify03',
        'syscalls.link08',
        'syscalls.linkat02',
        'syscalls.mkdir03',
        'syscalls.mkdirat02',
        'syscalls.mknod07',
        'syscalls.mknodat02',
        'syscalls.mmap16',
        'syscalls.mount01',
        'syscalls.mount02',
        'syscalls.mount03',
        'syscalls.mount04',
        'syscalls.mount06',
        'syscalls.rename11',
        'syscalls.renameat01',
        'syscalls.rmdir02',
        'syscalls.umount01',
        'syscalls.umount02',
        'syscalls.umount03',
        'syscalls.umount2_01',
        'syscalls.umount2_02',
        'syscalls.umount2_03',
        'syscalls.utime06',
        'syscalls.utimes01',
        'syscalls.mkfs01',
        'fs.quota_remount_test01',
    ],
    ltp_enums.Requirements.BIN_IN_PATH_LDD: ['commands.ldd'],
}

# Requirement for all test cases
REQUIREMENT_FOR_ALL = [ltp_enums.Requirements.LTP_TMP_DIR]

# Requirement to test suite dictionary
REQUIREMENT_TO_TESTSUITE = {}

# List of LTP test suites to run
TEST_SUITES = [
    'admin_tools',
    'can',
    'cap_bounds',
    'commands',
    'connectors',
    'containers',
#     'controllers',
    'cpuhotplug',
    'dio',
    'fcntl-locktests_android',
    'filecaps',
    'fs',
    'fs_bind',
    'fs_ext4',
    'fs_perms_simple',
    'fsx',
    'hugetlb',
    'hyperthreading',
    'input',
    'io',
    'ipc',
    'kernel_misc',
    'math',
    'mm',
    'modules',
    'nptl',
    'numa',
    'pipes',
    'power_management_tests',
    'pty',
    'sched',
    'syscalls',
    'timers',
    # The following are not included in default LTP scenario group
    'securebits',
    'tracing',
]

# List of LTP test suites to run
TEST_SUITES_LOW_MEM = [
    'admin_tools',
    'can',
    'cap_bounds',
    'commands',
    'connectors',
    'containers',
#     'controllers',
    'cpuhotplug',
    'dio',
    'fcntl-locktests_android',
    'filecaps',
    'fs',
    'fs_bind',
    'fs_ext4',
    'fs_perms_simple',
    'fsx',
    'hugetlb',
    'hyperthreading',
    'input',
    'io',
    'ipc',
    'kernel_misc',
    'math',
    'mm',
    'modules',
    'nptl',
    'numa',
    'pipes',
    'power_management_tests',
    'pty',
    'sched_low_mem',
    'syscalls',
    'timers',
    # The following are not included in default LTP scenario group
    'securebits',
    'tracing',
]

# List of LTP test suites that will not run in multi-thread mode
TEST_SUITES_REQUIRE_SINGLE_THREAD_MODE = [
    'dio',
    'io',
    'mm',
    'timers',
]
