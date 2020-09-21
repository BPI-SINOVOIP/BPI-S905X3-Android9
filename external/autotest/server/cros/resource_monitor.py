# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import csv
import cStringIO
import random
import re
import collections

from autotest_lib.client.common_lib.cros import path_utils

class ResourceMonitorRawResult(object):
    """Encapsulates raw resource_monitor results."""

    def __init__(self, raw_results_filename):
        self._raw_results_filename = raw_results_filename


    def get_parsed_results(self):
        """Constructs parsed results from the raw ones.

        @return ResourceMonitorParsedResult object

        """
        return ResourceMonitorParsedResult(self.raw_results_filename)


    @property
    def raw_results_filename(self):
        """@return string filename storing the raw top command output."""
        return self._raw_results_filename


class IncorrectTopFormat(Exception):
    """Thrown if top output format is not as expected"""
    pass


def _extract_value_before_single_keyword(line, keyword):
    """Extract word occurring immediately before the specified keyword.

    @param line string the line in which to search for the keyword.
    @param keyword string the keyword to look for. Can be a regexp.
    @return string the word just before the keyword.

    """
    pattern = ".*?(\S+) " + keyword
    matches = re.match(pattern, line)
    if matches is None or len(matches.groups()) != 1:
        raise IncorrectTopFormat

    return matches.group(1)


def _extract_values_before_keywords(line, *args):
    """Extract the words occuring immediately before each specified
        keyword in args.

    @param line string the string to look for the keywords.
    @param args variable number of string args the keywords to look for.
    @return string list the words occuring just before each keyword.

    """
    line_nocomma = re.sub(",", " ", line)
    line_singlespace = re.sub("\s+", " ", line_nocomma)

    return [_extract_value_before_single_keyword(
            line_singlespace, arg) for arg in args]


def _find_top_output_identifying_pattern(line):
    """Return true iff the line looks like the first line of top output.

    @param line string to look for the pattern
    @return boolean

    """
    pattern ="\s*top\s*-.*up.*users.*"
    matches = re.match(pattern, line)
    return matches is not None


class ResourceMonitorParsedResult(object):
    """Encapsulates logic to parse and represent top command results."""

    _columns = ["Time", "UserCPU", "SysCPU", "NCPU", "Idle",
            "IOWait", "IRQ", "SoftIRQ", "Steal",
            "MemUnits", "UsedMem", "FreeMem",
            "SwapUnits", "UsedSwap", "FreeSwap"]
    UtilValues = collections.namedtuple('UtilValues', ' '.join(_columns))

    def __init__(self, raw_results_filename):
        """Construct a ResourceMonitorResult.

        @param raw_results_filename string filename of raw batch top output.

        """
        self._raw_results_filename = raw_results_filename
        self.parse_resource_monitor_results()


    def parse_resource_monitor_results(self):
        """Extract utilization metrics from output file."""
        self._utils_over_time = []

        with open(self._raw_results_filename, "r") as results_file:
            while True:
                curr_line = '\n'
                while curr_line != '' and \
                        not _find_top_output_identifying_pattern(curr_line):
                    curr_line = results_file.readline()
                if curr_line == '':
                    break
                try:
                    time, = _extract_values_before_keywords(curr_line, "up")

                    # Ignore one line.
                    _ = results_file.readline()

                    # Get the cpu usage.
                    curr_line = results_file.readline()
                    (cpu_user, cpu_sys, cpu_nice, cpu_idle, io_wait, irq, sirq,
                            steal) = _extract_values_before_keywords(curr_line,
                            "us", "sy", "ni", "id", "wa", "hi", "si", "st")

                    # Get memory usage.
                    curr_line = results_file.readline()
                    (mem_units, mem_free,
                            mem_used) = _extract_values_before_keywords(
                            curr_line, "Mem", "free", "used")

                    # Get swap usage.
                    curr_line = results_file.readline()
                    (swap_units, swap_free,
                            swap_used) = _extract_values_before_keywords(
                            curr_line, "Swap", "free", "used")

                    curr_util_values = ResourceMonitorParsedResult.UtilValues(
                            Time=time, UserCPU=cpu_user,
                            SysCPU=cpu_sys, NCPU=cpu_nice, Idle=cpu_idle,
                            IOWait=io_wait, IRQ=irq, SoftIRQ=sirq, Steal=steal,
                            MemUnits=mem_units, UsedMem=mem_used,
                            FreeMem=mem_free,
                            SwapUnits=swap_units, UsedSwap=swap_used,
                            FreeSwap=swap_free)
                    self._utils_over_time.append(curr_util_values)
                except IncorrectTopFormat:
                    logging.error(
                            "Top output format incorrect. Aborting parse.")
                    return


    def __repr__(self):
        output_stringfile = cStringIO.StringIO()
        self.save_to_file(output_stringfile)
        return output_stringfile.getvalue()


    def save_to_file(self, file):
        """Save parsed top results to file

        @param file file object to write to

        """
        if len(self._utils_over_time) < 1:
            logging.warning("Tried to save parsed results, but they were "
                    "empty. Skipping the save.")
            return
        csvwriter = csv.writer(file, delimiter=',')
        csvwriter.writerow(self._utils_over_time[0]._fields)
        for row in self._utils_over_time:
            csvwriter.writerow(row)


    def save_to_filename(self, filename):
        """Save parsed top results to filename

        @param filename string filepath to write to

        """
        out_file = open(filename, "wb")
        self.save_to_file(out_file)
        out_file.close()


class ResourceMonitorConfig(object):
    """Defines a single top run."""

    DEFAULT_MONITOR_PERIOD = 3

    def __init__(self, monitor_period=DEFAULT_MONITOR_PERIOD,
            rawresult_output_filename=None):
        """Construct a ResourceMonitorConfig.

        @param monitor_period float seconds between successive top refreshes.
        @param rawresult_output_filename string filename to output the raw top
                                                results to

        """
        if monitor_period < 0.1:
            logging.info('Monitor period must be at least 0.1s.'
                    ' Given: %r. Defaulting to 0.1s', monitor_period)
            monitor_period = 0.1

        self._monitor_period = monitor_period
        self._server_outfile = rawresult_output_filename


class ResourceMonitor(object):
    """Delegate to run top on a client.

    Usage example (call from a test):
    rmc = resource_monitor.ResourceMonitorConfig(monitor_period=1,
            rawresult_output_filename=os.path.join(self.resultsdir,
                                                    'topout.txt'))
    with resource_monitor.ResourceMonitor(self.context.client.host, rmc) as rm:
        rm.start()
        <operation_to_monitor>
        rm_raw_res = rm.stop()
        rm_res = rm_raw_res.get_parsed_results()
        rm_res.save_to_filename(
                os.path.join(self.resultsdir, 'resource_mon.csv'))

    """

    def __init__(self, client_host, config):
        """Construct a ResourceMonitor.

        @param client_host: SSHHost object representing a remote ssh host

        """
        self._client_host = client_host
        self._config = config
        self._command_top = path_utils.must_be_installed(
                'top', host=self._client_host)
        self._top_pid = None


    def __enter__(self):
        return self


    def __exit__(self, exc_type, exc_value, traceback):
        if self._top_pid is not None:
            self._client_host.run('kill %s && rm %s' %
                    (self._top_pid, self._client_outfile), ignore_status=True)
        return True


    def start(self):
        """Run top and save results to a temp file on the client."""
        if self._top_pid is not None:
            logging.debug("Tried to start monitoring before stopping. "
                    "Ignoring request.")
            return

        # Decide where to write top's output to (on the client).
        random_suffix = random.random()
        self._client_outfile = '/tmp/topcap-%r' % random_suffix

        # Run top on the client.
        top_command = '%s -b -d%d > %s' % (self._command_top,
                self._config._monitor_period, self._client_outfile)
        logging.info('Running top.')
        self._top_pid = self._client_host.run_background(top_command)
        logging.info('Top running with pid %s', self._top_pid)


    def stop(self):
        """Stop running top and return the results.

        @return ResourceMonitorRawResult object

        """
        logging.debug("Stopping monitor")
        if self._top_pid is None:
            logging.debug("Tried to stop monitoring before starting. "
                    "Ignoring request.")
            return

        # Stop top on the client.
        self._client_host.run('kill %s' % self._top_pid, ignore_status=True)

        # Get the top output file from the client onto the server.
        if self._config._server_outfile is None:
            self._config._server_outfile = self._client_outfile
        self._client_host.get_file(
                self._client_outfile, self._config._server_outfile)

        # Delete the top output file from client.
        self._client_host.run('rm %s' % self._client_outfile,
                ignore_status=True)

        self._top_pid = None
        logging.info("Saved resource monitor results at %s",
                self._config._server_outfile)
        return ResourceMonitorRawResult(self._config._server_outfile)
