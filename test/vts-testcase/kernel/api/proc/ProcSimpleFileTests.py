#
# Copyright (C) 2017 The Android Open Source Project
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

import math

from parse import with_pattern
from vts.testcases.kernel.api.proc import KernelProcFileTestBase
from vts.utils.python.file import target_file_utils

# Test for /proc/sys/kernel/*.

class ProcCorePattern(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/core_pattern is used to specify a core dumpfile pattern
    name.
    '''

    def parse_contents(self, contents):
        pass

    def get_path(self):
        return "/proc/sys/kernel/core_pattern"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcCorePipeLimit(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/core_pipe_limit defines how many concurrent crashing
    processes may be piped to user space applications in parallel.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def get_path(self):
        return "/proc/sys/kernel/core_pipe_limit"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcDmesgRestrict(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/dmesg_restrict indicates whether unprivileged users are
    prevented from using dmesg.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result in [0, 1]

    def get_path(self):
        return "/proc/sys/kernel/dmesg_restrict"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcDomainname(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/domainname determines YP/NIS domain name of the system.'''

    def parse_contents(self, contents):
        pass

    def get_path(self):
        return "/proc/sys/kernel/domainname"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcHostname(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/hostname determines the system's host name.'''

    def parse_contents(self, contents):
        pass

    def get_path(self):
        return "/proc/sys/kernel/hostname"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcHungTaskTimeoutSecs(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/hung_task_timeout_secs controls the default timeout
    (in seconds) used to determine when a task has become non-responsive and
    should be considered hung.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def get_path(self):
        return "/proc/sys/kernel/hung_task_timeout_secs"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite

    def file_optional(self):
        return True

class ProcKptrRestrictTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/kptr_restrict determines whether kernel pointers are printed
    in proc files.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result >= 0 and result <= 4

    def get_path(self):
        return "/proc/sys/kernel/kptr_restrict"

    def get_permission_checker(self):
        """Get r/w file permission checker.
        """
        return target_file_utils.IsReadWrite


class ProcModulesDisabled(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/modules_disabled indicates if modules are allowed to be
    loaded.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result in [0, 1]

    def get_path(self):
        return "/proc/sys/kernel/modules_disabled"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcPanicOnOops(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/panic_on_oops controls kernel's behaviour on oops.'''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result in [0, 1]

    def get_path(self):
        return "/proc/sys/kernel/panic_on_oops"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcPerfEventMaxSampleRate(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/perf_event_max_sample_rate sets the maximum sample rate
    of performance events.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def get_path(self):
        return "/proc/sys/kernel/perf_event_max_sample_rate"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcPerfEventParanoid(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/perf_event_paranoid controls use of the performance
    events system by unprivileged users.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def get_path(self):
        return "/proc/sys/kernel/perf_event_paranoid"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcPidMax(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/pid_max is the pid allocation wrap value.'''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def get_path(self):
        return "/proc/sys/kernel/pid_max"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


@with_pattern(
    r"^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$"
)
def token_uuid(text):
    return text

class ProcSysKernelRandomBootId(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/random/boot_id generates a random ID each boot.'''

    def parse_contents(self, contents):
        return self.parse_line("{:uuid}", contents, dict(uuid=token_uuid))[0]

    def get_path(self):
        return "/proc/sys/kernel/random/boot_id"

    def get_permission_checker(self):
        return target_file_utils.IsReadOnly


class ProcRandomizeVaSpaceTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/randomize_va_space determines the address layout randomization
    policy for the system.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result >= 0 and result <= 2

    def get_path(self):
        return "/proc/sys/kernel/randomize_va_space"

    def get_permission_checker(self):
        """Get r/w file permission checker.
        """
        return target_file_utils.IsReadWrite


class ProcSchedChildRunsFirst(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/sched_child_runs_first causes newly forked tasks to
    be favored in scheduling over their parents.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def get_path(self):
        return "/proc/sys/kernel/sched_child_runs_first"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcSchedLatencyNS(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/sched_latency_ns is the maximum latency in nanoseconds a
    task may incur prior to being scheduled.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result >= 100000 and result <= 1000000000

    def get_path(self):
        return "/proc/sys/kernel/sched_latency_ns"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcSchedRTPeriodUS(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/sched_rt_period_us defines the period length used by the
    system-wide RT execution limit in microseconds.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result >= 1 and result <= math.pow(2,31)

    def get_path(self):
        return "/proc/sys/kernel/sched_rt_period_us"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcSchedRTRuntimeUS(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/sched_rt_runtime_us defines the amount of time in
    microseconds relative to sched_rt_period_us that the system may execute RT
    tasks.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result >= -1 and result <= (math.pow(2,31) - 1)

    def get_path(self):
        return "/proc/sys/kernel/sched_rt_runtime_us"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcSchedTunableScaling(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/sched_tunable_scaling determines whether
    sched_latency_ns should be automatically adjusted by the scheduler based on
    the number of CPUs.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result >= 0 and result <= 2

    def get_path(self):
        return "/proc/sys/kernel/sched_tunable_scaling"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcSchedWakeupGranularityNS(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/sched_wakeup_granularity_ns defines how much more
    virtual runtime task A must have than task B in nanoseconds in order for
    task B to preempt it.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result >= 0 and result <= 1000000000

    def get_path(self):
        return "/proc/sys/kernel/sched_wakeup_granularity_ns"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcSysRqTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/kernel/sysrq controls the functions allowed to be invoked
    via the SysRq key.'''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result >= 0 and result <= 511

    def get_path(self):
        return "/proc/sys/kernel/sysrq"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


# Tests for /proc/sys/vm/*.

class ProcDirtyBackgroundBytes(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/vm/dirty_background_bytes contains the amount of dirty memory
    at which the background kernel flusher threads will start writeback.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def get_path(self):
        return "/proc/sys/vm/dirty_background_bytes"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcDirtyBackgroundRatio(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/vm/dirty_background_ratio contains, as a percentage of total
    available memory that contains free pages and reclaimable pages, the number
    of pages at which the background kernel flusher threads will start writing
    out dirty data.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result >= 0 and result <= 100

    def get_path(self):
        return "/proc/sys/vm/dirty_background_ratio"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcDropCaches(KernelProcFileTestBase.KernelProcFileTestBase):
    '''Writing to /proc/sys/vm/drop_caches will cause the kernel to drop clean
    caches.
    '''

    def parse_contents(self, contents):
        # Format of this file is not documented, so don't check that.
        return ''

    def get_path(self):
        return "/proc/sys/vm/drop_caches"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcExtraFreeKbytes(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/vm/extra_free_kbytes tells the VM to keep extra free memory
    between the threshold where background reclaim (kswapd) kicks in, and the
    threshold where direct reclaim (by allocating processes) kicks in.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def get_path(self):
        return "/proc/sys/vm/extra_free_kbytes"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite

    def file_optional(self):
        # This file isn't in Android common kernel.
        return True


class ProcOverCommitMemoryTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/vm/overcommit_memory determines the kernel virtual memory accounting mode.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result >= 0 and result <= 2

    def get_path(self):
        return "/proc/sys/vm/overcommit_memory"

    def get_permission_checker(self):
        """Get r/w file permission checker.
        """
        return target_file_utils.IsReadWrite


class ProcMaxMapCount(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/vm/max_map_count contains the maximum number of memory map areas a process
    may have.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def get_path(self):
        return "/proc/sys/vm/max_map_count"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcMmapMinAddrTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/vm/mmap_min_addr specifies the minimum address that can be mmap'd.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def get_path(self):
        return "/proc/sys/vm/mmap_min_addr"

    def get_permission_checker(self):
        """Get r/w file permission checker.
        """
        return target_file_utils.IsReadWrite


class ProcMmapRndBitsTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/vm/mmap_rnd_(compat_)bits specifies the amount of randomness in mmap'd
    addresses. Must be >= 8.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result >= 8

    def get_path(self):
        return "/proc/sys/vm/mmap_rnd_bits"

    def get_permission_checker(self):
        """Get r/w file permission checker.
        """
        return target_file_utils.IsReadWrite


class ProcMmapRndCompatBitsTest(ProcMmapRndBitsTest):
    def get_path(self):
        return "/proc/sys/vm/mmap_rnd_compat_bits"


class ProcPageCluster(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/vm/page-cluster controls the number of pages up to which
    consecutive pages are read in from swap in a single attempt.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def get_path(self):
        return "/proc/sys/vm/page-cluster"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


# Tests for /proc/sys/fs/*.

class ProcPipeMaxSize(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/fs/pipe-max-size reports the maximum size (in bytes) of
    individual pipes.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def get_path(self):
        return "/proc/sys/fs/pipe-max-size"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcProtectedHardlinks(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/fs/protected_hardlinks reports hardlink creation behavior.'''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result in [0, 1]

    def get_path(self):
        return "/proc/sys/fs/protected_hardlinks"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcProtectedSymlinks(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/fs/protected_symlinks reports symlink following behavior.'''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result in [0, 1]

    def get_path(self):
        return "/proc/sys/fs/protected_symlinks"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcSuidDumpable(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/sys/fs/suid_dumpable value can be used to query and set the core
    dump mode for setuid or otherwise protected/tainted binaries.
    '''

    def parse_contents(self, contents):
        return self.parse_line("{:d}\n", contents)[0]

    def result_correct(self, result):
        return result in [0, 1, 2]

    def get_path(self):
        return "/proc/sys/fs/suid_dumpable"

    def get_permission_checker(self):
        return target_file_utils.IsReadWrite


class ProcUptime(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/uptime tells how long the system has been running.'''

    def parse_contents(self, contents):
        return self.parse_line("{:f} {:f}\n", contents)[0]

    def get_path(self):
        return "/proc/uptime"
