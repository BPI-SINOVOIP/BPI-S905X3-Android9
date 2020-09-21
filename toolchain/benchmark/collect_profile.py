#!/usr/bin/python

# Script to generate and collect PGO data based on benchmark
from __future__ import print_function

import argparse
import config
import logging
import os
import subprocess
import sys
import tempfile

# Turn the logging level to INFO before importing other code, to avoid having
# failed import logging messages confuse the user.
logging.basicConfig(level=logging.INFO)

def _parse_arguments_internal(argv):
    """
    Parse command line arguments

    @param argv: argument list to parse

    @returns:    tuple of parsed arguments and argv suitable for remote runs

    @raises SystemExit if arguments are malformed, or required arguments
            are not present.
    """

    parser = argparse.ArgumentParser(description='Run this script to collect '
                                                 'PGO data.')

    parser.add_argument('-b', '--bench',
                        help='Select which benchmark to collect profdata.')

    parser.add_argument('-d', '--pathDUT', default='/data/local/tmp',
                        help='Specify where to generate PGO data on device, '
                             'set to /data/local/tmp by default.')

    parser.add_argument('-p', '--path', default=config.bench_suite_dir,
                        help='Specify the location to put the profdata, set '
                             ' to bench_suite_dir by default.')

    parser.add_argument('-s', '--serial',
                        help='Device serial number.')

    parser.add_argument('-r', '--remote', default='localhost',
                        help='hostname[:port] if the ADB device is connected '
                             'to a remote machine. Ensure this workstation '
                             'is configured for passwordless ssh access as '
                             'users "root" or "adb"')
    return parser.parse_args(argv)

# Call run.py to build benchmark with -fprofile-generate flags and run on DUT
def run_suite(bench, serial, remote, pathDUT):
    logging.info('Build and run instrumented benchmark...')
    run_cmd = ['./run.py', '-b=' + bench]
    if serial:
        run_cmd.append('-s=' + serial)
    run_cmd.append('-r=' + remote)
    run_cmd.append('-f=-fprofile-generate=%s' % pathDUT)
    run_cmd.append('--ldflags=-fprofile-generate=%s' % pathDUT)
    try:
        subprocess.check_call(run_cmd)
    except subprocess.CalledProcessError:
        logging.error('Error running %s.', run_cmd)
        raise

# Pull profraw data from device using pull_device.py script in autotest utils.
def pull_result(bench, serial, remote, pathDUT, path):
    logging.info('Pulling profraw data from device to local')
    pull_cmd = [os.path.join(config.android_home,
                             config.autotest_dir,
                             'site_utils/pull_device.py')]
    pull_cmd.append('-b=' + bench)
    pull_cmd.append('-r=' + remote)
    if serial:
        pull_cmd.append('-s=' + serial)
    pull_cmd.append('-p=' + path)
    pull_cmd.append('-d=' + pathDUT)
    try:
        subprocess.check_call(pull_cmd)
    except:
        logging.error('Error while pulling profraw data.')
        raise

# Use llvm-profdata tool to convert profraw data to the format llvm can
# recgonize.
def merge(bench, pathDUT, path):
    logging.info('Generate profdata for PGO...')
    # Untar the compressed rawdata file collected from device
    tmp_dir = tempfile.mkdtemp()
    untar_cmd = ['tar',
                 '-xf',
                 os.path.join(path, bench + '_profraw.tar'),
                 '-C',
                 tmp_dir]

    # call llvm-profdata to merge the profraw data
    profdata = os.path.join(path, bench + '.profdata')
    merge_cmd = ['llvm-profdata',
                 'merge',
                 '-output=' + profdata,
                 tmp_dir + pathDUT]
    try:
        subprocess.check_call(untar_cmd)
        subprocess.check_call(merge_cmd)
        logging.info('Profdata is generated successfully, located at %s',
                     profdata)
    except:
        logging.error('Error while merging profraw data.')
        raise
    finally:
        subprocess.check_call(['rm', '-rf', tmp_dir])

def main(argv):
    """
    Entry point for nightly_run script.

    @param argv: arguments list
    """
    arguments = _parse_arguments_internal(argv)

    bench = arguments.bench
    serial = arguments.serial
    path = arguments.path
    remote = arguments.remote

    # Create a profraw directory to collect data
    pathDUT = os.path.join(arguments.pathDUT, bench + '_profraw')

    run_suite(bench, serial, remote, pathDUT)

    pull_result(bench, serial, remote, pathDUT, path)

    merge(bench, pathDUT, path)

if __name__ == '__main__':
    main(sys.argv[1:])
