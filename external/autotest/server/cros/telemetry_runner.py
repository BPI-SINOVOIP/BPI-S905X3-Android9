# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import StringIO

from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.common_lib.cros import dev_server


TELEMETRY_RUN_BENCHMARKS_SCRIPT = 'tools/perf/run_benchmark'
TELEMETRY_RUN_TESTS_SCRIPT = 'tools/telemetry/run_tests'
TELEMETRY_RUN_GPU_TESTS_SCRIPT = 'content/test/gpu/run_gpu_integration_test.py'
TELEMETRY_TIMEOUT_MINS = 150

DUT_CHROME_ROOT = '/usr/local/telemetry/src'

# Result Statuses
SUCCESS_STATUS = 'SUCCESS'
WARNING_STATUS = 'WARNING'
FAILED_STATUS = 'FAILED'

# A list of benchmarks with that the telemetry test harness can run on dut.
ON_DUT_WHITE_LIST = ['cros_ui_smoothness',
                     'dromaeo.domcoreattr',
                     'dromaeo.domcoremodify',
                     'dromaeo.domcorequery',
                     'dromaeo.domcoretraverse',
                     'image_decoding.image_decoding_measurement',
                     'jetstream',
                     'kraken',
                     'memory.top_7_stress',
                     'octane',
                     'page_cycler.typical_25',
                     'page_cycler_v2.typical_25',
                     'robohornet_pro',
                     'smoothness.top_25_smooth',
                     'smoothness.tough_animation_cases',
                     'smoothness.tough_canvas_cases',
                     'smoothness.tough_filters_cases',
                     'smoothness.tough_pinch_zoom_cases',
                     'smoothness.tough_scrolling_cases',
                     'smoothness.tough_webgl_cases',
                     'speedometer',
                     'sunspider',
                     'tab_switching.top_10',
                     'tab_switching.typical_25',
                     'webrtc.peerconnection',
                     'webrtc.stress']

# BLACK LIST
#  'session_restore.cold.typical_25', # profile generator not implemented on
                                      # CrOS.

class TelemetryResult(object):
    """Class to represent the results of a telemetry run.

    This class represents the results of a telemetry run, whether it ran
    successful, failed or had warnings.
    """


    def __init__(self, exit_code=0, stdout='', stderr=''):
        """Initializes this TelemetryResultObject instance.

        @param status: Status of the telemtry run.
        @param stdout: Stdout of the telemetry run.
        @param stderr: Stderr of the telemetry run.
        """
        if exit_code == 0:
            self.status = SUCCESS_STATUS
        else:
            self.status = FAILED_STATUS

        self._stdout = stdout
        self._stderr = stderr
        self.output = '\n'.join([stdout, stderr])


class TelemetryRunner(object):
    """Class responsible for telemetry for a given build.

    This class will extract and install telemetry on the devserver and is
    responsible for executing the telemetry benchmarks and returning their
    output to the caller.
    """

    def __init__(self, host, local=False, telemetry_on_dut=True):
        """Initializes this telemetry runner instance.

        If telemetry is not installed for this build, it will be.

        Basically, the following commands on the local pc on which test_that
        will be executed, depending on the 4 possible combinations of
        local x telemetry_on_dut:

        local=True, telemetry_on_dut=False:
        run_benchmark --browser=cros-chrome --remote=[dut] [test]

        local=True, telemetry_on_dut=True:
        ssh [dut] run_benchmark --browser=system [test]

        local=False, telemetry_on_dut=False:
        ssh [devserver] run_benchmark --browser=cros-chrome --remote=[dut] [test]

        local=False, telemetry_on_dut=True:
        ssh [devserver] ssh [dut] run_benchmark --browser=system [test]

        @param host: Host where the test will be run.
        @param local: If set, no devserver will be used, test will be run
                      locally.
                      If not set, "ssh [devserver] " will be appended to test
                      commands.
        @param telemetry_on_dut: If set, telemetry itself (the test harness)
                                 will run on dut.
                                 It decides browser=[system|cros-chrome]
        """
        self._host = host
        self._devserver = None
        self._telemetry_path = None
        self._telemetry_on_dut = telemetry_on_dut
        # TODO (llozano crbug.com/324964). Remove conditional code.
        # Use a class hierarchy instead.
        if local:
            self._setup_local_telemetry()
        else:
            self._setup_devserver_telemetry()

        logging.debug('Telemetry Path: %s', self._telemetry_path)


    def _setup_devserver_telemetry(self):
        """Setup Telemetry to use the devserver."""
        logging.debug('Setting up telemetry for devserver testing')
        logging.debug('Grabbing build from AFE.')
        info = self._host.host_info_store.get()
        if not info.build:
            logging.error('Unable to locate build label for host: %s.',
                          self._host.host_port)
            raise error.AutotestError('Failed to grab build for host %s.' %
                                      self._host.host_port)

        logging.debug('Setting up telemetry for build: %s', info.build)

        self._devserver = dev_server.ImageServer.resolve(
                info.build, hostname=self._host.hostname)
        self._devserver.stage_artifacts(info.build, ['autotest_packages'])
        self._telemetry_path = self._devserver.setup_telemetry(build=info.build)


    def _setup_local_telemetry(self):
        """Setup Telemetry to use local path to its sources.

        First look for chrome source root, either externally mounted, or inside
        the chroot.  Prefer chrome-src-internal source tree to chrome-src.
        """
        TELEMETRY_DIR = 'src'
        CHROME_LOCAL_SRC = '/var/cache/chromeos-cache/distfiles/target/'
        CHROME_EXTERNAL_SRC = os.path.expanduser('~/chrome_root/')

        logging.debug('Setting up telemetry for local testing')

        sources_list = ('chrome-src-internal', 'chrome-src')
        dir_list = [CHROME_EXTERNAL_SRC]
        dir_list.extend(
                [os.path.join(CHROME_LOCAL_SRC, x) for x in sources_list])
        if 'CHROME_ROOT' in os.environ:
            dir_list.insert(0, os.environ['CHROME_ROOT'])

        telemetry_src = ''
        for dir in dir_list:
            if os.path.exists(dir):
                telemetry_src = os.path.join(dir, TELEMETRY_DIR)
                break
        else:
            raise error.TestError('Telemetry source directory not found.')

        self._devserver = None
        self._telemetry_path = telemetry_src


    def _get_telemetry_cmd(self, script, test_or_benchmark, *args):
        """Build command to execute telemetry based on script and benchmark.

        @param script: Telemetry script we want to run. For example:
                       [path_to_telemetry_src]/src/tools/telemetry/run_tests.
        @param test_or_benchmark: Name of the test or benchmark we want to run,
                                  with the page_set (if required) as part of
                                  the string.
        @param args: additional list of arguments to pass to the script.

        @returns Full telemetry command to execute the script.
        """
        telemetry_cmd = []
        if self._devserver:
            devserver_hostname = self._devserver.hostname
            telemetry_cmd.extend(['ssh', devserver_hostname])

        if self._telemetry_on_dut:
            telemetry_cmd.extend(
                    [self._host.ssh_command(alive_interval=900,
                                            connection_attempts=4),
                     'python',
                     script,
                     '--verbose',
                     '--output-format=chartjson',
                     '--output-dir=%s' % DUT_CHROME_ROOT,
                     '--browser=system'])
        else:
            telemetry_cmd.extend(
                    ['python',
                     script,
                     '--verbose',
                     '--browser=cros-chrome',
                     '--output-format=chartjson',
                     '--output-dir=%s' % self._telemetry_path,
                     '--remote=%s' % self._host.host_port])
        telemetry_cmd.extend(args)
        telemetry_cmd.append(test_or_benchmark)

        return ' '.join(telemetry_cmd)


    def _scp_telemetry_results_cmd(self, perf_results_dir):
        """Build command to copy the telemetry results from the devserver.

        @param perf_results_dir: directory path where test output is to be
                                 collected.
        @returns SCP command to copy the results json to the specified directory.
        """
        if not perf_results_dir:
            return ''

        scp_cmd = ['scp']
        if self._telemetry_on_dut:
            scp_cmd.append(self._host.make_ssh_options(alive_interval=900,
                                                       connection_attempts=4))
            if not self._host.is_default_port:
                scp_cmd.append('-P %d' % self._host.port)
            src = 'root@%s:%s/results-chart.json' % (self._host.hostname,
                                                     DUT_CHROME_ROOT)
        else:
            devserver_hostname = ''
            if self._devserver:
                devserver_hostname = self._devserver.hostname + ':'
            src = '%s%s/results-chart.json' % (devserver_hostname,
                                               self._telemetry_path)

        scp_cmd.extend([src, perf_results_dir])
        return ' '.join(scp_cmd)


    def _run_cmd(self, cmd):
        """Execute an command in a external shell and capture the output.

        @param cmd: String of is a valid shell command.

        @returns The standard out, standard error and the integer exit code of
                 the executed command.
        """
        logging.debug('Running: %s', cmd)

        output = StringIO.StringIO()
        error_output = StringIO.StringIO()
        exit_code = 0
        try:
            result = utils.run(cmd, stdout_tee=output,
                               stderr_tee=error_output,
                               timeout=TELEMETRY_TIMEOUT_MINS*60)
            exit_code = result.exit_status
        except error.CmdError as e:
            logging.debug('Error occurred executing.')
            exit_code = e.result_obj.exit_status

        stdout = output.getvalue()
        stderr = error_output.getvalue()
        logging.debug('Completed with exit code: %d.\nstdout:%s\n'
                      'stderr:%s', exit_code, stdout, stderr)
        return stdout, stderr, exit_code


    def _run_telemetry(self, script, test_or_benchmark, *args):
        """Runs telemetry on a dut.

        @param script: Telemetry script we want to run. For example:
                       [path_to_telemetry_src]/src/tools/telemetry/run_tests.
        @param test_or_benchmark: Name of the test or benchmark we want to run,
                                 with the page_set (if required) as part of the
                                 string.
        @param args: additional list of arguments to pass to the script.

        @returns A TelemetryResult Instance with the results of this telemetry
                 execution.
        """
        # TODO (sbasi crbug.com/239933) add support for incognito mode.

        telemetry_cmd = self._get_telemetry_cmd(script,
                                                test_or_benchmark,
                                                *args)
        logging.debug('Running Telemetry: %s', telemetry_cmd)

        stdout, stderr, exit_code = self._run_cmd(telemetry_cmd)

        return TelemetryResult(exit_code=exit_code, stdout=stdout,
                               stderr=stderr)


    def _run_scp(self, perf_results_dir):
        """Runs telemetry on a dut.

        @param perf_results_dir: The local directory that results are being
                                 collected.
        """
        scp_cmd = self._scp_telemetry_results_cmd(perf_results_dir)
        logging.debug('Retrieving Results: %s', scp_cmd)
        _, _, exit_code = self._run_cmd(scp_cmd)
        if exit_code != 0:
            raise error.TestFail('Unable to retrieve results.')


    def _run_test(self, script, test, *args):
        """Runs a telemetry test on a dut.

        @param script: Which telemetry test script we want to run. Can be
                       telemetry's base test script or the Chrome OS specific
                       test script.
        @param test: Telemetry test we want to run.
        @param args: additional list of arguments to pass to the script.

        @returns A TelemetryResult Instance with the results of this telemetry
                 execution.
        """
        logging.debug('Running telemetry test: %s', test)
        telemetry_script = os.path.join(self._telemetry_path, script)
        result = self._run_telemetry(telemetry_script, test, *args)
        if result.status is FAILED_STATUS:
            raise error.TestFail('Telemetry test %s failed.' % test)
        return result


    def run_telemetry_test(self, test, *args):
        """Runs a telemetry test on a dut.

        @param test: Telemetry test we want to run.
        @param args: additional list of arguments to pass to the telemetry
                     execution script.

        @returns A TelemetryResult Instance with the results of this telemetry
                 execution.
        """
        return self._run_test(TELEMETRY_RUN_TESTS_SCRIPT, test, *args)


    def run_telemetry_benchmark(self, benchmark, perf_value_writer=None,
                                *args):
        """Runs a telemetry benchmark on a dut.

        @param benchmark: Benchmark we want to run.
        @param perf_value_writer: Should be an instance with the function
                                  output_perf_value(), if None, no perf value
                                  will be written. Typically this will be the
                                  job object from an autotest test.
        @param args: additional list of arguments to pass to the telemetry
                     execution script.

        @returns A TelemetryResult Instance with the results of this telemetry
                 execution.
        """
        logging.debug('Running telemetry benchmark: %s', benchmark)

        if benchmark not in ON_DUT_WHITE_LIST:
            self._telemetry_on_dut = False

        if self._telemetry_on_dut:
            telemetry_script = os.path.join(DUT_CHROME_ROOT,
                                            TELEMETRY_RUN_BENCHMARKS_SCRIPT)
            self._ensure_deps(self._host, benchmark)
        else:
            telemetry_script = os.path.join(self._telemetry_path,
                                            TELEMETRY_RUN_BENCHMARKS_SCRIPT)

        result = self._run_telemetry(telemetry_script, benchmark, *args)

        if result.status is WARNING_STATUS:
            raise error.TestWarn('Telemetry Benchmark: %s'
                                 ' exited with Warnings.' % benchmark)
        if result.status is FAILED_STATUS:
            raise error.TestFail('Telemetry Benchmark: %s'
                                 ' failed to run.' % benchmark)
        if perf_value_writer:
            self._run_scp(perf_value_writer.resultsdir)
        return result


    def run_gpu_integration_test(self, test, *args):
        """Runs a gpu test on a dut.

        @param test: Gpu test we want to run.
        @param args: additional list of arguments to pass to the telemetry
                     execution script.

         @returns A TelemetryResult instance with the results of this telemetry
                  execution.
        """
        script = os.path.join(DUT_CHROME_ROOT,
                              TELEMETRY_RUN_GPU_TESTS_SCRIPT)
        cmd = []
        if self._devserver:
            devserver_hostname = self._devserver.hostname
            cmd.extend(['ssh', devserver_hostname])

        cmd.extend(
            [self._host.ssh_command(alive_interval=900, connection_attempts=4),
             'python', script])
        cmd.extend(args)
        cmd.append(test)
        cmd = ' '.join(cmd)
        stdout, stderr, exit_code = self._run_cmd(cmd)

        return TelemetryResult(exit_code=exit_code, stdout=stdout,
                               stderr=stderr)


    def _ensure_deps(self, dut, test_name):
        """
        Ensure the dependencies are locally available on DUT.

        @param dut: The autotest host object representing DUT.
        @param test_name: Name of the telemetry test.
        """
        # Get DEPs using host's telemetry.
        format_string = ('python %s/tools/perf/fetch_benchmark_deps.py %s')
        command = format_string % (self._telemetry_path, test_name)
        stdout = StringIO.StringIO()
        stderr = StringIO.StringIO()

        if self._devserver:
            devserver_hostname = self._devserver.url().split(
                    'http://')[1].split(':')[0]
            command = 'ssh %s %s' % (devserver_hostname, command)

        logging.info('Getting DEPs: %s', command)
        try:
            result = utils.run(command, stdout_tee=stdout,
                               stderr_tee=stderr)
        except error.CmdError as e:
            logging.debug('Error occurred getting DEPs: %s\n %s\n',
                          stdout.getvalue(), stderr.getvalue())
            raise error.TestFail('Error occurred while getting DEPs.')

        # Download DEPs to DUT.
        # send_file() relies on rsync over ssh. Couldn't be better.
        stdout_str = stdout.getvalue()
        stdout.close()
        stderr.close()
        for dep in stdout_str.split():
            src = os.path.join(self._telemetry_path, dep)
            dst = os.path.join(DUT_CHROME_ROOT, dep)
            if self._devserver:
                logging.info('Copying: %s -> %s', src, dst)
                rsync_cmd = utils.sh_escape('rsync %s %s %s:%s' %
                                            (self._host.rsync_options(), src,
                                            self._host.hostname, dst))
                utils.run('ssh %s "%s"' % (devserver_hostname, rsync_cmd))
            else:
                if not os.path.isfile(src):
                    raise error.TestFail('Error occurred while saving DEPs.')
                logging.info('Copying: %s -> %s', src, dst)
                dut.send_file(src, dst)
