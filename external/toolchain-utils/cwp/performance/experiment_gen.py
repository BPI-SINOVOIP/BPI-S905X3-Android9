# Copyright 2012 Google Inc. All Rights Reserved.
"""This script generates a crosperf overhead-testing experiment file for MoreJS.

Use: experiment_gen.py --crosperf=/home/mrdmnd/depot2/crosperf --chromeos_root=
/home/mrdmnd/chromiumos --remote-host=chromeos-zgb3.mtv --board=x86-zgb --event=
cycles -F 10 -F 20 -c 10582 -c 10785211 --perf_options="-g"
"""

import optparse
import subprocess
import sys
import time

HEADER = """
board: %s
remote: %s
benchmark: baseline {
 iterations: %s
 autotest_name: desktopui_PyAutoPerfTests
 autotest_args: --args='--iterations=%s perf.PageCyclerTest.testMoreJSFile'
}"""

EXPERIMENT = """
benchmark: %s {
 iterations: %s
 autotest_name: desktopui_PyAutoPerfTests
 autotest_args: --args='--iterations=%s perf.PageCyclerTest.testMoreJSFile' --profiler=custom_perf --profiler_args='perf_options="record -a %s %s -e %s"' \n}"""  # pylint: disable-msg=C6310

DEFAULT_IMAGE = """
default {
 chromeos_image: %s/src/build/images/%s/latest/chromiumos_test_image.bin
}"""


def main():
  parser = optparse.OptionParser()
  parser.add_option('--crosperf',
                    dest='crosperf_root',
                    action='store',
                    default='/home/mrdmnd/depot2/crosperf',
                    help='Crosperf root directory.')
  parser.add_option('--chromeos_root',
                    dest='chromeos_root',
                    action='store',
                    default='/home/mrdmnd/chromiumos',
                    help='ChromiumOS root directory.')
  parser.add_option('--remote',
                    dest='remote',
                    action='store',
                    help='Host to run test on. Required.')
  parser.add_option('--board',
                    dest='board',
                    action='store',
                    help='Board architecture to run on. Required.')
  parser.add_option('--event',
                    dest='event',
                    action='store',
                    help='Event to profile. Required.')
  parser.add_option('-F',
                    dest='sampling_frequencies',
                    action='append',
                    help='A target frequency to sample at.')
  parser.add_option('-c',
                    dest='sampling_periods',
                    action='append',
                    help='A target period to sample at. Event specific.')
  parser.add_option('--benchmark-iterations',
                    dest='benchmark_iterations',
                    action='store',
                    default=4,
                    help='Number of benchmark iters')
  parser.add_option('--test-iterations',
                    dest='test_iterations',
                    action='store',
                    default=10,
                    help='Number of test iters')
  parser.add_option('-p',
                    dest='print_only',
                    action='store_true',
                    help='If enabled, will print experiment file and exit.')
  parser.add_option('--perf_options',
                    dest='perf_options',
                    action='store',
                    help='Arbitrary flags to perf. Surround with dblquotes.')
  options = parser.parse_args()[0]
  if options.remote is None:
    print '%s requires a remote hostname.' % sys.argv[0]
    return 1
  elif options.board is None:
    print '%s requires a target board.' % sys.argv[0]
    return 1
  elif options.event is None:
    print '%s requires an event to profile.' % sys.argv[0]
    return 1
  else:
    crosperf_root = options.crosperf_root
    chromeos_root = options.chromeos_root
    remote = options.remote
    board = options.board
    event = options.event
    bench_iters = options.benchmark_iterations
    test_iters = options.test_iterations
    perf_opts = options.perf_options
    # Set up baseline test.
    experiment_file = HEADER % (board, remote, bench_iters, test_iters)
    # Set up experimental tests.
    if options.sampling_frequencies:
      for freq in options.sampling_frequencies:
        test_string = str(freq) + 'Freq'
        experiment_file += EXPERIMENT % (test_string, bench_iters, test_iters,
                                         '-F %s' % freq, '' if perf_opts is None
                                         else perf_opts, event)
    if options.sampling_periods:
      for period in options.sampling_periods:
        test_string = str(period) + 'Period'
        experiment_file += EXPERIMENT % (
            test_string, bench_iters, test_iters, '-c %s' % period, '' if
            perf_opts is None else perf_opts, event)
    # Point to the target image.
    experiment_file += DEFAULT_IMAGE % (chromeos_root, board)
    if options.print_only:
      print experiment_file
    else:
      current_time = int(round(time.time() * 1000))
      file_name = 'perf_overhead_%s' % str(current_time)
      with open(file_name, 'w') as f:
        f.write(experiment_file)
      try:
        process = subprocess.Popen(['%s/crosperf' % crosperf_root, file_name])
        process.communicate()
      except OSError:
        print 'Could not find crosperf, make sure --crosperf flag is set right.'
        return 1
    return 0


if __name__ == '__main__':
  exit(main())
