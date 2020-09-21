#!/usr/bin/env python2
"""Generate summary report for ChromeOS toolchain waterfalls."""

# Desired future features (to be added):
# - arguments to allow generating only the main waterfall report,
#   or only the rotating builder reports, or only the failures
#   report; or the waterfall reports without the failures report.
# - Better way of figuring out which dates/builds to generate
#   reports for: probably an argument specifying a date or a date
#   range, then use something like the new buildbot utils to
#   query the build logs to find the right build numbers for the
#   builders for the specified dates.
# - Store/get the json/data files in mobiletc-prebuild's x20 area.
# - Update data in json file to reflect, for each testsuite, which
#   tests are not expected to run on which boards; update this
#   script to use that data appropriately.
# - Make sure user's prodaccess is up-to-date before trying to use
#   this script.
# - Add some nice formatting/highlighting to reports.

from __future__ import print_function

import argparse
import getpass
import json
import os
import re
import shutil
import sys
import time

from cros_utils import command_executer

# All the test suites whose data we might want for the reports.
TESTS = (('bvt-inline', 'HWTest'), ('bvt-cq', 'HWTest'), ('security', 'HWTest'),
         ('kernel_daily_regression', 'HWTest'), ('kernel_daily_benchmarks',
                                                 'HWTest'),)

# The main waterfall builders, IN THE ORDER IN WHICH WE WANT THEM
# LISTED IN THE REPORT.
WATERFALL_BUILDERS = [
    'amd64-gcc-toolchain', 'arm-gcc-toolchain', 'arm64-gcc-toolchain',
    'x86-gcc-toolchain', 'amd64-llvm-toolchain', 'arm-llvm-toolchain',
    'arm64-llvm-toolchain', 'x86-llvm-toolchain', 'amd64-llvm-next-toolchain',
    'arm-llvm-next-toolchain', 'arm64-llvm-next-toolchain',
    'x86-llvm-next-toolchain'
]

DATA_DIR = '/google/data/rw/users/mo/mobiletc-prebuild/waterfall-report-data/'
ARCHIVE_DIR = '/google/data/rw/users/mo/mobiletc-prebuild/waterfall-reports/'
DOWNLOAD_DIR = '/tmp/waterfall-logs'
MAX_SAVE_RECORDS = 7
BUILD_DATA_FILE = '%s/build-data.txt' % DATA_DIR
GCC_ROTATING_BUILDER = 'gcc_toolchain'
LLVM_ROTATING_BUILDER = 'llvm_next_toolchain'
ROTATING_BUILDERS = [GCC_ROTATING_BUILDER, LLVM_ROTATING_BUILDER]

# For int-to-string date conversion.  Note, the index of the month in this
# list needs to correspond to the month's integer value.  i.e. 'Sep' must
# be as MONTHS[9].
MONTHS = [
    '', 'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct',
    'Nov', 'Dec'
]


def format_date(int_date):
  """Convert an integer date to a string date. YYYYMMDD -> YYYY-MMM-DD"""

  if int_date == 0:
    return 'today'

  tmp_date = int_date
  day = tmp_date % 100
  tmp_date = tmp_date / 100
  month = tmp_date % 100
  year = tmp_date / 100

  month_str = MONTHS[month]
  date_str = '%d-%s-%d' % (year, month_str, day)
  return date_str


def EmailReport(report_file, report_type, date):
  subject = '%s Waterfall Summary report, %s' % (report_type, date)
  email_to = getpass.getuser()
  sendgmr_path = '/google/data/ro/projects/gws-sre/sendgmr'
  command = ('%s --to=%s@google.com --subject="%s" --body_file=%s' %
             (sendgmr_path, email_to, subject, report_file))
  command_executer.GetCommandExecuter().RunCommand(command)


def PruneOldFailures(failure_dict, int_date):
  earliest_date = int_date - MAX_SAVE_RECORDS
  for suite in failure_dict:
    suite_dict = failure_dict[suite]
    test_keys_to_remove = []
    for test in suite_dict:
      test_dict = suite_dict[test]
      msg_keys_to_remove = []
      for msg in test_dict:
        fails = test_dict[msg]
        i = 0
        while i < len(fails) and fails[i][0] <= earliest_date:
          i += 1
        new_fails = fails[i:]
        test_dict[msg] = new_fails
        if len(new_fails) == 0:
          msg_keys_to_remove.append(msg)

      for k in msg_keys_to_remove:
        del test_dict[k]

      suite_dict[test] = test_dict
      if len(test_dict) == 0:
        test_keys_to_remove.append(test)

    for k in test_keys_to_remove:
      del suite_dict[k]

    failure_dict[suite] = suite_dict


def GetBuildID(build_bot, date):
  """Get the build id for a build_bot at a given date."""
  day = '{day:02d}'.format(day=date % 100)
  mon = MONTHS[date / 100 % 100]
  date_string = mon + ' ' + day
  if build_bot in WATERFALL_BUILDERS:
    url = 'https://uberchromegw.corp.google.com/i/chromeos/' + \
          'builders/%s?numbuilds=200' % build_bot
  if build_bot in ROTATING_BUILDERS:
    url = 'https://uberchromegw.corp.google.com/i/chromiumos.tryserver/' + \
          'builders/%s?numbuilds=200' % build_bot
  command = 'sso_client %s' % url
  retval = 1
  retry_time = 3
  while retval and retry_time:
    retval, output, _ = \
        command_executer.GetCommandExecuter().RunCommandWOutput(command, \
        print_to_console=False)
    retry_time -= 1

  if retval:
    return []

  out = output.split('\n')
  line_num = 0
  build_id = []
  # Parse the output like this
  # <td>Dec 14 10:55</td>
  # <td class="revision">??</td>
  # <td failure</td><td><a href="../builders/gcc_toolchain/builds/109">#109</a>
  while line_num < len(out):
    if date_string in out[line_num]:
      if line_num + 2 < len(out):
        build_num_line = out[line_num + 2]
        raw_num = re.findall(r'builds/\d+', build_num_line)
        # raw_num is ['builds/109'] in the example.
        if raw_num:
          build_id.append(int(raw_num[0].split('/')[1]))
    line_num += 1
  return build_id


def GenerateFailuresReport(fail_dict, date):
  filename = 'waterfall_report.failures.%s.txt' % date
  date_string = format_date(date)
  with open(filename, 'w') as out_file:
    # Write failure report section.
    out_file.write('\n\nSummary of Test Failures as of %s\n\n' % date_string)

    # We want to sort the errors and output them in order of the ones that occur
    # most often.  So we have to collect the data about all of them, then sort
    # it.
    error_groups = []
    for suite in fail_dict:
      suite_dict = fail_dict[suite]
      if suite_dict:
        for test in suite_dict:
          test_dict = suite_dict[test]
          for err_msg in test_dict:
            err_list = test_dict[err_msg]
            sorted_list = sorted(err_list, key=lambda x: x[0], reverse=True)
            err_group = [len(sorted_list), suite, test, err_msg, sorted_list]
            error_groups.append(err_group)

    # Sort the errors by the number of errors of each type. Then output them in
    # order.
    sorted_errors = sorted(error_groups, key=lambda x: x[0], reverse=True)
    for i in range(0, len(sorted_errors)):
      err_group = sorted_errors[i]
      suite = err_group[1]
      test = err_group[2]
      err_msg = err_group[3]
      err_list = err_group[4]
      out_file.write('Suite: %s\n' % suite)
      out_file.write('    %s (%d failures)\n' % (test, len(err_list)))
      out_file.write('    (%s)\n' % err_msg)
      for i in range(0, len(err_list)):
        err = err_list[i]
        out_file.write('        %s, %s, %s\n' % (format_date(err[0]), err[1],
                                                 err[2]))
      out_file.write('\n')

  print('Report generated in %s.' % filename)
  return filename


def GenerateWaterfallReport(report_dict, fail_dict, waterfall_type, date,
                            omit_failures):
  """Write out the actual formatted report."""

  filename = 'waterfall_report.%s_waterfall.%s.txt' % (waterfall_type, date)

  date_string = ''
  date_list = report_dict['date']
  num_dates = len(date_list)
  i = 0
  for d in date_list:
    date_string += d
    if i < num_dates - 1:
      date_string += ', '
    i += 1

  if waterfall_type == 'main':
    report_list = WATERFALL_BUILDERS
  else:
    report_list = report_dict.keys()

  with open(filename, 'w') as out_file:
    # Write Report Header
    out_file.write('\nStatus of %s Waterfall Builds from %s\n\n' %
                   (waterfall_type, date_string))
    out_file.write('                                                          '
                   '                kernel       kernel\n')
    out_file.write('                         Build    bvt-         bvt-cq     '
                   ' security       daily        daily\n')
    out_file.write('                         status  inline                   '
                   '              regression   benchmarks\n')
    out_file.write('                               [P/ F/ DR]*   [P/ F /DR]*  '
                   '[P/ F/ DR]* [P/ F/ DR]* [P/ F/ DR]*\n\n')

    # Write daily waterfall status section.
    for i in range(0, len(report_list)):
      builder = report_list[i]
      if builder == 'date':
        continue

      if builder not in report_dict:
        out_file.write('Unable to find information for %s.\n\n' % builder)
        continue

      build_dict = report_dict[builder]
      status = build_dict.get('build_status', 'bad')
      inline = build_dict.get('bvt-inline', '[??/ ?? /??]')
      cq = build_dict.get('bvt-cq', '[??/ ?? /??]')
      inline_color = build_dict.get('bvt-inline-color', '')
      cq_color = build_dict.get('bvt-cq-color', '')
      if 'x86' not in builder:
        security = build_dict.get('security', '[??/ ?? /??]')
        security_color = build_dict.get('security-color', '')
        if 'gcc' in builder:
          regression = build_dict.get('kernel_daily_regression', '[??/ ?? /??]')
          bench = build_dict.get('kernel_daily_benchmarks', '[??/ ?? /??]')
          regression_color = build_dict.get('kernel_daily_regression-color', '')
          bench_color = build_dict.get('kernel_daily_benchmarks-color', '')
          out_file.write('                                  %6s        %6s'
                         '      %6s      %6s      %6s\n' %
                         (inline_color, cq_color, security_color,
                          regression_color, bench_color))
          out_file.write('%25s %3s  %s %s %s %s %s\n' %
                         (builder, status, inline, cq, security, regression,
                          bench))
        else:
          out_file.write('                                  %6s        %6s'
                         '      %6s\n' % (inline_color, cq_color,
                                          security_color))
          out_file.write('%25s %3s  %s %s %s\n' % (builder, status, inline, cq,
                                                   security))
      else:
        out_file.write('                                  %6s        %6s\n' %
                       (inline_color, cq_color))
        out_file.write('%25s %3s  %s %s\n' % (builder, status, inline, cq))
      if 'build_link' in build_dict:
        out_file.write('%s\n\n' % build_dict['build_link'])

    out_file.write('\n\n*P = Number of tests in suite that Passed; F = '
                   'Number of tests in suite that Failed; DR = Number of tests'
                   ' in suite that Didn\'t Run.\n')

    if omit_failures:
      print('Report generated in %s.' % filename)
      return filename

    # Write failure report section.
    out_file.write('\n\nSummary of Test Failures as of %s\n\n' % date_string)

    # We want to sort the errors and output them in order of the ones that occur
    # most often.  So we have to collect the data about all of them, then sort
    # it.
    error_groups = []
    for suite in fail_dict:
      suite_dict = fail_dict[suite]
      if suite_dict:
        for test in suite_dict:
          test_dict = suite_dict[test]
          for err_msg in test_dict:
            err_list = test_dict[err_msg]
            sorted_list = sorted(err_list, key=lambda x: x[0], reverse=True)
            err_group = [len(sorted_list), suite, test, err_msg, sorted_list]
            error_groups.append(err_group)

    # Sort the errors by the number of errors of each type. Then output them in
    # order.
    sorted_errors = sorted(error_groups, key=lambda x: x[0], reverse=True)
    for i in range(0, len(sorted_errors)):
      err_group = sorted_errors[i]
      suite = err_group[1]
      test = err_group[2]
      err_msg = err_group[3]
      err_list = err_group[4]
      out_file.write('Suite: %s\n' % suite)
      out_file.write('    %s (%d failures)\n' % (test, len(err_list)))
      out_file.write('    (%s)\n' % err_msg)
      for i in range(0, len(err_list)):
        err = err_list[i]
        out_file.write('        %s, %s, %s\n' % (format_date(err[0]), err[1],
                                                 err[2]))
      out_file.write('\n')

  print('Report generated in %s.' % filename)
  return filename


def UpdateReport(report_dict, builder, test, report_date, build_link,
                 test_summary, board, color):
  """Update the data in our report dictionary with current test's data."""

  if 'date' not in report_dict:
    report_dict['date'] = [report_date]
  elif report_date not in report_dict['date']:
    # It is possible that some of the builders started/finished on different
    # days, so we allow for multiple dates in the reports.
    report_dict['date'].append(report_date)

  build_key = ''
  if builder == GCC_ROTATING_BUILDER:
    build_key = '%s-gcc-toolchain' % board
  elif builder == LLVM_ROTATING_BUILDER:
    build_key = '%s-llvm-next-toolchain' % board
  else:
    build_key = builder

  if build_key not in report_dict.keys():
    build_dict = dict()
  else:
    build_dict = report_dict[build_key]

  if 'build_link' not in build_dict:
    build_dict['build_link'] = build_link

  if 'date' not in build_dict:
    build_dict['date'] = report_date

  if 'board' in build_dict and build_dict['board'] != board:
    raise RuntimeError(
        'Error: Two different boards (%s,%s) in one build (%s)!' %
        (board, build_dict['board'], build_link))
  build_dict['board'] = board

  color_key = '%s-color' % test
  build_dict[color_key] = color

  # Check to see if we already have a build status for this build_key
  status = ''
  if 'build_status' in build_dict.keys():
    # Use current build_status, unless current test failed (see below).
    status = build_dict['build_status']

  if not test_summary:
    # Current test data was not available, so something was bad with build.
    build_dict['build_status'] = 'bad'
    build_dict[test] = '[  no data  ]'
  else:
    build_dict[test] = test_summary
    if not status:
      # Current test ok; no other data, so assume build was ok.
      build_dict['build_status'] = 'ok'

  report_dict[build_key] = build_dict


def UpdateBuilds(builds):
  """Update the data in our build-data.txt file."""

  # The build data file records the last build number for which we
  # generated a report.  When we generate the next report, we read
  # this data and increment it to get the new data; when we finish
  # generating the reports, we write the updated values into this file.
  # NOTE: One side effect of doing this at the end:  If the script
  # fails in the middle of generating a report, this data does not get
  # updated.
  with open(BUILD_DATA_FILE, 'w') as fp:
    gcc_max = 0
    llvm_max = 0
    for b in builds:
      if b[0] == GCC_ROTATING_BUILDER:
        gcc_max = max(gcc_max, b[1])
      elif b[0] == LLVM_ROTATING_BUILDER:
        llvm_max = max(llvm_max, b[1])
      else:
        fp.write('%s,%d\n' % (b[0], b[1]))
    if gcc_max > 0:
      fp.write('%s,%d\n' % (GCC_ROTATING_BUILDER, gcc_max))
    if llvm_max > 0:
      fp.write('%s,%d\n' % (LLVM_ROTATING_BUILDER, llvm_max))


def GetBuilds(date=0):
  """Get build id from builds."""

  # If date is set, get the build id from waterfall.
  builds = []

  if date:
    for builder in WATERFALL_BUILDERS + ROTATING_BUILDERS:
      build_ids = GetBuildID(builder, date)
      for build_id in build_ids:
        builds.append((builder, build_id))
    return builds

  # If date is not set, we try to get the most recent builds.
  # Read the values of the last builds used to generate a report, and
  # increment them appropriately, to get values for generating the
  # current report.  (See comments in UpdateBuilds).
  with open(BUILD_DATA_FILE, 'r') as fp:
    lines = fp.readlines()

  for l in lines:
    l = l.rstrip()
    words = l.split(',')
    builder = words[0]
    build = int(words[1])
    builds.append((builder, build + 1))
    # NOTE: We are assuming here that there are always 2 daily builds in
    # each of the rotating builders.  I am not convinced this is a valid
    # assumption.
    if builder in ROTATING_BUILDERS:
      builds.append((builder, build + 2))

  return builds


def RecordFailures(failure_dict, platform, suite, builder, int_date, log_file,
                   build_num, failed):
  """Read and update the stored data about test  failures."""

  # Get the dictionary for this particular test suite from the failures
  # dictionary.
  suite_dict = failure_dict[suite]

  # Read in the entire log file for this test/build.
  with open(log_file, 'r') as in_file:
    lines = in_file.readlines()

  # Update the entries in the failure dictionary for each test within this suite
  # that failed.
  for test in failed:
    # Check to see if there is already an entry in the suite dictionary for this
    # test; if so use that, otherwise create a new entry.
    if test in suite_dict:
      test_dict = suite_dict[test]
    else:
      test_dict = dict()
    # Parse the lines from the log file, looking for lines that indicate this
    # test failed.
    msg = ''
    for l in lines:
      words = l.split()
      if len(words) < 3:
        continue
      if ((words[0] == test and words[1] == 'ERROR:') or
          (words[0] == 'provision' and words[1] == 'FAIL:')):
        words = words[2:]
        # Get the error message for the failure.
        msg = ' '.join(words)
    if not msg:
      msg = 'Unknown_Error'

    # Look for an existing entry for this error message in the test dictionary.
    # If found use that, otherwise create a new entry for this error message.
    if msg in test_dict:
      error_list = test_dict[msg]
    else:
      error_list = list()
    # Create an entry for this new failure
    new_item = [int_date, platform, builder, build_num]
    # Add this failure to the error list if it's not already there.
    if new_item not in error_list:
      error_list.append([int_date, platform, builder, build_num])
    # Sort the error list by date.
    error_list.sort(key=lambda x: x[0])
    # Calculate the earliest date to save; delete records for older failures.
    earliest_date = int_date - MAX_SAVE_RECORDS
    i = 0
    while i < len(error_list) and error_list[i][0] <= earliest_date:
      i += 1
    if i > 0:
      error_list = error_list[i:]
    # Save the error list in the test's dictionary, keyed on error_msg.
    test_dict[msg] = error_list

    # Save the updated test dictionary in the test_suite dictionary.
    suite_dict[test] = test_dict

  # Save the updated test_suite dictionary in the failure dictionary.
  failure_dict[suite] = suite_dict


def ParseLogFile(log_file, test_data_dict, failure_dict, test, builder,
                 build_num, build_link):
  """Parse the log file from the given builder, build_num and test.

     Also adds the results for this test to our test results dictionary,
     and calls RecordFailures, to update our test failure data.
  """

  lines = []
  with open(log_file, 'r') as infile:
    lines = infile.readlines()

  passed = {}
  failed = {}
  not_run = {}
  date = ''
  status = ''
  board = ''
  num_provision_errors = 0
  build_ok = True
  afe_line = ''

  for line in lines:
    if line.rstrip() == '<title>404 Not Found</title>':
      print('Warning: File for %s (build number %d), %s was not found.' %
            (builder, build_num, test))
      build_ok = False
      break
    if '[ PASSED ]' in line:
      test_name = line.split()[0]
      if test_name != 'Suite':
        passed[test_name] = True
    elif '[ FAILED ]' in line:
      test_name = line.split()[0]
      if test_name == 'provision':
        num_provision_errors += 1
        not_run[test_name] = True
      elif test_name != 'Suite':
        failed[test_name] = True
    elif line.startswith('started: '):
      date = line.rstrip()
      date = date[9:]
      date_obj = time.strptime(date, '%a %b %d %H:%M:%S %Y')
      int_date = (
          date_obj.tm_year * 10000 + date_obj.tm_mon * 100 + date_obj.tm_mday)
      date = time.strftime('%a %b %d %Y', date_obj)
    elif not status and line.startswith('status: '):
      status = line.rstrip()
      words = status.split(':')
      status = words[-1]
    elif line.find('Suite passed with a warning') != -1:
      status = 'WARNING'
    elif line.startswith('@@@STEP_LINK@Link to suite@'):
      afe_line = line.rstrip()
      words = afe_line.split('@')
      for w in words:
        if w.startswith('http'):
          afe_line = w
          afe_line = afe_line.replace('&amp;', '&')
    elif 'INFO: RunCommand:' in line:
      words = line.split()
      for i in range(0, len(words) - 1):
        if words[i] == '--board':
          board = words[i + 1]

  test_dict = test_data_dict[test]
  test_list = test_dict['tests']

  if build_ok:
    for t in test_list:
      if not t in passed and not t in failed:
        not_run[t] = True

    total_pass = len(passed)
    total_fail = len(failed)
    total_notrun = len(not_run)

  else:
    total_pass = 0
    total_fail = 0
    total_notrun = 0
    status = 'Not found.'
  if not build_ok:
    return [], date, board, 0, '     '

  build_dict = dict()
  build_dict['id'] = build_num
  build_dict['builder'] = builder
  build_dict['date'] = date
  build_dict['build_link'] = build_link
  build_dict['total_pass'] = total_pass
  build_dict['total_fail'] = total_fail
  build_dict['total_not_run'] = total_notrun
  build_dict['afe_job_link'] = afe_line
  build_dict['provision_errors'] = num_provision_errors
  if status.strip() == 'SUCCESS':
    build_dict['color'] = 'green '
  elif status.strip() == 'FAILURE':
    build_dict['color'] = ' red  '
  elif status.strip() == 'WARNING':
    build_dict['color'] = 'orange'
  else:
    build_dict['color'] = '      '

  # Use YYYYMMDD (integer) as the build record key
  if build_ok:
    if board in test_dict:
      board_dict = test_dict[board]
    else:
      board_dict = dict()
    board_dict[int_date] = build_dict

  # Only keep the last 5 records (based on date)
  keys_list = board_dict.keys()
  if len(keys_list) > MAX_SAVE_RECORDS:
    min_key = min(keys_list)
    del board_dict[min_key]

  # Make sure changes get back into the main dictionary
  test_dict[board] = board_dict
  test_data_dict[test] = test_dict

  if len(failed) > 0:
    RecordFailures(failure_dict, board, test, builder, int_date, log_file,
                   build_num, failed)

  summary_result = '[%2d/ %2d/ %2d]' % (total_pass, total_fail, total_notrun)

  return summary_result, date, board, int_date, build_dict['color']


def DownloadLogFile(builder, buildnum, test, test_family):

  ce = command_executer.GetCommandExecuter()
  os.system('mkdir -p %s/%s/%s' % (DOWNLOAD_DIR, builder, test))
  if builder in ROTATING_BUILDERS:
    source = ('https://uberchromegw.corp.google.com/i/chromiumos.tryserver'
              '/builders/%s/builds/%d/steps/%s%%20%%5B%s%%5D/logs/stdio' %
              (builder, buildnum, test_family, test))
    build_link = ('https://uberchromegw.corp.google.com/i/chromiumos.tryserver'
                  '/builders/%s/builds/%d' % (builder, buildnum))
  else:
    source = ('https://uberchromegw.corp.google.com/i/chromeos/builders/%s/'
              'builds/%d/steps/%s%%20%%5B%s%%5D/logs/stdio' %
              (builder, buildnum, test_family, test))
    build_link = ('https://uberchromegw.corp.google.com/i/chromeos/builders/%s'
                  '/builds/%d' % (builder, buildnum))

  target = '%s/%s/%s/%d' % (DOWNLOAD_DIR, builder, test, buildnum)
  if not os.path.isfile(target) or os.path.getsize(target) == 0:
    cmd = 'sso_client %s > %s' % (source, target)
    status = ce.RunCommand(cmd)
    if status != 0:
      return '', ''

  return target, build_link


# Check for prodaccess.
def CheckProdAccess():
  status, output, _ = command_executer.GetCommandExecuter().RunCommandWOutput(
      'prodcertstatus')
  if status != 0:
    return False
  # Verify that status is not expired
  if 'expires' in output:
    return True
  return False


def ValidOptions(parser, options):
  too_many_options = False
  if options.main:
    if options.rotating or options.failures_report:
      too_many_options = True
  elif options.rotating and options.failures_report:
    too_many_options = True

  if too_many_options:
    parser.error('Can only specify one of --main, --rotating or'
                 ' --failures_report.')

  conflicting_failure_options = False
  if options.failures_report and options.omit_failures:
    conflicting_failure_options = True
    parser.error('Cannot specify both --failures_report and --omit_failures.')

  return not too_many_options and not conflicting_failure_options


def Main(argv):
  """Main function for this script."""
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--main',
      dest='main',
      default=False,
      action='store_true',
      help='Generate report only for main waterfall '
      'builders.')
  parser.add_argument(
      '--rotating',
      dest='rotating',
      default=False,
      action='store_true',
      help='Generate report only for rotating builders.')
  parser.add_argument(
      '--failures_report',
      dest='failures_report',
      default=False,
      action='store_true',
      help='Only generate the failures section of the report.')
  parser.add_argument(
      '--omit_failures',
      dest='omit_failures',
      default=False,
      action='store_true',
      help='Do not generate the failures section of the report.')
  parser.add_argument(
      '--no_update',
      dest='no_update',
      default=False,
      action='store_true',
      help='Run reports, but do not update the data files.')
  parser.add_argument(
      '--date',
      dest='date',
      default=0,
      type=int,
      help='The date YYYYMMDD of waterfall report.')

  options = parser.parse_args(argv)

  if not ValidOptions(parser, options):
    return 1

  main_only = options.main
  rotating_only = options.rotating
  failures_report = options.failures_report
  omit_failures = options.omit_failures
  date = options.date

  test_data_dict = dict()
  failure_dict = dict()

  prod_access = CheckProdAccess()
  if not prod_access:
    print('ERROR: Please run prodaccess first.')
    return

  with open('%s/waterfall-test-data.json' % DATA_DIR, 'r') as input_file:
    test_data_dict = json.load(input_file)

  with open('%s/test-failure-data.json' % DATA_DIR, 'r') as fp:
    failure_dict = json.load(fp)

  builds = GetBuilds(date)

  waterfall_report_dict = dict()
  rotating_report_dict = dict()
  int_date = 0
  for test_desc in TESTS:
    test, test_family = test_desc
    for build in builds:
      (builder, buildnum) = build
      if test.startswith('kernel') and 'llvm' in builder:
        continue
      if 'x86' in builder and not test.startswith('bvt'):
        continue
      target, build_link = DownloadLogFile(builder, buildnum, test, test_family)

      if os.path.exists(target):
        test_summary, report_date, board, tmp_date, color = ParseLogFile(
            target, test_data_dict, failure_dict, test, builder, buildnum,
            build_link)

        if tmp_date != 0:
          int_date = tmp_date

        if builder in ROTATING_BUILDERS:
          UpdateReport(rotating_report_dict, builder, test, report_date,
                       build_link, test_summary, board, color)
        else:
          UpdateReport(waterfall_report_dict, builder, test, report_date,
                       build_link, test_summary, board, color)

  PruneOldFailures(failure_dict, int_date)

  if waterfall_report_dict and not rotating_only and not failures_report:
    main_report = GenerateWaterfallReport(waterfall_report_dict, failure_dict,
                                          'main', int_date, omit_failures)
    EmailReport(main_report, 'Main', format_date(int_date))
    shutil.copy(main_report, ARCHIVE_DIR)
  if rotating_report_dict and not main_only and not failures_report:
    rotating_report = GenerateWaterfallReport(
        rotating_report_dict, failure_dict, 'rotating', int_date, omit_failures)
    EmailReport(rotating_report, 'Rotating', format_date(int_date))
    shutil.copy(rotating_report, ARCHIVE_DIR)

  if failures_report:
    failures_report = GenerateFailuresReport(failure_dict, int_date)
    EmailReport(failures_report, 'Failures', format_date(int_date))
    shutil.copy(failures_report, ARCHIVE_DIR)

  if not options.no_update:
    with open('%s/waterfall-test-data.json' % DATA_DIR, 'w') as out_file:
      json.dump(test_data_dict, out_file, indent=2)

    with open('%s/test-failure-data.json' % DATA_DIR, 'w') as out_file:
      json.dump(failure_dict, out_file, indent=2)

    UpdateBuilds(builds)


if __name__ == '__main__':
  Main(sys.argv[1:])
  sys.exit(0)
