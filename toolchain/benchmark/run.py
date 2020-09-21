#!/usr/bin/env python2
#
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# pylint: disable=cros-logging-import

# This is the script to run specified benchmark with different toolchain
# settings. It includes the process of building benchmark locally and running
# benchmark on DUT.

"""Main script to run the benchmark suite from building to testing."""
from __future__ import print_function

import argparse
import config
import ConfigParser
import logging
import os
import subprocess
import sys

logging.basicConfig(level=logging.INFO)

def _parse_arguments(argv):
    parser = argparse.ArgumentParser(description='Build and run specific '
                                     'benchamrk')
    parser.add_argument(
        '-b',
        '--bench',
        action='append',
        default=[],
        help='Select which benchmark to run')

    # Only one of compiler directory and llvm prebuilts version can be indicated
    # at the beginning, so set -c and -l into a exclusive group.
    group = parser.add_mutually_exclusive_group()

    # The toolchain setting arguments has action of 'append', so that users
    # could compare performance with several toolchain settings together.
    group.add_argument(
        '-c',
        '--compiler_dir',
        metavar='DIR',
        action='append',
        default=[],
        help='Specify path to the compiler\'s bin directory. '
        'You shall give several paths, each with a -c, to '
        'compare performance differences in '
        'each compiler.')

    parser.add_argument(
        '-o',
        '--build_os',
        action='append',
        default=[],
        help='Specify the host OS to build the benchmark.')

    group.add_argument(
        '-l',
        '--llvm_prebuilts_version',
        action='append',
        default=[],
        help='Specify the version of prebuilt LLVM. When '
        'specific prebuilt version of LLVM already '
        'exists, no need to pass the path to compiler '
        'directory.')

    parser.add_argument(
        '-f',
        '--cflags',
        action='append',
        default=[],
        help='Specify the cflags options for the toolchain. '
        'Be sure to quote all the cflags with quotation '
        'mark("") or use equal(=).')
    parser.add_argument(
        '--ldflags',
        action='append',
        default=[],
        help='Specify linker flags for the toolchain.')

    parser.add_argument(
        '-i',
        '--iterations',
        type=int,
        default=1,
        help='Specify how many iterations does the test '
        'take.')

    # Arguments -s and -r are for connecting to DUT.
    parser.add_argument(
        '-s',
        '--serials',
        help='Comma separate list of device serials under '
        'test.')

    parser.add_argument(
        '-r',
        '--remote',
        default='localhost',
        help='hostname[:port] if the ADB device is connected '
        'to a remote machine. Ensure this workstation '
        'is configured for passwordless ssh access as '
        'users "root" or "adb"')

    # Arguments -frequency and -m are for device settings
    parser.add_argument(
        '--frequency',
        type=int,
        default=979200,
        help='Specify the CPU frequency of the device. The '
        'unit is KHZ. The available value is defined in'
        'cpufreq/scaling_available_frequency file in '
        'device\'s each core directory. '
        'The default value is 979200, which shows a '
        'balance in noise and performance. Lower '
        'frequency will slow down the performance but '
        'reduce noise.')

    parser.add_argument(
        '-m',
        '--mode',
        default='little',
        help='User can specify whether \'little\' or \'big\' '
        'mode to use. The default one is little mode. '
        'The little mode runs on a single core of '
        'Cortex-A53, while big mode runs on single core '
        'of Cortex-A57.')

    # Configure file for benchmark test
    parser.add_argument(
        '-t',
        '--test',
        help='Specify the test settings with configuration '
        'file.')

    # Whether to keep old json result or not
    parser.add_argument(
        '-k',
        '--keep',
        default='False',
        help='User can specify whether to keep the old json '
        'results from last run. This can be useful if you '
        'want to compare performance differences in two or '
        'more different runs. Default is False(off).')

    return parser.parse_args(argv)


# Clear old log files in bench suite directory
def clear_logs():
    logging.info('Removing old logfiles...')
    for f in ['build_log', 'device_log', 'test_log']:
        logfile = os.path.join(config.bench_suite_dir, f)
        try:
            os.remove(logfile)
        except OSError:
            logging.info('No logfile %s need to be removed. Ignored.', f)
    logging.info('Old logfiles been removed.')


# Clear old json files in bench suite directory
def clear_results():
    logging.info('Clearing old json results...')
    for bench in config.bench_list:
        result = os.path.join(config.bench_suite_dir, bench + '.json')
        try:
            os.remove(result)
        except OSError:
            logging.info('no %s json file need to be removed. Ignored.', bench)
    logging.info('Old json results been removed.')


# Use subprocess.check_call to run other script, and put logs to files
def check_call_with_log(cmd, log_file):
    log_file = os.path.join(config.bench_suite_dir, log_file)
    with open(log_file, 'a') as logfile:
        log_header = 'Log for command: %s\n' % (cmd)
        logfile.write(log_header)
        try:
            subprocess.check_call(cmd, stdout=logfile)
        except subprocess.CalledProcessError:
            logging.error('Error running %s, please check %s for more info.',
                          cmd, log_file)
            raise
    logging.info('Logs for %s are written to %s.', cmd, log_file)


def set_device(serials, remote, frequency):
    setting_cmd = [
        os.path.join(
            os.path.join(config.android_home, config.autotest_dir),
            'site_utils/set_device.py')
    ]
    setting_cmd.append('-r=' + remote)
    setting_cmd.append('-q=' + str(frequency))

    # Deal with serials.
    # If there is no serails specified, try to run test on the only device.
    # If specified, split the serials into a list and run test on each device.
    if serials:
        for serial in serials.split(','):
            setting_cmd.append('-s=' + serial)
            check_call_with_log(setting_cmd, 'device_log')
            setting_cmd.pop()
    else:
        check_call_with_log(setting_cmd, 'device_log')

    logging.info('CPU mode and frequency set successfully!')


def log_ambiguous_args():
    logging.error('The count of arguments does not match!')
    raise ValueError('The count of arguments does not match.')


# Check if the count of building arguments are log_ambiguous or not.  The
# number of -c/-l, -f, and -os should be either all 0s or all the same.
def check_count(compiler, llvm_version, build_os, cflags, ldflags):
    # Count will be set to 0 if no compiler or llvm_version specified.
    # Otherwise, one of these two args length should be 0 and count will be
    # the other one.
    count = max(len(compiler), len(llvm_version))

    # Check if number of cflags is 0 or the same with before.
    if len(cflags) != 0:
        if count != 0 and len(cflags) != count:
            log_ambiguous_args()
        count = len(cflags)

    if len(ldflags) != 0:
        if count != 0 and len(ldflags) != count:
            log_ambiguous_args()
        count = len(ldflags)

    if len(build_os) != 0:
        if count != 0 and len(build_os) != count:
            log_ambiguous_args()
        count = len(build_os)

    # If no settings are passed, only run default once.
    return max(1, count)


# Build benchmark binary with toolchain settings
def build_bench(setting_no, bench, compiler, llvm_version, build_os, cflags,
                ldflags):
    # Build benchmark locally
    build_cmd = ['./build_bench.py', '-b=' + bench]
    if compiler:
        build_cmd.append('-c=' + compiler[setting_no])
    if llvm_version:
        build_cmd.append('-l=' + llvm_version[setting_no])
    if build_os:
        build_cmd.append('-o=' + build_os[setting_no])
    if cflags:
        build_cmd.append('-f=' + cflags[setting_no])
    if ldflags:
        build_cmd.append('--ldflags=' + ldflags[setting_no])

    logging.info('Building benchmark for toolchain setting No.%d...',
                 setting_no)
    logging.info('Command: %s', build_cmd)

    try:
        subprocess.check_call(build_cmd)
    except:
        logging.error('Error while building benchmark!')
        raise


def run_and_collect_result(test_cmd, setting_no, i, bench, serial='default'):

    # Run autotest script for benchmark on DUT
    check_call_with_log(test_cmd, 'test_log')

    logging.info('Benchmark with setting No.%d, iter.%d finished testing on '
                 'device %s.', setting_no, i, serial)

    # Rename results from the bench_result generated in autotest
    bench_result = os.path.join(config.bench_suite_dir, 'bench_result')
    if not os.path.exists(bench_result):
        logging.error('No result found at %s, '
                      'please check test_log for details.', bench_result)
        raise OSError('Result file %s not found.' % bench_result)

    new_bench_result = 'bench_result_%s_%s_%d_%d' % (bench, serial,
                                                     setting_no, i)
    new_bench_result_path = os.path.join(config.bench_suite_dir,
                                         new_bench_result)
    try:
        os.rename(bench_result, new_bench_result_path)
    except OSError:
        logging.error('Error while renaming raw result %s to %s',
                      bench_result, new_bench_result_path)
        raise

    logging.info('Benchmark result saved at %s.', new_bench_result_path)


def test_bench(bench, setting_no, iterations, serials, remote, mode):
    logging.info('Start running benchmark on device...')

    # Run benchmark and tests on DUT
    for i in xrange(iterations):
        logging.info('Iteration No.%d:', i)
        test_cmd = [
            os.path.join(
                os.path.join(config.android_home, config.autotest_dir),
                'site_utils/test_bench.py')
        ]
        test_cmd.append('-b=' + bench)
        test_cmd.append('-r=' + remote)
        test_cmd.append('-m=' + mode)

        # Deal with serials. If there is no serails specified, try to run test
        # on the only device. If specified, split the serials into a list and
        # run test on each device.
        if serials:
            for serial in serials.split(','):
                test_cmd.append('-s=' + serial)

                run_and_collect_result(test_cmd, setting_no, i, bench, serial)
                test_cmd.pop()
        else:
            run_and_collect_result(test_cmd, setting_no, i, bench)


def gen_json(bench, setting_no, iterations, serials):
    bench_result = os.path.join(config.bench_suite_dir, 'bench_result')

    logging.info('Generating JSON file for Crosperf...')

    if not serials:
        serials = 'default'

    for serial in serials.split(','):

        # Platform will be used as device lunch combo instead
        #experiment = '_'.join([serial, str(setting_no)])
        experiment = config.product_combo

        # Input format: bench_result_{bench}_{serial}_{setting_no}_
        input_file = '_'.join([bench_result, bench,
                               serial, str(setting_no), ''])
        gen_json_cmd = [
            './gen_json.py', '--input=' + input_file,
            '--output=%s.json' % os.path.join(config.bench_suite_dir, bench),
            '--bench=' + bench, '--platform=' + experiment,
            '--iterations=' + str(iterations)
        ]

        logging.info('Command: %s', gen_json_cmd)
        if subprocess.call(gen_json_cmd):
            logging.error('Error while generating JSON file, please check raw'
                          ' data of the results at %s.', input_file)


def gen_crosperf(infile, outfile):
    # Set environment variable for crosperf
    os.environ['PYTHONPATH'] = os.path.dirname(config.toolchain_utils)

    logging.info('Generating Crosperf Report...')
    crosperf_cmd = [
        os.path.join(config.toolchain_utils, 'generate_report.py'),
        '-i=' + infile, '-o=' + outfile, '-f'
    ]

    # Run crosperf generate_report.py
    logging.info('Command: %s', crosperf_cmd)
    subprocess.call(crosperf_cmd)

    logging.info('Report generated successfully!')
    logging.info('Report Location: ' + outfile + '.html at bench'
                 'suite directory.')


def main(argv):
    # Set environment variable for the local loacation of benchmark suite.
    # This is for collecting testing results to benchmark suite directory.
    os.environ['BENCH_SUITE_DIR'] = config.bench_suite_dir

    # Set Android type, used for the difference part between aosp and internal.
    os.environ['ANDROID_TYPE'] = config.android_type

    # Set ANDROID_HOME for both building and testing.
    os.environ['ANDROID_HOME'] = config.android_home

    # Set environment variable for architecture, this will be used in
    # autotest.
    os.environ['PRODUCT'] = config.product

    arguments = _parse_arguments(argv)

    bench_list = arguments.bench
    if not bench_list:
        bench_list = config.bench_list

    compiler = arguments.compiler_dir
    build_os = arguments.build_os
    llvm_version = arguments.llvm_prebuilts_version
    cflags = arguments.cflags
    ldflags = arguments.ldflags
    iterations = arguments.iterations
    serials = arguments.serials
    remote = arguments.remote
    frequency = arguments.frequency
    mode = arguments.mode
    keep = arguments.keep

    # Clear old logs every time before run script
    clear_logs()

    if keep == 'False':
        clear_results()

    # Set test mode and frequency of CPU on the DUT
    set_device(serials, remote, frequency)

    test = arguments.test
    # if test configuration file has been given, use the build settings
    # in the configuration file and run the test.
    if test:
        test_config = ConfigParser.ConfigParser(allow_no_value=True)
        if not test_config.read(test):
            logging.error('Error while reading from building '
                          'configuration file %s.', test)
            raise RuntimeError('Error while reading configuration file %s.'
                               % test)

        for setting_no, section in enumerate(test_config.sections()):
            bench = test_config.get(section, 'bench')
            compiler = [test_config.get(section, 'compiler')]
            build_os = [test_config.get(section, 'build_os')]
            llvm_version = [test_config.get(section, 'llvm_version')]
            cflags = [test_config.get(section, 'cflags')]
            ldflags = [test_config.get(section, 'ldflags')]

            # Set iterations from test_config file, if not exist, use the one
            # from command line.
            it = test_config.get(section, 'iterations')
            if not it:
                it = iterations
            it = int(it)

            # Build benchmark for each single test configuration
            build_bench(0, bench, compiler, llvm_version,
                        build_os, cflags, ldflags)

            test_bench(bench, setting_no, it, serials, remote, mode)

            gen_json(bench, setting_no, it, serials)

        for bench in config.bench_list:
            infile = os.path.join(config.bench_suite_dir, bench + '.json')
            if os.path.exists(infile):
                outfile = os.path.join(config.bench_suite_dir,
                                       bench + '_report')
                gen_crosperf(infile, outfile)

        # Stop script if there is only config file provided
        return 0

    # If no configuration file specified, continue running.
    # Check if the count of the setting arguments are log_ambiguous.
    setting_count = check_count(compiler, llvm_version, build_os,
                                cflags, ldflags)

    for bench in bench_list:
        logging.info('Start building and running benchmark: [%s]', bench)
        # Run script for each toolchain settings
        for setting_no in xrange(setting_count):
            build_bench(setting_no, bench, compiler, llvm_version,
                        build_os, cflags, ldflags)

            # Run autotest script for benchmark test on device
            test_bench(bench, setting_no, iterations, serials, remote, mode)

            gen_json(bench, setting_no, iterations, serials)

        infile = os.path.join(config.bench_suite_dir, bench + '.json')
        outfile = os.path.join(config.bench_suite_dir, bench + '_report')
        gen_crosperf(infile, outfile)


if __name__ == '__main__':
    main(sys.argv[1:])
