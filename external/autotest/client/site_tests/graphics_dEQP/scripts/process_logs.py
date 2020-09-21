#!/usr/bin/python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable-msg=W0311

from collections import namedtuple
import argparse
import glob
import json
import os
import pprint
import re
import subprocess

_EXPECTATIONS_DIR = 'expectations'
_AUTOTEST_RESULT_ID_TEMPLATE = 'gs://chromeos-autotest-results/%s-chromeos-test/chromeos*/graphics_dEQP/debug/graphics_dEQP.DEBUG'
#_AUTOTEST_RESULT_TAG_TEMPLATE = 'gs://chromeos-autotest-results/%s/graphics_dEQP/debug/graphics_dEQP.DEBUG'
_AUTOTEST_RESULT_TAG_TEMPLATE = 'gs://chromeos-autotest-results/%s/debug/client.0.DEBUG'
# Use this template for tryjob results:
#_AUTOTEST_RESULT_TEMPLATE = 'gs://chromeos-autotest-results/%s-ihf/*/graphics_dEQP/debug/graphics_dEQP.DEBUG'
_BOARD_REGEX = re.compile(r'ChromeOS BOARD = (.+)')
_CPU_FAMILY_REGEX = re.compile(r'ChromeOS CPU family = (.+)')
_GPU_FAMILY_REGEX = re.compile(r'ChromeOS GPU family = (.+)')
_TEST_FILTER_REGEX = re.compile(r'dEQP test filter = (.+)')
_HASTY_MODE_REGEX = re.compile(r'\'hasty\': \'True\'|Running in hasty mode.')

#04/23 07:30:21.624 INFO |graphics_d:0240| TestCase: dEQP-GLES3.functional.shaders.operator.unary_operator.bitwise_not.highp_ivec3_vertex
#04/23 07:30:21.840 INFO |graphics_d:0261| Result: Pass
_TEST_RESULT_REGEX = re.compile(r'TestCase: (.+?)$\n.+? Result: (.+?)$',
                                re.MULTILINE)
_HASTY_TEST_RESULT_REGEX = re.compile(
    r'\[stdout\] Test case \'(.+?)\'..$\n'
    r'.+?\[stdout\]   (Pass|NotSupported|QualityWarning|CompatibilityWarning|'
    r'Fail|ResourceError|Crash|Timeout|InternalError|Skipped) \((.+)\)', re.MULTILINE)
Logfile = namedtuple('Logfile', 'job_id name gs_path')


def execute(cmd_list):
  sproc = subprocess.Popen(cmd_list, stdout=subprocess.PIPE)
  return sproc.communicate()[0]


def get_metadata(s):
  cpu = re.search(_CPU_FAMILY_REGEX, s).group(1)
  gpu = re.search(_GPU_FAMILY_REGEX, s).group(1)
  board = re.search(_BOARD_REGEX, s).group(1)
  filter = re.search(_TEST_FILTER_REGEX, s).group(1)
  hasty = False
  if re.search(_HASTY_MODE_REGEX, s):
    hasty = True
  print('Found results from %s for GPU = %s, filter = %s and hasty = %r.' %
        (board, gpu, filter, hasty))
  return board, gpu, filter, hasty


def copy_logs_from_gs_path(autotest_result_path):
  logs = []
  gs_paths = execute(['gsutil', 'ls', autotest_result_path]).splitlines()
  for gs_path in gs_paths:
    job_id = gs_path.split('/')[3].split('-')[0]
    # DEBUG logs have more information than INFO logs, especially for hasty.
    name = os.path.join('logs', job_id + '_graphics_dEQP.DEBUG')
    logs.append(Logfile(job_id, name, gs_path))
  for log in logs:
    execute(['gsutil', 'cp', log.gs_path, log.name])
  return logs


def get_local_logs():
  logs = []
  for name in glob.glob(os.path.join('logs', '*_graphics_dEQP.INFO')):
    job_id = name.split('_')[0]
    logs.append(Logfile(job_id, name, name))
  for name in glob.glob(os.path.join('logs', '*_graphics_dEQP.DEBUG')):
    job_id = name.split('_')[0]
    logs.append(Logfile(job_id, name, name))
  return logs


def get_all_tests(text):
  tests = []
  for test, result in re.findall(_TEST_RESULT_REGEX, text):
    tests.append((test, result))
  for test, result, details in re.findall(_HASTY_TEST_RESULT_REGEX, text):
    tests.append((test, result))
  return tests


def get_not_passing_tests(text):
  not_passing = []
  for test, result in re.findall(_TEST_RESULT_REGEX, text):
    if not (result == 'Pass' or result == 'NotSupported' or result == 'Skipped' or
            result == 'QualityWarning' or result == 'CompatibilityWarning'):
      not_passing.append((test, result))
  for test, result, details in re.findall(_HASTY_TEST_RESULT_REGEX, text):
    if result != 'Pass':
      not_passing.append((test, result))
  return not_passing


def load_expectation_dict(json_file):
  data = {}
  if os.path.isfile(json_file):
    print 'Loading file ' + json_file
    with open(json_file, 'r') as f:
      text = f.read()
      data = json.loads(text)
  return data


def load_expectations(json_file):
  data = load_expectation_dict(json_file)
  expectations = {}
  # Convert from dictionary of lists to dictionary of sets.
  for key in data:
    expectations[key] = set(data[key])
  return expectations


def expectation_list_to_dict(tests):
  data = {}
  tests = list(set(tests))
  for test, result in tests:
    if data.has_key(result):
      new_list = list(set(data[result].append(test)))
      data.pop(result)
      data[result] = new_list
    else:
      data[result] = [test]
  return data


def save_expectation_dict(expectation_path, expectation_dict):
  # Clean up obsolete expectations.
  for file_name in glob.glob(expectation_path + '.*'):
    if not '.hasty.' in file_name or '.hasty' in expectation_path:
      os.remove(file_name)
  # Dump json for next iteration.
  with open(expectation_path + '.json', 'w') as f:
    json.dump(expectation_dict,
              f,
              sort_keys=True,
              indent=4,
              separators=(',', ': '))
  # Dump plain text for autotest.
  for key in expectation_dict:
    if expectation_dict[key]:
      with open(expectation_path + '.' + key, 'w') as f:
        for test in expectation_dict[key]:
          f.write(test)
          f.write('\n')


# Figure out duplicates and move them to Flaky result set/list.
def process_flaky(status_dict):
  """Figure out duplicates and move them to Flaky result set/list."""
  clean_dict = {}
  flaky = set([])
  if status_dict.has_key('Flaky'):
    flaky = status_dict['Flaky']

  # FLaky tests are tests with 2 distinct results.
  for key1 in status_dict.keys():
    for key2 in status_dict.keys():
      if key1 != key2:
        flaky |= status_dict[key1] & status_dict[key2]

  # Remove Flaky tests from other status and convert to dict of list.
  for key in status_dict.keys():
    if key != 'Flaky':
      not_flaky = list(status_dict[key] - flaky)
      not_flaky.sort()
      print 'Number of "%s" is %d.' % (key, len(not_flaky))
      clean_dict[key] = not_flaky

  # And finally process flaky list/set.
  flaky_list = list(flaky)
  flaky_list.sort()
  clean_dict['Flaky'] = flaky_list

  return clean_dict


def merge_expectation_list(expectation_path, tests):
  status_dict = {}
  expectation_json = expectation_path + '.json'
  if os.access(expectation_json, os.R_OK):
    status_dict = load_expectations(expectation_json)
  else:
    print 'Could not load', expectation_json
  for test, result in tests:
    if status_dict.has_key(result):
      new_set = status_dict[result]
      new_set.add(test)
      status_dict.pop(result)
      status_dict[result] = new_set
    else:
      status_dict[result] = set([test])
  clean_dict = process_flaky(status_dict)
  save_expectation_dict(expectation_path, clean_dict)


def load_log(name):
  """Load test log and clean it from stderr spew."""
  with open(name) as f:
    lines = f.read().splitlines()
  text = ''
  for line in lines:
    if ('dEQP test filter =' in line or 'ChromeOS BOARD = ' in line or
        'ChromeOS CPU family =' in line or 'ChromeOS GPU family =' in line or
        'TestCase: ' in line or 'Result: ' in line or
        'Test Options: ' in line or 'Running in hasty mode.' in line or
        # For hasty logs we have:
        'Pass (' in line or 'NotSupported (' in line or 'Skipped (' in line or
        'QualityWarning (' in line or 'CompatibilityWarning (' in line or
        'Fail (' in line or 'ResourceError (' in line or 'Crash (' in line or
        'Timeout (' in line or 'InternalError (' in line or
        ' Test case \'' in line):
      text += line + '\n'
  # TODO(ihf): Warn about or reject log files missing the end marker.
  return text


def all_passing(tests):
  for _, result in tests:
    if not (result == 'Pass'):
      return False
  return True


def process_logs(logs):
  for log in logs:
    text = load_log(log.name)
    if text:
      print '================================================================'
      print 'Loading %s...' % log.name
      try:
        _, gpu, filter, hasty = get_metadata(text)
        tests = get_all_tests(text)
        print 'Found %d test results.' % len(tests)
        if all_passing(tests):
          # Delete logs that don't contain failures.
          os.remove(log.name)
        else:
          # GPU family goes first in path to simplify adding/deleting families.
          output_path = os.path.join(_EXPECTATIONS_DIR, gpu)
          if not os.access(output_path, os.R_OK):
            os.makedirs(output_path)
          expectation_path = os.path.join(output_path, filter)
          if hasty:
            expectation_path = os.path.join(output_path, filter + '.hasty')
          merge_expectation_list(expectation_path, tests)
      except:
        print 'Error processing %s' % log.name


JOB_TAGS_ALL = (
'select distinct job_tag from chromeos_autotest_db.tko_test_view_2 '
'where not job_tag like "%%hostless" and '
'test_name LIKE "graphics_dEQP%%" and '
'build_version>="%s" and '
'build_version<="%s" and '
'((status = "FAIL" and not job_name like "%%.NotPass") or '
'job_name like "%%.functional" or '
'job_name like "%%-master")' )

JOB_TAGS_MASTER = (
'select distinct job_tag from chromeos_autotest_db.tko_test_view_2 '
'where not job_tag like "%%hostless" and '
'test_name LIKE "graphics_dEQP%%" and '
'build_version>="%s" and '
'build_version<="%s" and '
'job_name like "%%-master"' )

def get_result_paths_from_autotest_db(host, user, password, build_from,
                                      build_to):
  paths = []
  # TODO(ihf): Introduce flag to toggle between JOB_TAGS_ALL and _MASTER.
  sql = JOB_TAGS_MASTER % (build_from, build_to)
  cmd = ['mysql', '-u%s' % user, '-p%s' % password, '--host', host, '-e', sql]
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
  for line in p.communicate()[0].splitlines():
    # Skip over unrelated sql spew (really first line only):
    if line and 'chromeos-test' in line:
      paths.append(_AUTOTEST_RESULT_TAG_TEMPLATE % line.rstrip())
  print 'Found %d potential results in the database.' % len(paths)
  return paths


def copy_logs_from_gs_paths(paths):
  i = 1
  for gs_path in paths:
    print '[%d/%d] %s' % (i, len(paths), gs_path)
    copy_logs_from_gs_path(gs_path)
    i = i+1


argparser = argparse.ArgumentParser(
    description='Download from GS and process dEQP logs into expectations.')
argparser.add_argument(
    '--host',
    dest='host',
    default='173.194.81.83',
    help='Host containing autotest result DB.')
argparser.add_argument('--user', dest='user', help='Database user account.')
argparser.add_argument(
    '--password',
    dest='password',
    help='Password for user account.')
argparser.add_argument(
    '--from',
    dest='build_from',
    help='Lowest build revision to include. Example: R51-8100.0.0')
argparser.add_argument(
    '--to',
    dest='build_to',
    help='Highest build revision to include. Example: R51-8101.0.0')

args = argparser.parse_args()

print pprint.pformat(args)
# This is somewhat optional. Remove existing expectations to start clean, but
# feel free to process them incrementally.
execute(['rm', '-rf', _EXPECTATIONS_DIR])

copy_logs_from_gs_paths(get_result_paths_from_autotest_db(
    args.host, args.user, args.password, args.build_from, args.build_to))

# This will include the just downloaded logs from GS as well.
logs = get_local_logs()
process_logs(logs)
