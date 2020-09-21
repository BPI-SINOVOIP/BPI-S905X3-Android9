#!/usr/bin/env python2
#
# Copyright 2013 Google Inc. All Rights Reserved.
"""Script to maintain the Telemetry benchmark default results file.

This script allows the user to see and update the set of default
results to be used in generating reports from running the Telemetry
benchmarks.
"""

from __future__ import print_function

__author__ = 'cmtice@google.com (Caroline Tice)'

import os
import sys
import json

from cros_utils import misc

Defaults = {}


class TelemetryDefaults(object):
  """Class for handling telemetry default return result fields."""

  DEFAULTS_FILE_NAME = 'crosperf/default-telemetry-results.json'

  def __init__(self):
    # Get the Crosperf directory; that is where the defaults
    # file should be.
    dirname, __ = misc.GetRoot(__file__)
    fullname = os.path.join(dirname, self.DEFAULTS_FILE_NAME)
    self._filename = fullname
    self._defaults = {}

  def ReadDefaultsFile(self):
    if os.path.exists(self._filename):
      with open(self._filename, 'r') as fp:
        self._defaults = json.load(fp)

  def WriteDefaultsFile(self):
    with open(self._filename, 'w') as fp:
      json.dump(self._defaults, fp, indent=2)

  def ListCurrentDefaults(self, benchmark=all):
    # Show user current defaults. By default, show all.  The user
    # can specify the name of a particular benchmark to see only that
    # benchmark's default values.
    if len(self._defaults) == 0:
      print('The benchmark default results are currently empty.')
    if benchmark == all:
      for b in self._defaults.keys():
        results = self._defaults[b]
        out_str = b + ' : '
        for r in results:
          out_str += r + ' '
        print(out_str)
    elif benchmark in self._defaults:
      results = self._defaults[benchmark]
      out_str = benchmark + ' : '
      for r in results:
        out_str += r + ' '
      print(out_str)
    else:
      print("Error:  Unrecognized benchmark '%s'" % benchmark)

  def AddDefault(self, benchmark, result):
    if benchmark in self._defaults:
      resultList = self._defaults[benchmark]
    else:
      resultList = []
    resultList.append(result)
    self._defaults[benchmark] = resultList
    print("Updated results set for '%s': " % benchmark)
    print('%s : %s' % (benchmark, repr(self._defaults[benchmark])))

  def RemoveDefault(self, benchmark, result):
    if benchmark in self._defaults:
      resultList = self._defaults[benchmark]
      if result in resultList:
        resultList.remove(result)
        print("Updated results set for '%s': " % benchmark)
        print('%s : %s' % (benchmark, repr(self._defaults[benchmark])))
      else:
        print("'%s' is not in '%s's default results list." % (result,
                                                              benchmark))
    else:
      print("Cannot find benchmark named '%s'" % benchmark)

  def GetDefault(self):
    return self._defaults

  def RemoveBenchmark(self, benchmark):
    if benchmark in self._defaults:
      del self._defaults[benchmark]
      print("Deleted benchmark '%s' from list of benchmarks." % benchmark)
    else:
      print("Cannot find benchmark named '%s'" % benchmark)

  def RenameBenchmark(self, old_name, new_name):
    if old_name in self._defaults:
      resultsList = self._defaults[old_name]
      del self._defaults[old_name]
      self._defaults[new_name] = resultsList
      print("Renamed '%s' to '%s'." % (old_name, new_name))
    else:
      print("Cannot find benchmark named '%s'" % old_name)

  def UsageError(self, user_input):
    # Print error message, then show options
    print("Error:Invalid user input: '%s'" % user_input)
    self.ShowOptions()

  def ShowOptions(self):
    print("""
Below are the valid user options and their arguments, and an explanation
of what each option does.  You may either print out the full name of the
option, or you may use the first letter of the option.  Case (upper or
lower) does not matter, for the command (case of the result name DOES matter):

    (L)ist                           - List all current defaults
    (L)ist <benchmark>               - List current defaults for benchmark
    (H)elp                           - Show this information.
    (A)dd <benchmark> <result>       - Add a default result for a particular
                                       benchmark (appends to benchmark's list
                                       of results, if list already exists)
    (D)elete <benchmark> <result>    - Delete a default result for a
                                       particular benchmark
    (R)emove <benchmark>             - Remove an entire benchmark (and its
                                       results)
    (M)ove <old-benchmark> <new-benchmark>    - Rename a benchmark
    (Q)uit                           - Exit this program, saving changes.
    (T)erminate                      - Exit this program; abandon changes.

""")

  def GetUserInput(self):
    # Prompt user
    print('Enter option> ')
    # Process user input
    inp = sys.stdin.readline()
    inp = inp[:-1]
    # inp = inp.lower()
    words = inp.split(' ')
    option = words[0]
    option = option.lower()
    if option == 'h' or option == 'help':
      self.ShowOptions()
    elif option == 'l' or option == 'list':
      if len(words) == 1:
        self.ListCurrentDefaults()
      else:
        self.ListCurrentDefaults(benchmark=words[1])
    elif option == 'a' or option == 'add':
      if len(words) < 3:
        self.UsageError(inp)
      else:
        benchmark = words[1]
        resultList = words[2:]
        for r in resultList:
          self.AddDefault(benchmark, r)
    elif option == 'd' or option == 'delete':
      if len(words) != 3:
        self.UsageError(inp)
      else:
        benchmark = words[1]
        result = words[2]
        self.RemoveDefault(benchmark, result)
    elif option == 'r' or option == 'remove':
      if len(words) != 2:
        self.UsageError(inp)
      else:
        benchmark = words[1]
        self.RemoveBenchmark(benchmark)
    elif option == 'm' or option == 'move':
      if len(words) != 3:
        self.UsageError(inp)
      else:
        old_name = words[1]
        new_name = words[2]
        self.RenameBenchmark(old_name, new_name)
    elif option == 'q' or option == 'quit':
      self.WriteDefaultsFile()

    return (option == 'q' or option == 'quit' or option == 't' or
            option == 'terminate')


def Main():
  defaults = TelemetryDefaults()
  defaults.ReadDefaultsFile()
  defaults.ShowOptions()
  done = defaults.GetUserInput()
  while not done:
    done = defaults.GetUserInput()
  return 0


if __name__ == '__main__':
  retval = Main()
  sys.exit(retval)
