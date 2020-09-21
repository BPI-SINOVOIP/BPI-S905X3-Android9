#!/usr/bin/python -u
# Copyright 2007-2008 Martin J. Bligh <mbligh@google.com>, Google Inc.
# Released under the GPL v2

"""
Run a control file through the server side engine
"""

import datetime
import contextlib
import getpass
import logging
import os
import re
import signal
import socket
import sys
import traceback
import time
import urllib2


import common
from autotest_lib.client.bin.result_tools import utils as result_utils
from autotest_lib.client.bin.result_tools import view as result_view
from autotest_lib.client.common_lib import control_data
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.server import results_mocker

try:
    from chromite.lib import metrics
    from chromite.lib import cloud_trace
except ImportError:
    from autotest_lib.client.common_lib import utils as common_utils
    metrics = common_utils.metrics_mock
    import mock
    cloud_trace = mock.MagicMock()

_CONFIG = global_config.global_config


# Number of seconds to wait before returning if testing mode is enabled
TESTING_MODE_SLEEP_SECS = 1


from autotest_lib.server import frontend
from autotest_lib.server import server_logging_config
from autotest_lib.server import server_job, utils, autoserv_parser, autotest
from autotest_lib.server import utils as server_utils
from autotest_lib.server import site_utils
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.site_utils import job_directories
from autotest_lib.site_utils import job_overhead
from autotest_lib.site_utils import lxc
from autotest_lib.site_utils.lxc import utils as lxc_utils
from autotest_lib.client.common_lib import pidfile, logging_manager


# Control segment to stage server-side package.
STAGE_SERVER_SIDE_PACKAGE_CONTROL_FILE = server_job._control_segment_path(
        'stage_server_side_package')

# Command line to start servod in a moblab.
START_SERVOD_CMD = 'sudo start servod BOARD=%s PORT=%s'
STOP_SERVOD_CMD = 'sudo stop servod'

def log_alarm(signum, frame):
    logging.error("Received SIGALARM. Ignoring and continuing on.")
    sys.exit(1)


def _get_machines(parser):
    """Get a list of machine names from command line arg -m or a file.

    @param parser: Parser for the command line arguments.

    @return: A list of machine names from command line arg -m or the
             machines file specified in the command line arg -M.
    """
    if parser.options.machines:
        machines = parser.options.machines.replace(',', ' ').strip().split()
    else:
        machines = []
    machines_file = parser.options.machines_file
    if machines_file:
        machines = []
        for m in open(machines_file, 'r').readlines():
            # remove comments, spaces
            m = re.sub('#.*', '', m).strip()
            if m:
                machines.append(m)
        logging.debug('Read list of machines from file: %s', machines_file)
        logging.debug('Machines: %s', ','.join(machines))

    if machines:
        for machine in machines:
            if not machine or re.search('\s', machine):
                parser.parser.error("Invalid machine: %s" % str(machine))
        machines = list(set(machines))
        machines.sort()
    return machines


def _stage_ssp(parser, resultsdir):
    """Stage server-side package.

    This function calls a control segment to stage server-side package based on
    the job and autoserv command line option. The detail implementation could
    be different for each host type. Currently, only CrosHost has
    stage_server_side_package function defined.
    The script returns None if no server-side package is available. However,
    it may raise exception if it failed for reasons other than artifact (the
    server-side package) not found.

    @param parser: Command line arguments parser passed in the autoserv process.
    @param resultsdir: Folder to store results. This could be different from
            parser.options.results: parser.options.results  can be set to None
            for results to be stored in a temp folder. resultsdir can be None
            for autoserv run requires no logging.

    @return: (ssp_url, error_msg), where
              ssp_url is a url to the autotest server-side package. None if
              server-side package is not supported.
              error_msg is a string indicating the failures. None if server-
              side package is staged successfully.
    """
    machines_list = _get_machines(parser)
    machines_list = server_job.get_machine_dicts(machines_list, resultsdir,
                                                 parser.options.lab,
                                                 parser.options.host_attributes)

    # If test_source_build is not specified, default to use server-side test
    # code from build specified in --image.
    namespace = {'machines': machines_list,
                 'image': (parser.options.test_source_build or
                           parser.options.image),}
    script_locals = {}
    execfile(STAGE_SERVER_SIDE_PACKAGE_CONTROL_FILE, namespace, script_locals)
    return script_locals['ssp_url'], script_locals['error_msg']


def _run_with_ssp(job, container_id, job_id, results, parser, ssp_url,
                  job_folder, machines):
    """Run the server job with server-side packaging.

    @param job: The server job object.
    @param container_id: ID of the container to run the test.
    @param job_id: ID of the test job.
    @param results: Folder to store results. This could be different from
                    parser.options.results:
                    parser.options.results  can be set to None for results to be
                    stored in a temp folder.
                    results can be None for autoserv run requires no logging.
    @param parser: Command line parser that contains the options.
    @param ssp_url: url of the staged server-side package.
    @param job_folder: Name of the job result folder.
    @param machines: A list of machines to run the test.
    """
    bucket = lxc.ContainerBucket()
    control = (parser.args[0] if len(parser.args) > 0 and parser.args[0] != ''
               else None)
    try:
        dut_name = machines[0] if len(machines) >= 1 else None
        test_container = bucket.setup_test(container_id, job_id, ssp_url,
                                           results, control=control,
                                           job_folder=job_folder,
                                           dut_name=dut_name)
    except Exception as e:
        job.record('FAIL', None, None,
                   'Failed to setup container for test: %s. Check logs in '
                   'ssp_logs folder for more details.' % e)
        raise

    args = sys.argv[:]
    args.remove('--require-ssp')
    # --parent_job_id is only useful in autoserv running in host, not in
    # container. Include this argument will cause test to fail for builds before
    # CL 286265 was merged.
    if '--parent_job_id' in args:
        index = args.index('--parent_job_id')
        args.remove('--parent_job_id')
        # Remove the actual parent job id in command line arg.
        del args[index]

    # A dictionary of paths to replace in the command line. Key is the path to
    # be replaced with the one in value.
    paths_to_replace = {}
    # Replace the control file path with the one in container.
    if control:
        container_control_filename = os.path.join(
                lxc.CONTROL_TEMP_PATH, os.path.basename(control))
        paths_to_replace[control] = container_control_filename
    # Update result directory with the one in container.
    container_result_dir = os.path.join(lxc.RESULT_DIR_FMT % job_folder)
    if parser.options.results:
        paths_to_replace[parser.options.results] = container_result_dir
    # Update parse_job directory with the one in container. The assumption is
    # that the result folder to be parsed is always the same as the results_dir.
    if parser.options.parse_job:
        paths_to_replace[parser.options.parse_job] = container_result_dir

    args = [paths_to_replace.get(arg, arg) for arg in args]

    # Apply --use-existing-results, results directory is aready created and
    # mounted in container. Apply this arg to avoid exception being raised.
    if not '--use-existing-results' in args:
        args.append('--use-existing-results')

    # Make sure autoserv running in container using a different pid file.
    if not '--pidfile-label' in args:
        args.extend(['--pidfile-label', 'container_autoserv'])

    cmd_line = ' '.join(["'%s'" % arg if ' ' in arg else arg for arg in args])
    logging.info('Run command in container: %s', cmd_line)
    success = False
    try:
        test_container.attach_run(cmd_line)
        success = True
    except Exception as e:
        # If the test run inside container fails without generating any log,
        # write a message to status.log to help troubleshooting.
        debug_files = os.listdir(os.path.join(results, 'debug'))
        if not debug_files:
            job.record('FAIL', None, None,
                       'Failed to run test inside the container: %s. Check '
                       'logs in ssp_logs folder for more details.' % e)
        raise
    finally:
        metrics.Counter(
            'chromeos/autotest/experimental/execute_job_in_ssp').increment(
                fields={'success': success})
        test_container.destroy()


def correct_results_folder_permission(results):
    """Make sure the results folder has the right permission settings.

    For tests running with server-side packaging, the results folder has the
    owner of root. This must be changed to the user running the autoserv
    process, so parsing job can access the results folder.
    TODO(dshi): crbug.com/459344 Remove this function when test container can be
    unprivileged container.

    @param results: Path to the results folder.

    """
    if not results:
        return

    utils.run('sudo -n chown -R %s "%s"' % (os.getuid(), results))
    utils.run('sudo -n chgrp -R %s "%s"' % (os.getgid(), results))


def _start_servod(machine):
    """Try to start servod in moblab if it's not already running or running with
    different board or port.

    @param machine: Name of the dut used for test.
    """
    if not utils.is_moblab():
        return

    logging.debug('Trying to start servod.')
    try:
        afe = frontend.AFE()
        board = server_utils.get_board_from_afe(machine, afe)
        hosts = afe.get_hosts(hostname=machine)
        servo_host = hosts[0].attributes.get('servo_host', None)
        servo_port = hosts[0].attributes.get('servo_port', 9999)
        if not servo_host in ['localhost', '127.0.0.1']:
            logging.warn('Starting servod is aborted. The dut\'s servo_host '
                         'attribute is not set to localhost.')
            return
    except (urllib2.HTTPError, urllib2.URLError):
        # Ignore error if RPC failed to get board
        logging.error('Failed to get board name from AFE. Start servod is '
                      'aborted')
        return

    try:
        pid = utils.run('pgrep servod').stdout
        cmd_line = utils.run('ps -fp %s' % pid).stdout
        if ('--board %s' % board in cmd_line and
            '--port %s' % servo_port in cmd_line):
            logging.debug('Servod is already running with given board and port.'
                          ' There is no need to restart servod.')
            return
        logging.debug('Servod is running with different board or port. '
                      'Stopping existing servod.')
        utils.run('sudo stop servod')
    except error.CmdError:
        # servod is not running.
        pass

    try:
        utils.run(START_SERVOD_CMD % (board, servo_port))
        logging.debug('Servod is started')
    except error.CmdError as e:
        logging.error('Servod failed to be started, error: %s', e)


def run_autoserv(pid_file_manager, results, parser, ssp_url, use_ssp):
    """Run server job with given options.

    @param pid_file_manager: PidFileManager used to monitor the autoserv process
    @param results: Folder to store results.
    @param parser: Parser for the command line arguments.
    @param ssp_url: Url to server-side package.
    @param use_ssp: Set to True to run with server-side packaging.
    """
    if parser.options.warn_no_ssp:
        # Post a warning in the log.
        logging.warn('Autoserv is required to run with server-side packaging. '
                     'However, no drone is found to support server-side '
                     'packaging. The test will be executed in a drone without '
                     'server-side packaging supported.')

    # send stdin to /dev/null
    dev_null = os.open(os.devnull, os.O_RDONLY)
    os.dup2(dev_null, sys.stdin.fileno())
    os.close(dev_null)

    # Create separate process group if the process is not a process group
    # leader. This allows autoserv process to keep running after the caller
    # process (drone manager call) exits.
    if os.getpid() != os.getpgid(0):
        os.setsid()

    # Container name is predefined so the container can be destroyed in
    # handle_sigterm.
    job_or_task_id = job_directories.get_job_id_or_task_id(
            parser.options.results)
    container_id = lxc.ContainerId(job_or_task_id, time.time(), os.getpid())
    job_folder = job_directories.get_job_folder_name(parser.options.results)

    # Implement SIGTERM handler
    def handle_sigterm(signum, frame):
        logging.debug('Received SIGTERM')
        if pid_file_manager:
            pid_file_manager.close_file(1, signal.SIGTERM)
        logging.debug('Finished writing to pid_file. Killing process.')

        # Update results folder's file permission. This needs to be done ASAP
        # before the parsing process tries to access the log.
        if use_ssp and results:
            correct_results_folder_permission(results)

        # TODO (sbasi) - remove the time.sleep when crbug.com/302815 is solved.
        # This sleep allows the pending output to be logged before the kill
        # signal is sent.
        time.sleep(.1)
        if use_ssp:
            logging.debug('Destroy container %s before aborting the autoserv '
                          'process.', container_id)
            try:
                bucket = lxc.ContainerBucket()
                container = bucket.get_container(container_id)
                if container:
                    container.destroy()
                else:
                    logging.debug('Container %s is not found.', container_id)
            except:
                # Handle any exception so the autoserv process can be aborted.
                logging.exception('Failed to destroy container %s.',
                                  container_id)
            # Try to correct the result file permission again after the
            # container is destroyed, as the container might have created some
            # new files in the result folder.
            if results:
                correct_results_folder_permission(results)

        os.killpg(os.getpgrp(), signal.SIGKILL)

    # Set signal handler
    signal.signal(signal.SIGTERM, handle_sigterm)

    # faulthandler is only needed to debug in the Lab and is not avaliable to
    # be imported in the chroot as part of VMTest, so Try-Except it.
    try:
        import faulthandler
        faulthandler.register(signal.SIGTERM, all_threads=True, chain=True)
        logging.debug('faulthandler registered on SIGTERM.')
    except ImportError:
        sys.exc_clear()

    # Ignore SIGTTOU's generated by output from forked children.
    signal.signal(signal.SIGTTOU, signal.SIG_IGN)

    # If we received a SIGALARM, let's be loud about it.
    signal.signal(signal.SIGALRM, log_alarm)

    # Server side tests that call shell scripts often depend on $USER being set
    # but depending on how you launch your autotest scheduler it may not be set.
    os.environ['USER'] = getpass.getuser()

    label = parser.options.label
    group_name = parser.options.group_name
    user = parser.options.user
    client = parser.options.client
    server = parser.options.server
    install_before = parser.options.install_before
    install_after = parser.options.install_after
    verify = parser.options.verify
    repair = parser.options.repair
    cleanup = parser.options.cleanup
    provision = parser.options.provision
    reset = parser.options.reset
    job_labels = parser.options.job_labels
    no_tee = parser.options.no_tee
    parse_job = parser.options.parse_job
    execution_tag = parser.options.execution_tag
    if not execution_tag:
        execution_tag = parse_job
    ssh_user = parser.options.ssh_user
    ssh_port = parser.options.ssh_port
    ssh_pass = parser.options.ssh_pass
    collect_crashinfo = parser.options.collect_crashinfo
    control_filename = parser.options.control_filename
    test_retry = parser.options.test_retry
    verify_job_repo_url = parser.options.verify_job_repo_url
    skip_crash_collection = parser.options.skip_crash_collection
    ssh_verbosity = int(parser.options.ssh_verbosity)
    ssh_options = parser.options.ssh_options
    no_use_packaging = parser.options.no_use_packaging
    host_attributes = parser.options.host_attributes
    in_lab = bool(parser.options.lab)

    # can't be both a client and a server side test
    if client and server:
        parser.parser.error("Can not specify a test as both server and client!")

    if provision and client:
        parser.parser.error("Cannot specify provisioning and client!")

    is_special_task = (verify or repair or cleanup or collect_crashinfo or
                       provision or reset)
    if len(parser.args) < 1 and not is_special_task:
        parser.parser.error("Missing argument: control file")

    if ssh_verbosity > 0:
        # ssh_verbosity is an integer between 0 and 3, inclusive
        ssh_verbosity_flag = '-' + 'v' * ssh_verbosity
    else:
        ssh_verbosity_flag = ''

    # We have a control file unless it's just a verify/repair/cleanup job
    if len(parser.args) > 0:
        control = parser.args[0]
    else:
        control = None

    machines = _get_machines(parser)
    if group_name and len(machines) < 2:
        parser.parser.error('-G %r may only be supplied with more than one '
                            'machine.' % group_name)

    kwargs = {'group_name': group_name, 'tag': execution_tag,
              'disable_sysinfo': parser.options.disable_sysinfo}
    if parser.options.parent_job_id:
        kwargs['parent_job_id'] = int(parser.options.parent_job_id)
    if control_filename:
        kwargs['control_filename'] = control_filename
    if host_attributes:
        kwargs['host_attributes'] = host_attributes
    kwargs['in_lab'] = in_lab
    job = server_job.server_job(control, parser.args[1:], results, label,
                                user, machines, client, parse_job,
                                ssh_user, ssh_port, ssh_pass,
                                ssh_verbosity_flag, ssh_options,
                                test_retry, **kwargs)

    job.logging.start_logging()
    job.init_parser()

    # perform checks
    job.precheck()

    # run the job
    exit_code = 0
    auto_start_servod = _CONFIG.get_config_value(
            'AUTOSERV', 'auto_start_servod', type=bool, default=False)

    site_utils.SetupTsMonGlobalState('autoserv', indirect=False,
                                     auto_flush=False, short_lived=True)
    try:
        try:
            if repair:
                if auto_start_servod and len(machines) == 1:
                    _start_servod(machines[0])
                job.repair(job_labels)
            elif verify:
                job.verify(job_labels)
            elif provision:
                job.provision(job_labels)
            elif reset:
                job.reset(job_labels)
            elif cleanup:
                job.cleanup(job_labels)
            else:
                if auto_start_servod and len(machines) == 1:
                    _start_servod(machines[0])
                if use_ssp:
                    try:
                        _run_with_ssp(job, container_id, job_or_task_id,
                                        results, parser, ssp_url, job_folder,
                                        machines)
                    finally:
                        # Update the ownership of files in result folder.
                        correct_results_folder_permission(results)
                else:
                    if collect_crashinfo:
                        # Update the ownership of files in result folder. If the
                        # job to collect crashinfo was running inside container
                        # (SSP) and crashed before correcting folder permission,
                        # the result folder might have wrong permission setting.
                        try:
                            correct_results_folder_permission(results)
                        except:
                            # Ignore any error as the user may not have root
                            # permission to run sudo command.
                            pass
                    metric_name = ('chromeos/autotest/experimental/'
                                   'autoserv_job_run_duration')
                    f = {'in_container': utils.is_in_container(),
                         'success': False}
                    with metrics.SecondsTimer(metric_name, fields=f) as c:
                        job.run(install_before, install_after,
                                verify_job_repo_url=verify_job_repo_url,
                                only_collect_crashinfo=collect_crashinfo,
                                skip_crash_collection=skip_crash_collection,
                                job_labels=job_labels,
                                use_packaging=(not no_use_packaging))
                        c['success'] = True

        finally:
            job.close()
            # Special task doesn't run parse, so result summary needs to be
            # built here.
            if results and (repair or verify or reset or cleanup or provision):
                # Throttle the result on the server side.
                try:
                    result_utils.execute(
                            results, control_data.DEFAULT_MAX_RESULT_SIZE_KB)
                except:
                    logging.exception(
                            'Non-critical failure: Failed to throttle results '
                            'in directory %s.', results)
                # Build result view and report metrics for result sizes.
                site_utils.collect_result_sizes(results)
    except:
        exit_code = 1
        traceback.print_exc()
    finally:
        metrics.Flush()

    if pid_file_manager:
        pid_file_manager.num_tests_failed = job.num_tests_failed
        pid_file_manager.close_file(exit_code)
    job.cleanup_parser()

    sys.exit(exit_code)


def record_autoserv(options, start_time):
    """Record autoserv end-to-end time in metadata db.

    @param options: parser options.
    @param start_time: When autoserv started
    """
    # Get machine hostname
    machines = options.machines.replace(
            ',', ' ').strip().split() if options.machines else []
    num_machines = len(machines)
    if num_machines > 1:
        # Skip the case where atomic group is used.
        return
    elif num_machines == 0:
        machines.append('hostless')

    # Determine the status that will be reported.
    status = get_job_status(options)
    is_special_task = status not in [
            job_overhead.STATUS.RUNNING, job_overhead.STATUS.GATHERING]
    job_or_task_id = job_directories.get_job_id_or_task_id(options.results)
    duration_secs = (datetime.datetime.now() - start_time).total_seconds()
    job_overhead.record_state_duration(
            job_or_task_id, machines[0], status, duration_secs,
            is_special_task=is_special_task)


def get_job_status(options):
    """Returns the HQE Status for this run.

    @param options: parser options.
    """
    s = job_overhead.STATUS
    task_mapping = {
            'reset': s.RESETTING, 'verify': s.VERIFYING,
            'provision': s.PROVISIONING, 'repair': s.REPAIRING,
            'cleanup': s.CLEANING, 'collect_crashinfo': s.GATHERING}
    match = [task for task in task_mapping if getattr(options, task, False)]
    return task_mapping[match[0]] if match else s.RUNNING


def main():
    start_time = datetime.datetime.now()
    # grab the parser
    parser = autoserv_parser.autoserv_parser
    parser.parse_args()

    if len(sys.argv) == 1:
        parser.parser.print_help()
        sys.exit(1)

    if parser.options.no_logging:
        results = None
    else:
        results = parser.options.results
        if not results:
            results = 'results.' + time.strftime('%Y-%m-%d-%H.%M.%S')
        results = os.path.abspath(results)
        resultdir_exists = False
        for filename in ('control.srv', 'status.log', '.autoserv_execute'):
            if os.path.exists(os.path.join(results, filename)):
                resultdir_exists = True
        if not parser.options.use_existing_results and resultdir_exists:
            error = "Error: results directory already exists: %s\n" % results
            sys.stderr.write(error)
            sys.exit(1)

        # Now that we certified that there's no leftover results dir from
        # previous jobs, lets create the result dir since the logging system
        # needs to create the log file in there.
        if not os.path.isdir(results):
            os.makedirs(results)

    # If the job requires to run with server-side package, try to stage server-
    # side package first. If that fails with error that autotest server package
    # does not exist, fall back to run the job without using server-side
    # packaging. If option warn_no_ssp is specified, that means autoserv is
    # running in a drone does not support SSP, thus no need to stage server-side
    # package.
    ssp_url = None
    ssp_url_warning = False
    if (not parser.options.warn_no_ssp and parser.options.require_ssp):
        ssp_url, ssp_error_msg = _stage_ssp(parser, results)
        # The build does not have autotest server package. Fall back to not
        # to use server-side package. Logging is postponed until logging being
        # set up.
        ssp_url_warning = not ssp_url

    # Server-side packaging will only be used if it's required and the package
    # is available. If warn_no_ssp is specified, it means that autoserv is
    # running in a drone does not have SSP supported and a warning will be logs.
    # Therefore, it should not run with SSP.
    use_ssp = (not parser.options.warn_no_ssp and parser.options.require_ssp
               and ssp_url)
    if use_ssp:
        log_dir = os.path.join(results, 'ssp_logs') if results else None
        if log_dir and not os.path.exists(log_dir):
            os.makedirs(log_dir)
    else:
        log_dir = results

    logging_manager.configure_logging(
            server_logging_config.ServerLoggingConfig(),
            results_dir=log_dir,
            use_console=not parser.options.no_tee,
            verbose=parser.options.verbose,
            no_console_prefix=parser.options.no_console_prefix)

    if ssp_url_warning:
        logging.warn(
                'Autoserv is required to run with server-side packaging. '
                'However, no server-side package can be staged based on '
                '`--image`, host attribute job_repo_url or host OS version '
                'label. It could be that the build to test is older than the '
                'minimum version that supports server-side packaging, or no '
                'devserver can be found to stage server-side package. The test '
                'will be executed without using erver-side packaging. '
                'Following is the detailed error:\n%s', ssp_error_msg)

    if results:
        logging.info("Results placed in %s" % results)

        # wait until now to perform this check, so it get properly logged
        if (parser.options.use_existing_results and not resultdir_exists and
            not utils.is_in_container()):
            logging.error("No existing results directory found: %s", results)
            sys.exit(1)

    logging.debug('autoserv is running in drone %s.', socket.gethostname())
    logging.debug('autoserv command was: %s', ' '.join(sys.argv))

    if parser.options.write_pidfile and results:
        pid_file_manager = pidfile.PidFileManager(parser.options.pidfile_label,
                                                  results)
        pid_file_manager.open_file()
    else:
        pid_file_manager = None

    autotest.Autotest.set_install_in_tmpdir(
        parser.options.install_in_tmpdir)

    exit_code = 0
    # TODO(beeps): Extend this to cover different failure modes.
    # Testing exceptions are matched against labels sent to autoserv. Eg,
    # to allow only the hostless job to run, specify
    # testing_exceptions: test_suite in the shadow_config. To allow both
    # the hostless job and dummy_Pass to run, specify
    # testing_exceptions: test_suite,dummy_Pass. You can figure out
    # what label autoserv is invoked with by looking through the logs of a test
    # for the autoserv command's -l option.
    testing_exceptions = _CONFIG.get_config_value(
            'AUTOSERV', 'testing_exceptions', type=list, default=[])
    test_mode = _CONFIG.get_config_value(
            'AUTOSERV', 'testing_mode', type=bool, default=False)
    test_mode = (results_mocker and test_mode and not
                 any([ex in parser.options.label
                      for ex in testing_exceptions]))
    is_task = (parser.options.verify or parser.options.repair or
               parser.options.provision or parser.options.reset or
               parser.options.cleanup or parser.options.collect_crashinfo)

    trace_labels = {
            'job_id': job_directories.get_job_id_or_task_id(
                    parser.options.results)
    }
    trace = cloud_trace.SpanStack(
            labels=trace_labels,
            global_context=parser.options.cloud_trace_context)
    trace.enabled = parser.options.cloud_trace_context_enabled == 'True'
    try:
        try:
            if test_mode:
                # The parser doesn't run on tasks anyway, so we can just return
                # happy signals without faking results.
                if not is_task:
                    machine = parser.options.results.split('/')[-1]

                    # TODO(beeps): The proper way to do this would be to
                    # refactor job creation so we can invoke job.record
                    # directly. To do that one needs to pipe the test_name
                    # through run_autoserv and bail just before invoking
                    # the server job. See the comment in
                    # puppylab/results_mocker for more context.
                    results_mocker.ResultsMocker(
                            'unknown-test', parser.options.results, machine
                            ).mock_results()
                return
            else:
                with trace.Span(get_job_status(parser.options)):
                    run_autoserv(pid_file_manager, results, parser, ssp_url,
                                 use_ssp)
        except SystemExit as e:
            exit_code = e.code
            if exit_code:
                logging.exception('Uncaught SystemExit with code %s', exit_code)
        except Exception:
            # If we don't know what happened, we'll classify it as
            # an 'abort' and return 1.
            logging.exception('Uncaught Exception, exit_code = 1.')
            exit_code = 1
    finally:
        if pid_file_manager:
            pid_file_manager.close_file(exit_code)
        # Record the autoserv duration time. Must be called
        # just before the system exits to ensure accuracy.
        record_autoserv(parser.options, start_time)
    sys.exit(exit_code)


if __name__ == '__main__':
    main()
