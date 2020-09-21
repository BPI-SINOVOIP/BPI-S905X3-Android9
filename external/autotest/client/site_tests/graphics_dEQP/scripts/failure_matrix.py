#!/usr/bin/python
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable-msg=W0311

import argparse
import json
import os


gpu_list = [
    #'pinetrail',
    'sandybridge',
    'ivybridge',
    'baytrail',
    'haswell',
    'broadwell',
    'braswell',
    'skylake',
    'broxton',
    'mali-t604',
    'mali-t628',
    'mali-t760',
    'mali-t860',
    'rogue',
    'tegra',
]

_PROBLEM_STATUS = ['Fail', 'Flaky']
_UNKNOWN_STATUS = ['NotSupported', 'Skipped', 'Unknown', None]

status_dict = {
    'Fail': 'FAIL ',
    'Flaky': 'flaky',
    'Pass': '  +  ',
    'NotSupported': ' --- ',
    'Skipped': ' === ',
    'QualityWarning': 'qw   ',
    'CompatibilityWarning': 'cw   ',
    'Unknown': ' ??? ',
    None: ' n/a ',
}

def load_expectation_dict(json_file):
  data = {}
  if os.path.isfile(json_file):
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


def get_problem_count(dict, gpu):
  if gpu in dict:
    if not dict[gpu]:
      return None
    count = 0
    for status in dict[gpu]:
      if status not in _UNKNOWN_STATUS:
        count = count + len((dict[gpu])[status])
    # If every result has an unknown status then don't return a count.
    if count < 1:
      return None
    count = 0
    # Return counts of truly problematic statuses.
    for status in _PROBLEM_STATUS:
      if status in dict[gpu]:
        count = count + len((dict[gpu])[status])
  else:
    print 'Warning: %s not found in dict!' % gpu
  return count


def get_problem_tests(dict):
  tests = set([])
  for gpu in dict:
    for status in _PROBLEM_STATUS:
      if status in dict[gpu]:
        tests = tests.union((dict[gpu])[status])
  return sorted(list(tests))


def get_test_result(dict, test):
  for key in dict:
    if test in dict[key]:
      return key
  return None


argparser = argparse.ArgumentParser(
    description='Create a matrix of failing tests per GPU.')
argparser.add_argument('interface',
                       default='gles2',
                       help='Interface for matrix (gles2, gles3, gles31).')
args = argparser.parse_args()
status = '%s-master.txt.json' % args.interface

dict = {}
for gpu in gpu_list:
  filename = 'expectations/%s/%s' % (gpu, status)
  dict[gpu] = load_expectations(filename)

tests = get_problem_tests(dict)

print 'Legend:'
for key in status_dict:
  print '%s  -->  %s' % (status_dict[key], key)
print

offset = ''
for gpu in gpu_list:
  print '%s%s' % (offset, gpu)
  offset = '%s    |    ' % offset
print offset

text_count = ''
text_del = ''
for gpu in gpu_list:
  problem_count = get_problem_count(dict, gpu)
  if problem_count is None:
    text_count = '%s  %s  ' % (text_count, status_dict[problem_count])
  else:
    text_count = '%s%5d    ' % (text_count, problem_count)
  text_del = '%s=========' % text_del
text_count = '%s  Total failure count (Fail + Flaky)' % text_count
print text_del
print text_count
print text_del

for test in tests:
  text = ''
  for gpu in gpu_list:
    result = get_test_result(dict[gpu], test)
    status = status_dict[result]
    text = '%s  %s  ' % (text, status)
  text = '%s  %s' % (text, test)
  print text

print text_del
print '%s repeated' % text_count
print text_del

