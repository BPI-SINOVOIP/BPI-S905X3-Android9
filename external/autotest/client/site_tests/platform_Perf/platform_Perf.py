# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import errno, os, re, subprocess
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error


class platform_Perf(test.test):
    """
    Gathers perf data and makes sure it is well-formed.
    """
    version = 1

    # Whitelist of DSOs that are expected to appear in perf data from a CrOS
    # device. The actual name may change so use regex pattern matching. This is
    # a list of allowed but not required DSOs. With this list, we can filter out
    # unknown DSOs that might not have a build ID, e.g. JIT code.
    _KERNEL_NAME_REGEX = re.compile(r'.*kernel\.kallsyms.*')
    _DSO_WHITELIST_REGEX = [
      _KERNEL_NAME_REGEX,
      re.compile(r'bash'),
      re.compile(r'chrome'),
      re.compile(r'ld-.*\.so.*'),
      # For simplicity since the libbase binaries are built together, we assume
      # that if one of them (libbase-core) was properly built and passes this
      # test, then the others will pass as well. It's easier than trying to
      # include all libbase-* while filtering out libbase-XXXXXX.so, which is a
      # text file that links to the other files.
      re.compile(r'libbase-core-.*\.so.*'),
      re.compile(r'libc-.*\.so.*'),
      re.compile(r'libdbus-.*\.so.*'),
      re.compile(r'libpthread-.*\.so.*'),
      re.compile(r'libstdc\+\+.*\.so\..*'),
    ]


    def run_once(self):
        """
        Collect a perf data profile and check the detailed perf report.
        """
        keyvals = {}
        num_errors = 0

        try:
            # Create temporary file and get its name. Then close it.
            perf_file_path = os.tempnam()

            # Perf command for recording a profile.
            perf_record_args = [ 'perf', 'record', '-a', '-o', perf_file_path,
                                 '--', 'sleep', '2']
            # Perf command for getting a detailed report.
            perf_report_args = [ 'perf', 'report', '-D', '-i', perf_file_path ]
            # Perf command for getting a report grouped by DSO name.
            perf_report_dso_args = [ 'perf', 'report', '--sort', 'dso', '-i',
                                     perf_file_path ]
            # Perf command for getting a list of all build IDs in a data file.
            perf_buildid_list_args = [ 'perf', 'buildid-list', '-i',
                                       perf_file_path ]

            try:
                subprocess.check_output(perf_record_args,
                                        stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as cmd_error:
                raise error.TestFail("Running command [%s] failed: %s" %
                                     (' '.join(perf_record_args),
                                      cmd_error.output))

            # Make sure the file still exists.
            if not os.path.isfile(perf_file_path):
                raise error.TestFail('Could not find perf output file: ' +
                                     perf_file_path)

            # Get detailed perf data view and extract the line containing the
            # kernel MMAP summary.
            kernel_mapping = None
            p = subprocess.Popen(perf_report_args, stdout=subprocess.PIPE)
            for line in p.stdout:
                if ('PERF_RECORD_MMAP' in line and
                    self._KERNEL_NAME_REGEX.match(line)):
                    kernel_mapping = line
                    break

            # Read the rest of output to EOF.
            for _ in p.stdout:
                pass
            p.wait();

            # Generate a list of whitelisted DSOs from the perf report.
            dso_list = []
            p = subprocess.Popen(perf_report_dso_args, stdout=subprocess.PIPE)
            for line in p.stdout:
                # Skip comments.
                if line.startswith('#'):
                    continue
                # The output consists of percentage and DSO name.
                tokens = line.split()
                if len(tokens) < 2:
                    continue
                # Store the DSO name if it appears in the whitelist.
                dso_name = tokens[1]
                for regex in self._DSO_WHITELIST_REGEX:
                    if regex.match(dso_name):
                        dso_list += [dso_name]

            p.wait();

            # Generate a mapping of DSOs to their build IDs.
            dso_to_build_ids = {}
            p = subprocess.Popen(perf_buildid_list_args, stdout=subprocess.PIPE)
            for line in p.stdout:
                # The output consists of build ID and DSO name.
                tokens = line.split()
                if len(tokens) < 2:
                    continue
                # The build ID list uses the full path of the DSOs, while the
                # report output usesonly the basename. Store the basename to
                # make lookups easier.
                dso_to_build_ids[os.path.basename(tokens[1])] = tokens[0]

            p.wait();


        finally:
            # Delete the perf data file.
            try:
                os.remove(perf_file_path)
            except OSError as e:
                if e.errno != errno.ENONENT: raise

        if kernel_mapping is None:
            raise error.TestFail('Could not find kernel mapping in perf '
                                 'report.')
        # Get the kernel mapping values.
        kernel_mapping = kernel_mapping.split(':')[2]
        start, length, pgoff = re.sub(r'[][()@]', ' ',
                                      kernel_mapping).strip().split()

        # Check that all whitelisted DSOs from the report have build IDs.
        kernel_name = None
        kernel_build_id = None
        for dso in dso_list:
            if dso not in dso_to_build_ids:
                raise error.TestFail('Could not find build ID for %s' % dso)
            if self._KERNEL_NAME_REGEX.match(dso):
                kernel_name = dso
                kernel_build_id = dso_to_build_ids[dso]

        # Make sure the kernel build ID was found.
        if not kernel_build_id:
            raise error.TestFail('Could not find kernel entry (containing '
                                 '"%s") in build ID list' % self._KERNEL_NAME)

        # Write keyvals.
        keyvals = {}
        keyvals['start'] = start
        keyvals['length'] = length
        keyvals['pgoff'] = pgoff
        keyvals['kernel_name'] = kernel_name
        keyvals['kernel_build_id'] = kernel_build_id
        self.write_perf_keyval(keyvals)

        # Make sure that the kernel mapping values follow an expected pattern,
        #
        # Expect one of two patterns:
        # (1) start == pgoff, e.g.:
        #   start=0x80008200
        #   pgoff=0x80008200
        #   len  =0xfffffff7ff7dff
        # (2) start < pgoff < start + len, e.g.:
        #   start=0x3bc00000
        #   pgoff=0xffffffffbcc00198
        #   len  =0xffffffff843fffff
        start = int(start, 0)
        length = int(length, 0)
        pgoff = int(pgoff, 0)
        if not (start == pgoff or start < pgoff < start + length):
            raise error.TestFail('Improper kernel mapping values!')
