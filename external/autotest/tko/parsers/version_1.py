import json
import math
import os
import re

import common
from autotest_lib.tko import models
from autotest_lib.tko import status_lib
from autotest_lib.tko import utils as tko_utils
from autotest_lib.tko.parsers import base
from autotest_lib.tko.parsers import version_0


class job(version_0.job):
    """Represents a job."""

    def exit_status(self):
        """Returns the string exit status of this job."""

        # Find the .autoserv_execute path.
        top_dir = tko_utils.find_toplevel_job_dir(self.dir)
        if not top_dir:
            return 'ABORT'
        execute_path = os.path.join(top_dir, '.autoserv_execute')

        # If for some reason we can't read the status code, assume disaster.
        if not os.path.exists(execute_path):
            return 'ABORT'
        lines = open(execute_path).readlines()
        if len(lines) < 2:
            return 'ABORT'
        try:
            status_code = int(lines[1])
        except ValueError:
            return 'ABORT'

        if not os.WIFEXITED(status_code):
            # Looks like a signal - an ABORT.
            return 'ABORT'
        elif os.WEXITSTATUS(status_code) != 0:
            # Looks like a non-zero exit - a failure.
            return 'FAIL'
        else:
            # Looks like exit code == 0.
            return 'GOOD'


class kernel(models.kernel):
    """Represents a kernel."""

    def __init__(self, base, patches):
        if base:
            patches = [patch(*p.split()) for p in patches]
            hashes = [p.hash for p in patches]
            kernel_hash = self.compute_hash(base, hashes)
        else:
            base = 'UNKNOWN'
            patches = []
            kernel_hash = 'UNKNOWN'
        super(kernel, self).__init__(base, patches, kernel_hash)


class test(models.test):
    """Represents a test."""

    @staticmethod
    def load_iterations(keyval_path):
        return iteration.load_from_keyval(keyval_path)


    @staticmethod
    def load_perf_values(perf_values_file):
        return perf_value_iteration.load_from_perf_values_file(
                perf_values_file)


class iteration(models.iteration):
    """Represents an iteration."""

    @staticmethod
    def parse_line_into_dicts(line, attr_dict, perf_dict):
        key, val_type, value = "", "", ""

        # Figure out what the key, value and keyval type are.
        typed_match = re.search('^([^=]*)\{(\w*)\}=(.*)$', line)
        if typed_match:
            key, val_type, value = typed_match.groups()
        else:
            # Old-fashioned untyped match, assume perf.
            untyped_match = re.search('^([^=]*)=(.*)$', line)
            if untyped_match:
                key, value = untyped_match.groups()
                val_type = 'perf'

        # Parse the actual value into a dict.
        try:
            if val_type == 'attr':
                attr_dict[key] = value
            elif val_type == 'perf':
                perf_dict[key] = float(value)
            else:
                raise ValueError
        except ValueError:
            msg = ('WARNING: line "%s" found in test '
                   'iteration keyval could not be parsed')
            msg %= line
            tko_utils.dprint(msg)


class perf_value_iteration(models.perf_value_iteration):
    """Represents a perf value iteration."""

    @staticmethod
    def parse_line_into_dict(line):
        """
        Parse a perf measurement text line into a dictionary.

        The line is assumed to be a JSON-formatted string containing key/value
        pairs, where each pair represents a piece of information associated
        with a measured perf metric:

            'description': a string description for the perf metric.
            'value': a numeric value, or list of numeric values.
            'units': the string units associated with the perf metric.
            'higher_is_better': a boolean whether a higher value is considered
                better.  If False, a lower value is considered better.
            'graph': a string indicating the name of the perf dashboard graph
                     on which the perf data will be displayed.

        The resulting dictionary will also have a standard deviation key/value
        pair, 'stddev'.  If the perf measurement value is a list of values
        instead of a single value, then the average and standard deviation of
        the list of values is computed and stored.  If a single value, the
        value itself is used, and is associated with a standard deviation of 0.

        @param line: A string line of JSON text from a perf measurements output
            file.

        @return A dictionary containing the parsed perf measurement information
            along with a computed standard deviation value (key 'stddev'), or
            an empty dictionary if the inputted line cannot be parsed.
        """
        try:
            perf_dict = json.loads(line)
        except ValueError:
            msg = 'Could not parse perf measurements line as json: "%s"' % line
            tko_utils.dprint(msg)
            return {}

        def mean_and_standard_deviation(data):
            """
            Computes the mean and standard deviation of a list of numbers.

            @param data: A list of numbers.

            @return A 2-tuple (mean, standard_deviation) computed from the list
                of numbers.

            """
            n = len(data)
            if n == 0:
                return 0.0, 0.0
            if n == 1:
                return data[0], 0.0
            mean = float(sum(data)) / n
            # Divide by n-1 to compute "sample standard deviation".
            variance = sum([(elem - mean) ** 2 for elem in data]) / (n - 1)
            return mean, math.sqrt(variance)

        value = perf_dict['value']
        perf_dict['stddev'] = 0.0
        if isinstance(value, list):
            value, stddev = mean_and_standard_deviation(map(float, value))
            perf_dict['value'] = value
            perf_dict['stddev'] = stddev

        return perf_dict


class status_line(version_0.status_line):
    """Represents a status line."""

    def __init__(self, indent, status, subdir, testname, reason,
                 optional_fields):
        # Handle INFO fields.
        if status == 'INFO':
            self.type = 'INFO'
            self.indent = indent
            self.status = self.subdir = self.testname = self.reason = None
            self.optional_fields = optional_fields
        else:
            # Everything else is backwards compatible.
            super(status_line, self).__init__(indent, status, subdir,
                                              testname, reason,
                                              optional_fields)


    def is_successful_reboot(self, current_status):
        """
        Checks whether the status represents a successful reboot.

        @param current_status: A string representing the current status.

        @return True, if the status represents a successful reboot, or False
            if not.

        """
        # Make sure this is a reboot line.
        if self.testname != 'reboot':
            return False

        # Make sure this was not a failure.
        if status_lib.is_worse_than_or_equal_to(current_status, 'FAIL'):
            return False

        # It must have been a successful reboot.
        return True


    def get_kernel(self):
        # Get the base kernel version.
        fields = self.optional_fields
        base = re.sub('-autotest$', '', fields.get('kernel', ''))
        # Get a list of patches.
        patches = []
        patch_index = 0
        while ('patch%d' % patch_index) in fields:
            patches.append(fields['patch%d' % patch_index])
            patch_index += 1
        # Create a new kernel instance.
        return kernel(base, patches)


    def get_timestamp(self):
        return tko_utils.get_timestamp(self.optional_fields, 'timestamp')


# The default implementations from version 0 will do for now.
patch = version_0.patch


class parser(base.parser):
    """Represents a parser."""

    @staticmethod
    def make_job(dir):
        return job(dir)


    @staticmethod
    def make_dummy_abort(indent, subdir, testname, timestamp, reason):
        """
        Creates an abort string.

        @param indent: The number of indentation levels for the string.
        @param subdir: The subdirectory name.
        @param testname: The test name.
        @param timestamp: The timestamp value.
        @param reason: The reason string.

        @return A string describing the abort.

        """
        indent = '\t' * indent
        if not subdir:
            subdir = '----'
        if not testname:
            testname = '----'

        # There is no guarantee that this will be set.
        timestamp_field = ''
        if timestamp:
            timestamp_field = '\ttimestamp=%s' % timestamp

        msg = indent + 'END ABORT\t%s\t%s%s\t%s'
        return msg % (subdir, testname, timestamp_field, reason)


    @staticmethod
    def put_back_line_and_abort(
        line_buffer, line, indent, subdir, testname, timestamp, reason):
        """
        Appends a line to the line buffer and aborts.

        @param line_buffer: A line_buffer object.
        @param line: A line to append to the line buffer.
        @param indent: The number of indentation levels.
        @param subdir: The subdirectory name.
        @param testname: The test name.
        @param timestamp: The timestamp value.
        @param reason: The reason string.

        """
        tko_utils.dprint('Unexpected indent: aborting log parse')
        line_buffer.put_back(line)
        abort = parser.make_dummy_abort(
            indent, subdir, testname, timestamp, reason)
        line_buffer.put_back(abort)


    def state_iterator(self, buffer):
        """
        Yields a list of tests out of the buffer.

        @param buffer: a buffer object

        """
        line = None
        new_tests = []
        job_count, boot_count = 0, 0
        min_stack_size = 0
        stack = status_lib.status_stack()
        current_kernel = kernel("", [])  # UNKNOWN
        current_status = status_lib.statuses[-1]
        current_reason = None
        started_time_stack = [None]
        subdir_stack = [None]
        testname_stack = [None]
        running_test = None
        running_reasons = set()
        ignored_lines = []
        yield []   # We're ready to start running.

        def print_ignored_lines():
            """
            Prints the ignored_lines using tko_utils.dprint method.
            """
            tko_utils.dprint('The following lines were ignored:')
            for line in ignored_lines:
                tko_utils.dprint(line)
            tko_utils.dprint('---------------------------------')

        # Create a RUNNING SERVER_JOB entry to represent the entire test.
        running_job = test.parse_partial_test(self.job, '----', 'SERVER_JOB',
                                              '', current_kernel,
                                              self.job.started_time)
        new_tests.append(running_job)

        while True:
            # Are we finished with parsing?
            if buffer.size() == 0 and self.finished:
                if ignored_lines:
                    print_ignored_lines()
                    ignored_lines = []
                if stack.size() == 0:
                    break
                # We have status lines left on the stack;
                # we need to implicitly abort them first.
                tko_utils.dprint('\nUnexpected end of job, aborting')
                abort_subdir_stack = list(subdir_stack)
                if self.job.aborted_by:
                    reason = 'Job aborted by %s' % self.job.aborted_by
                    reason += self.job.aborted_on.strftime(
                        ' at %b %d %H:%M:%S')
                else:
                    reason = 'Job aborted unexpectedly'

                timestamp = line.optional_fields.get('timestamp')
                for i in reversed(xrange(stack.size())):
                    if abort_subdir_stack:
                        subdir = abort_subdir_stack.pop()
                    else:
                        subdir = None
                    abort = self.make_dummy_abort(
                        i, subdir, subdir, timestamp, reason)
                    buffer.put(abort)

            # Stop processing once the buffer is empty.
            if buffer.size() == 0:
                yield new_tests
                new_tests = []
                continue

            # Reinitialize the per-iteration state.
            started_time = None
            finished_time = None

            # Get the next line.
            raw_line = status_lib.clean_raw_line(buffer.get())
            line = status_line.parse_line(raw_line)
            if line is None:
                ignored_lines.append(raw_line)
                continue
            elif ignored_lines:
                print_ignored_lines()
                ignored_lines = []

            # Do an initial sanity check of the indentation.
            expected_indent = stack.size()
            if line.type == 'END':
                expected_indent -= 1
            if line.indent < expected_indent:
                # ABORT the current level if indentation was unexpectedly low.
                self.put_back_line_and_abort(
                    buffer, raw_line, stack.size() - 1, subdir_stack[-1],
                    testname_stack[-1], line.optional_fields.get('timestamp'),
                    line.reason)
                continue
            elif line.indent > expected_indent:
                # Ignore the log if the indent was unexpectedly high.
                tko_utils.dprint('ignoring line because of extra indentation')
                continue

            # Initial line processing.
            if line.type == 'START':
                stack.start()
                started_time = line.get_timestamp()
                testname = None
                if (line.testname is None and line.subdir is None
                    and not running_test):
                    # We just started a client; all tests are relative to here.
                    min_stack_size = stack.size()
                    # Start a "RUNNING" CLIENT_JOB entry.
                    job_name = 'CLIENT_JOB.%d' % job_count
                    running_client = test.parse_partial_test(self.job, None,
                                                             job_name,
                                                             '', current_kernel,
                                                             started_time)
                    msg = 'RUNNING: %s\n%s\n'
                    msg %= (running_client.status, running_client.testname)
                    tko_utils.dprint(msg)
                    new_tests.append(running_client)
                    testname = running_client.testname
                elif stack.size() == min_stack_size + 1 and not running_test:
                    # We just started a new test; insert a running record.
                    running_reasons = set()
                    if line.reason:
                        running_reasons.add(line.reason)
                    running_test = test.parse_partial_test(self.job,
                                                           line.subdir,
                                                           line.testname,
                                                           line.reason,
                                                           current_kernel,
                                                           started_time)
                    msg = 'RUNNING: %s\nSubdir: %s\nTestname: %s\n%s'
                    msg %= (running_test.status, running_test.subdir,
                            running_test.testname, running_test.reason)
                    tko_utils.dprint(msg)
                    new_tests.append(running_test)
                    testname = running_test.testname
                started_time_stack.append(started_time)
                subdir_stack.append(line.subdir)
                testname_stack.append(testname)
                continue
            elif line.type == 'INFO':
                fields = line.optional_fields
                # Update the current kernel if one is defined in the info.
                if 'kernel' in fields:
                    current_kernel = line.get_kernel()
                # Update the SERVER_JOB reason if one was logged for an abort.
                if 'job_abort_reason' in fields:
                    running_job.reason = fields['job_abort_reason']
                    new_tests.append(running_job)
                continue
            elif line.type == 'STATUS':
                # Update the stacks.
                if line.subdir and stack.size() > min_stack_size:
                    subdir_stack[-1] = line.subdir
                    testname_stack[-1] = line.testname
                # Update the status, start and finished times.
                stack.update(line.status)
                if status_lib.is_worse_than_or_equal_to(line.status,
                                                        current_status):
                    if line.reason:
                        # Update the status of a currently running test.
                        if running_test:
                            running_reasons.add(line.reason)
                            running_reasons = tko_utils.drop_redundant_messages(
                                    running_reasons)
                            sorted_reasons = sorted(running_reasons)
                            running_test.reason = ', '.join(sorted_reasons)
                            current_reason = running_test.reason
                            new_tests.append(running_test)
                            msg = 'update RUNNING reason: %s' % line.reason
                            tko_utils.dprint(msg)
                        else:
                            current_reason = line.reason
                    current_status = stack.current_status()
                started_time = None
                finished_time = line.get_timestamp()
                # If this is a non-test entry there's nothing else to do.
                if line.testname is None and line.subdir is None:
                    continue
            elif line.type == 'END':
                # Grab the current subdir off of the subdir stack, or, if this
                # is the end of a job, just pop it off.
                if (line.testname is None and line.subdir is None
                    and not running_test):
                    min_stack_size = stack.size() - 1
                    subdir_stack.pop()
                    testname_stack.pop()
                else:
                    line.subdir = subdir_stack.pop()
                    testname_stack.pop()
                    if not subdir_stack[-1] and stack.size() > min_stack_size:
                        subdir_stack[-1] = line.subdir
                # Update the status, start and finished times.
                stack.update(line.status)
                current_status = stack.end()
                if stack.size() > min_stack_size:
                    stack.update(current_status)
                    current_status = stack.current_status()
                started_time = started_time_stack.pop()
                finished_time = line.get_timestamp()
                # Update the current kernel.
                if line.is_successful_reboot(current_status):
                    current_kernel = line.get_kernel()
                # Adjust the testname if this is a reboot.
                if line.testname == 'reboot' and line.subdir is None:
                    line.testname = 'boot.%d' % boot_count
            else:
                assert False

            # Have we just finished a test?
            if stack.size() <= min_stack_size:
                # If there was no testname, just use the subdir.
                if line.testname is None:
                    line.testname = line.subdir
                # If there was no testname or subdir, use 'CLIENT_JOB'.
                if line.testname is None:
                    line.testname = 'CLIENT_JOB.%d' % job_count
                    running_test = running_client
                    job_count += 1
                    if not status_lib.is_worse_than_or_equal_to(
                        current_status, 'ABORT'):
                        # A job hasn't really failed just because some of the
                        # tests it ran have.
                        current_status = 'GOOD'

                if not current_reason:
                    current_reason = line.reason
                new_test = test.parse_test(self.job,
                                           line.subdir,
                                           line.testname,
                                           current_status,
                                           current_reason,
                                           current_kernel,
                                           started_time,
                                           finished_time,
                                           running_test)
                running_test = None
                current_status = status_lib.statuses[-1]
                current_reason = None
                if new_test.testname == ('boot.%d' % boot_count):
                    boot_count += 1
                msg = 'ADD: %s\nSubdir: %s\nTestname: %s\n%s'
                msg %= (new_test.status, new_test.subdir,
                        new_test.testname, new_test.reason)
                tko_utils.dprint(msg)
                new_tests.append(new_test)

        # The job is finished; produce the final SERVER_JOB entry and exit.
        final_job = test.parse_test(self.job, '----', 'SERVER_JOB',
                                    self.job.exit_status(), running_job.reason,
                                    current_kernel,
                                    self.job.started_time,
                                    self.job.finished_time,
                                    running_job)
        new_tests.append(final_job)
        yield new_tests
