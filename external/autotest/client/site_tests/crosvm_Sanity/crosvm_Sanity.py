# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error


class crosvm_Sanity(test.test):
    """Set baseline expectations for hosting Chrome OS VM images.

    In the Chrome OS lab these are based on current Google Compute Engine
    (GCE) server offerings and represent the lowest common denominator to
    effectively run QEMU/kvm. As a result VMs can be thought of as a
    slightly outdated Intel Chromebox. GCE is not the only host
    environment, and it runs in a VM as well. We attempt to control this
    environment as well as possible. This test ensures relevant changes
    are detected over time
    """
    version = 1

    def initialize(self):
        """Initialize baseline parameters."""

        # We expect to use 8 Haswell CPU cores.
        self.cpu_cores = 8
        # The current GCE offering is a stripped Haswell. This is similar to
        # Z840. Matching CPU arch and flags are requested by
        # crosutils/lib/cros_vm_lib.sh.
        # TODO(pwang): Replace with 'Haswell, no TSX' once lab is ready.
        self.cpu_arch = 'Sandy Bridge'

        # These are flags that sampled from GCE builders on cros lab.
        self.cpu_flags = [
            'abm', 'aes', 'apic', 'arat', 'avx', 'avx2', 'bmi1', 'bmi2',
            'clflush', 'cmov', 'constant_tsc', 'cx16', 'cx8', 'de', 'eagerfpu',
            'erms', 'f16c', 'fma', 'fpu', 'fsgsbase', 'fxsr', 'hypervisor',
            'kaiser', 'lahf_lm', 'lm', 'mca', 'mce', 'mmx', 'movbe', 'msr',
            'mtrr', 'nopl', 'nx', 'pae', 'pat', 'pcid', 'pclmulqdq', 'pge',
            'pni', 'popcnt', 'pse', 'pse36', 'rdrand', 'rdtscp', 'rep_good',
            'sep', 'smep', 'sse', 'sse2', 'sse4_1', 'sse4_2', 'ssse3',
            'syscall', 'tsc', 'vme', 'x2apic', 'xsave', 'xsaveopt'
        ]
        self.min_memory_kb = 7.5 * 1024 * 1024

    def cleanup(self):
        """Test cleanup."""

    def run_once(self):
        """Run the test."""
        errors = ''
        warnings = ''
        funcs = [self.test_cpu, self.test_gpu, self.test_mem]
        for func in funcs:
            error_msg, warning_msg = func()
            errors += error_msg
            warnings += warning_msg

        if errors:
            raise error.TestFail('Failed: %s' % (errors + warnings))
        if warnings:
            raise error.TestWarn('Warning: %s' % warnings)

    def test_cpu(self):
        """Test the CPU configuration."""
        errors = ''
        warning = ''
        if self.cpu_cores != utils.count_cpus():
            errors += 'Expecting %d CPU cores but found %d cores\n' % (
                self.cpu_cores, utils.count_cpus())

        for cpu_info in utils.get_cpuinfo():
            if self.cpu_arch not in cpu_info['model name']:
                errors += 'Expecting %s CPU but found %s' % (
                    self.cpu_arch, cpu_info['model name'])

            flags = sorted(cpu_info['flags'].split(' '))
            if flags != self.cpu_flags:
                # TODO(pwang): convert warning to error once VM got better
                # infra support.
                warning += 'Expecting CPU flags %s but found %s\n' % (
                    self.cpu_flags, flags)
        return errors, warning

    def test_gpu(self):
        """Test the GPU configuration."""

        # TODO(pwang): Add check once virgl is fully introduced to VM.
        errors = ''
        warning = ''
        return errors, warning

    def test_mem(self):
        """Test the RAM configuration."""
        errors = ''
        warning = ''
        if self.min_memory_kb > utils.memtotal():
            errors += 'Expecting at least %dKB memory but found %sKB\n' % (
                self.min_memory_kb, utils.memtotal())
        return errors, warning
