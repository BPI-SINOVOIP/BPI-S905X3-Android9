#!/usr/bin/python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Script to summarize the results of various log files."""

from __future__ import print_function

__author__ = 'raymes@google.com (Raymes Khoury)'

from cros_utils import command_executer
import os
import sys
import re

RESULTS_DIR = 'results'
RESULTS_FILE = RESULTS_DIR + '/results.csv'

# pylint: disable=anomalous-backslash-in-string

class DejaGNUSummarizer(object):
  """DejaGNU Summarizer Class"""

  def __int__(self):
    pass

  def Matches(self, log_file):
    for log_line in log_file:
      if log_line.find("""tests ===""") > -1:
        return True
    return False

  def Summarize(self, log_file, filename):
    result = ''
    pass_statuses = ['PASS', 'XPASS']
    fail_statuses = ['FAIL', 'XFAIL', 'UNSUPPORTED']
    name_count = {}
    for line in log_file:
      line = line.strip().split(':')
      if len(line) > 1 and (line[0] in pass_statuses or
                            line[0] in fail_statuses):
        test_name = (':'.join(line[1:])).replace('\t', ' ').strip()
        count = name_count.get(test_name, 0) + 1
        name_count[test_name] = count
        test_name = '%s (%s)' % (test_name, str(count))
        if line[0] in pass_statuses:
          test_result = 'pass'
        else:
          test_result = 'fail'
        result += '%s\t%s\t%s\n' % (test_name, test_result, filename)
    return result


class PerflabSummarizer(object):
  """Perflab Summarizer class"""

  def __init__(self):
    pass

  def Matches(self, log_file):
    p = re.compile('METRIC isolated \w+')
    for log_line in log_file:
      if p.search(log_line):
        return True
    return False

  def Summarize(self, log_file, filename):
    result = ''
    p = re.compile("METRIC isolated (\w+) .*\['(.*?)'\]")
    log_file_lines = '\n'.join(log_file)
    matches = p.findall(log_file_lines)
    for match in matches:
      if len(match) != 2:
        continue
      result += '%s\t%s\t%s\n' % (match[0], match[1], filename)
    return result


class AutoTestSummarizer(object):
  """AutoTest Summarizer class"""

  def __init__(self):
    pass

  def Matches(self, log_file):
    for log_line in log_file:
      if log_line.find("""Installing autotest on""") > -1:
        return True
    return False

  def Summarize(self, log_file, filename):
    result = ''
    pass_statuses = ['PASS']
    fail_statuses = ['FAIL']
    for line in log_file:
      line = line.strip().split(' ')
      if len(line) > 1 and (line[-1].strip() in pass_statuses or
                            line[-1].strip() in fail_statuses):
        test_name = (line[0].strip())
        if line[-1].strip() in pass_statuses:
          test_result = 'pass'
        else:
          test_result = 'fail'
        result += '%s\t%s\t%s\n' % (test_name, test_result, filename)
    return result


def Usage():
  print('Usage: %s log_file' % sys.argv[0])
  sys.exit(1)


def SummarizeFile(filename):
  summarizers = [DejaGNUSummarizer(), AutoTestSummarizer(), PerflabSummarizer()]
  inp = open(filename, 'rb')
  executer = command_executer.GetCommandExecuter()
  for summarizer in summarizers:
    inp.seek(0)
    if summarizer.Matches(inp):
      executer.CopyFiles(filename, RESULTS_DIR, recursive=False)
      inp.seek(0)
      result = summarizer.Summarize(inp, os.path.basename(filename))
      inp.close()
      return result
  inp.close()
  return None


def Main(argv):
  if len(argv) != 2:
    Usage()
  filename = argv[1]

  executer = command_executer.GetCommandExecuter()
  executer.RunCommand('mkdir -p %s' % RESULTS_DIR)
  summary = SummarizeFile(filename)
  if summary is not None:
    output = open(RESULTS_FILE, 'a')
    output.write(summary.strip() + '\n')
    output.close()
  return 0


if __name__ == '__main__':
  retval = Main(sys.argv)
  sys.exit(retval)
