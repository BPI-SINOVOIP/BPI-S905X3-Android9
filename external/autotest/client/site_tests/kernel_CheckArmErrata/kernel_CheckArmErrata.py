# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

from autotest_lib.client.cros.power import sys_power


class kernel_CheckArmErrata(test.test):
    """
    Test for the presence of ARM errata fixes.

    See control file for more details.
    """
    version = 1
    SECS_TO_SUSPEND = 15

    @staticmethod
    def _parse_cpu_info(cpuinfo_str):
        """
        Parse the contents of /proc/cpuinfo

        This will return a dict of dicts with info about all the CPUs.

        :param cpuinfo_str: The contents of /proc/cpuinfo as a string.
        :return: A dictionary of dictionaries; top key is processor ID and
                 secondary key is info from cpuinfo about that processor.

        >>> cpuinfo = kernel_CheckArmErrata._parse_cpu_info(
        ... '''processor       : 0
        ... model name      : ARMv7 Processor rev 1 (v7l)
        ... Features        : swp half thumb fastmult vfp edsp thumbee neon ...
        ... CPU implementer : 0x41
        ... CPU architecture: 7
        ... CPU variant     : 0x0
        ... CPU part        : 0xc0d
        ... CPU revision    : 1
        ...
        ... processor       : 1
        ... model name      : ARMv7 Processor rev 1 (v7l)
        ... Features        : swp half thumb fastmult vfp edsp thumbee neon ...
        ... CPU implementer : 0x41
        ... CPU architecture: 7
        ... CPU variant     : 0x0
        ... CPU part        : 0xc0d
        ... CPU revision    : 1
        ...
        ... Hardware        : Rockchip (Device Tree)
        ... Revision        : 0000
        ... Serial          : 0000000000000000''')
        >>> cpuinfo == {
        ... 0: {"CPU architecture": 7,
        ...     "CPU implementer": 65,
        ...     "CPU part": 3085,
        ...     "CPU revision": 1,
        ...     "CPU variant": 0,
        ...     "Features": "swp half thumb fastmult vfp edsp thumbee neon ...",
        ...     "model name": "ARMv7 Processor rev 1 (v7l)",
        ...     "processor": 0},
        ... 1: {"CPU architecture": 7,
        ...     "CPU implementer": 65,
        ...     "CPU part": 3085,
        ...     "CPU revision": 1,
        ...     "CPU variant": 0,
        ...     "Features": "swp half thumb fastmult vfp edsp thumbee neon ...",
        ...     "model name": "ARMv7 Processor rev 1 (v7l)",
        ...     "processor": 1}
        ... }
        True
        """
        cpuinfo = {}
        processor = None
        for info_line in cpuinfo_str.splitlines():
            # Processors are separated by blank lines
            if not info_line:
                processor = None
                continue

            key, _, val = info_line.partition(':')
            key = key.strip()
            val = val.strip()

            # Try to convert to int...
            try:
                val = int(val, 0)
            except ValueError:
                pass

            # Handle processor special.
            if key == "processor":
                processor = int(val)
                cpuinfo[processor] = { "processor": processor }
                continue

            # Skip over any info we have no processor for
            if processor is None:
                continue

            cpuinfo[processor][key] = val

        return cpuinfo

    @staticmethod
    def _get_regid_to_val(cpu_id):
        """
        Parse the contents of /sys/kernel/debug/arm_coprocessor_debug

        This will read /sys/kernel/debug/arm_coprocessor_debug and create
        a dictionary mapping register IDs, which look like:
          "(p15, 0, c15, c0, 1)"
        ...to values (as integers).

        :return: A dictionary mapping string register IDs to int value.
        """
        try:
            arm_debug_info = utils.run(
                "taskset -c %d cat /sys/kernel/debug/arm_coprocessor_debug" %
                cpu_id).stdout.strip()
        except error.CmdError:
            arm_debug_info = ""

        regid_to_val = {}
        for line in arm_debug_info.splitlines():
            # Each line is supposed to show the CPU number; confirm it's right
            if not line.startswith("CPU %d: " % cpu_id):
                raise error.TestError(
                    "arm_coprocessor_debug error: CPU %d != %s" %
                    (cpu_id, line.split(":")[0]))

            _, _, regid, val = line.split(":")
            regid_to_val[regid.strip()] = int(val, 0)

        return regid_to_val

    def _check_one_cortex_a12(self, cpuinfo):
        """
        Check the errata for a Cortex-A12.

        :param cpuinfo: The CPU info for one CPU.  See _parse_cpu_info for
                        the format.

        >>> _testobj._get_regid_to_val = lambda cpu_id: {}
        >>> try:
        ...     _testobj._check_one_cortex_a12({
        ...         "processor": 2,
        ...         "model name": "ARMv7 Processor rev 1 (v7l)",
        ...         "CPU implementer": ord("A"),
        ...         "CPU part": 0xc0d,
        ...         "CPU variant": 0,
        ...         "CPU revision": 1})
        ... except Exception:
        ...     import traceback
        ...     print traceback.format_exc(),
        Traceback (most recent call last):
        ...
        TestError: Kernel didn't provide register vals

        >>> _testobj._get_regid_to_val = lambda cpu_id: \
               {"(p15, 0, c15, c0, 1)": 0, "(p15, 0, c15, c0, 2)": 0}
        >>> try:
        ...     _testobj._check_one_cortex_a12({
        ...         "processor": 2,
        ...         "model name": "ARMv7 Processor rev 1 (v7l)",
        ...         "CPU implementer": ord("A"),
        ...         "CPU part": 0xc0d,
        ...         "CPU variant": 0,
        ...         "CPU revision": 1})
        ... except Exception:
        ...     import traceback
        ...     print traceback.format_exc(),
        Traceback (most recent call last):
        ...
        TestError: Missing bit 12 for erratum 818325 / 852422: 0x00000000

        >>> _testobj._get_regid_to_val = lambda cpu_id: \
               { "(p15, 0, c15, c0, 1)": (1 << 12) | (1 << 24), \
                 "(p15, 0, c15, c0, 2)": (1 << 1)}
        >>> _info_io.seek(0); _info_io.truncate()
        >>> _testobj._check_one_cortex_a12({
        ...    "processor": 2,
        ...     "model name": "ARMv7 Processor rev 1 (v7l)",
        ...     "CPU implementer": ord("A"),
        ...     "CPU part": 0xc0d,
        ...     "CPU variant": 0,
        ...     "CPU revision": 1})
        >>> "good" in _info_io.getvalue()
        True

        >>> _testobj._check_one_cortex_a12({
        ...    "processor": 2,
        ...     "model name": "ARMv7 Processor rev 1 (v7l)",
        ...     "CPU implementer": ord("A"),
        ...     "CPU part": 0xc0d,
        ...     "CPU variant": 0,
        ...     "CPU revision": 2})
        Traceback (most recent call last):
        ...
        TestError: Unexpected A12 revision: r0p2
        """
        cpu_id = cpuinfo["processor"]
        variant = cpuinfo.get("CPU variant", -1)
        revision = cpuinfo.get("CPU revision", -1)

        # Handy to express this the way ARM does
        rev_str = "r%dp%d" % (variant, revision)

        # We don't ever expect an A12 newer than r0p1.  If we ever see one
        # then maybe the errata was fixed.
        if rev_str not in ("r0p0", "r0p1"):
            raise error.TestError("Unexpected A12 revision: %s" % rev_str)

        regid_to_val = self._get_regid_to_val(cpu_id)

        # Getting this means we're missing the change to expose debug
        # registers via arm_coprocessor_debug
        if not regid_to_val:
            raise error.TestError("Kernel didn't provide register vals")

        # Erratum 818325 applies to old A12s and erratum 852422 to newer.
        # Fix is to set bit 12 in diag register.  Confirm that's done.
        diag_reg = regid_to_val.get("(p15, 0, c15, c0, 1)")
        if diag_reg is None:
            raise error.TestError("Kernel didn't provide diag register")
        elif not (diag_reg & (1 << 12)):
            raise error.TestError(
                "Missing bit 12 for erratum 818325 / 852422: %#010x" %
                diag_reg)
        logging.info("CPU %d: erratum 818325 / 852422 good", cpu_id)

        # Erratum 821420 applies to all A12s.  Make sure bit 1 of the
        # internal feature register is set.
        int_feat_reg = regid_to_val.get("(p15, 0, c15, c0, 2)")
        if int_feat_reg is None:
            raise error.TestError("Kernel didn't provide internal feature reg")
        elif not (int_feat_reg & (1 << 1)):
            raise error.TestError(
                "Missing bit 1 for erratum 821420: %#010x" % int_feat_reg)
        logging.info("CPU %d: erratum 821420 good", cpu_id)

        # Erratum 825619 applies to all A12s.  Need bit 24 in diag reg.
        diag_reg = regid_to_val.get("(p15, 0, c15, c0, 1)")
        if diag_reg is None:
            raise error.TestError("Kernel didn't provide diag register")
        elif not (diag_reg & (1 << 24)):
            raise error.TestError(
                "Missing bit 24 for erratum 825619: %#010x" % diag_reg)
        logging.info("CPU %d: erratum 825619 good", cpu_id)

    def _check_one_cortex_a17(self, cpuinfo):
        """
        Check the errata for a Cortex-A17.

        :param cpuinfo: The CPU info for one CPU.  See _parse_cpu_info for
                        the format.

        >>> _testobj._get_regid_to_val = lambda cpu_id: {}
        >>> try:
        ...     _testobj._check_one_cortex_a17({
        ...         "processor": 2,
        ...         "model name": "ARMv7 Processor rev 1 (v7l)",
        ...         "CPU implementer": ord("A"),
        ...         "CPU part": 0xc0e,
        ...         "CPU variant": 1,
        ...         "CPU revision": 1})
        ... except Exception:
        ...     import traceback
        ...     print traceback.format_exc(),
        Traceback (most recent call last):
        ...
        TestError: Kernel didn't provide register vals

        >>> _testobj._get_regid_to_val = lambda cpu_id: \
               {"(p15, 0, c15, c0, 1)": 0}
        >>> try:
        ...     _testobj._check_one_cortex_a17({
        ...         "processor": 2,
        ...         "model name": "ARMv7 Processor rev 1 (v7l)",
        ...         "CPU implementer": ord("A"),
        ...         "CPU part": 0xc0e,
        ...         "CPU variant": 1,
        ...         "CPU revision": 1})
        ... except Exception:
        ...     import traceback
        ...     print traceback.format_exc(),
        Traceback (most recent call last):
        ...
        TestError: Missing bit 24 for erratum 852421: 0x00000000

        >>> _testobj._get_regid_to_val = lambda cpu_id: \
               {"(p15, 0, c15, c0, 1)": (1 << 12) | (1 << 24)}
        >>> _info_io.seek(0); _info_io.truncate()
        >>> _testobj._check_one_cortex_a17({
        ...    "processor": 2,
        ...     "model name": "ARMv7 Processor rev 1 (v7l)",
        ...     "CPU implementer": ord("A"),
        ...     "CPU part": 0xc0e,
        ...     "CPU variant": 1,
        ...     "CPU revision": 2})
        >>> "good" in _info_io.getvalue()
        True

        >>> _info_io.seek(0); _info_io.truncate()
        >>> _testobj._check_one_cortex_a17({
        ...    "processor": 2,
        ...     "model name": "ARMv7 Processor rev 1 (v7l)",
        ...     "CPU implementer": ord("A"),
        ...     "CPU part": 0xc0e,
        ...     "CPU variant": 2,
        ...     "CPU revision": 0})
        >>> print _info_io.getvalue()
        CPU 2: new A17 (r2p0) = no errata
        """
        cpu_id = cpuinfo["processor"]
        variant = cpuinfo.get("CPU variant", -1)
        revision = cpuinfo.get("CPU revision", -1)

        # Handy to express this the way ARM does
        rev_str = "r%dp%d" % (variant, revision)

        regid_to_val = self._get_regid_to_val(cpu_id)

        # Erratum 852421 and 852423 apply to "r1p0", "r1p1", "r1p2"
        if rev_str in ("r1p0", "r1p1", "r1p2"):
            # Getting this means we're missing the change to expose debug
            # registers via arm_coprocessor_debug
            if not regid_to_val:
                raise error.TestError("Kernel didn't provide register vals")

            diag_reg = regid_to_val.get("(p15, 0, c15, c0, 1)")
            if diag_reg is None:
                raise error.TestError("Kernel didn't provide diag register")
            elif not (diag_reg & (1 << 24)):
                raise error.TestError(
                    "Missing bit 24 for erratum 852421: %#010x" % diag_reg)
            logging.info("CPU %d: erratum 852421 good",cpu_id)

            diag_reg = regid_to_val.get("(p15, 0, c15, c0, 1)")
            if diag_reg is None:
                raise error.TestError("Kernel didn't provide diag register")
            elif not (diag_reg & (1 << 12)):
                raise error.TestError(
                    "Missing bit 12 for erratum 852423: %#010x" % diag_reg)
            logging.info("CPU %d: erratum 852423 good",cpu_id)
        else:
            logging.info("CPU %d: new A17 (%s) = no errata", cpu_id, rev_str)

    def _check_one_armv7(self, cpuinfo):
        """
        Check the errata for one ARMv7 CPU.

        :param cpuinfo: The CPU info for one CPU.  See _parse_cpu_info for
                        the format.

        >>> _info_io.seek(0); _info_io.truncate()
        >>> _testobj._check_one_cpu({
        ...     "processor": 2,
        ...     "model name": "ARMv7 Processor rev 1 (v7l)",
        ...     "CPU implementer": ord("B"),
        ...     "CPU part": 0xc99,
        ...     "CPU variant": 7,
        ...     "CPU revision": 9})
        >>> print _info_io.getvalue()
        CPU 2: No errata tested: imp=0x42, part=0xc99
        """
        # Get things so we don't worry about key errors below
        cpu_id = cpuinfo["processor"]
        implementer = cpuinfo.get("CPU implementer", "?")
        part = cpuinfo.get("CPU part", 0xfff)

        if implementer == ord("A") and part == 0xc0d:
            self._check_one_cortex_a12(cpuinfo)
        elif implementer == ord("A") and part == 0xc0e:
            self._check_one_cortex_a17(cpuinfo)
        else:
            logging.info("CPU %d: No errata tested: imp=%#04x, part=%#05x",
                         cpu_id, implementer, part)

    def _check_one_cpu(self, cpuinfo):
        """
        Check the errata for one CPU.

        :param cpuinfo: The CPU info for one CPU.  See _parse_cpu_info for
                        the format.

        >>> _info_io.seek(0); _info_io.truncate()
        >>> _testobj._check_one_cpu({
        ...    "processor": 0,
        ...    "model name": "LEGv7 Processor"})
        >>> print _info_io.getvalue()
        CPU 0: Not an ARM, skipping: LEGv7 Processor

        >>> _info_io.seek(0); _info_io.truncate()
        >>> _testobj._check_one_cpu({
        ...    "processor": 1,
        ...    "model name": "ARMv99 Processor"})
        >>> print _info_io.getvalue()
        CPU 1: No errata tested; model: ARMv99 Processor
        """
        if cpuinfo["model name"].startswith("ARMv7"):
            self._check_one_armv7(cpuinfo)
        elif cpuinfo["model name"].startswith("ARM"):
            logging.info("CPU %d: No errata tested; model: %s",
                         cpuinfo["processor"], cpuinfo["model name"])
        else:
            logging.info("CPU %d: Not an ARM, skipping: %s",
                         cpuinfo["processor"], cpuinfo["model name"])

    def run_once(self, doctest=False):
        """
        Run the test.

        :param doctest: If true we will just run our doctests.  We'll set these
                        globals to help our tests:
                        - _testobj: An instance of this object.
                        - _info_io: A StringIO that's stuffed into logging.info
                          so we can see what was written there.
        ...
        """
        if doctest:
            import doctest, inspect, StringIO
            global _testobj, _info_io

            # Keep a backup of _get_regid_to_val() since tests will clobber.
            old_get_regid_to_val = self._get_regid_to_val

            # Mock out logging.info to help tests.
            _info_io = StringIO.StringIO()
            old_logging_info = logging.info
            logging.info = lambda fmt, *args: _info_io.write(fmt % args)

            # Stash an object in a global to help tests
            _testobj = self
            try:
                failure_count, test_count = doctest.testmod(
                    inspect.getmodule(self), optionflags=doctest.ELLIPSIS)
            finally:
                logging.info = old_logging_info

                # Technically don't need to clean this up, but let's be nice.
                self._get_regid_to_val = old_get_regid_to_val

            logging.info("Doctest ran %d tests, had %d failures",
                         test_count, failure_count)
            return

        if utils.get_cpu_soc_family() != 'arm':
            raise error.TestNAError('Applicable to ARM processors only')

        cpuinfo = self._parse_cpu_info(utils.read_file('/proc/cpuinfo'))

        for cpu_id in sorted(cpuinfo.keys()):
            self._check_one_cpu(cpuinfo[cpu_id])

        sys_power.do_suspend(self.SECS_TO_SUSPEND)

        for cpu_id in sorted(cpuinfo.keys()):
            self._check_one_cpu(cpuinfo[cpu_id])
