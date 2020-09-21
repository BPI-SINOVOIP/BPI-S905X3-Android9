#!/usr/bin/python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Script to compare ChromeOS benchmarks

Inputs:
    <perflab-output directory 1 - baseline>
    <perflab-output directory 2 - results>
    --csv - comma separated results

This script doesn't really know much about benchmarks. It simply looks for
similarly names directories and a results.txt file, and compares
the results and presents it, along with a geometric mean.

"""

from __future__ import print_function

__author__ = 'bjanakiraman@google.com (Bhaskar Janakiraman)'

import glob
import math
import argparse
import re
import sys

from cros_utils import command_executer

BENCHDIRS = ('%s/default/default/*/gcc-4.4.3-glibc-2.11.1-grte-k8-opt/ref/*'
             '/results.txt')

# Common initializations
cmd_executer = command_executer.GetCommandExecuter()


def Usage(parser, message):
  print('ERROR: %s' % message)
  parser.print_help()
  sys.exit(0)


def GetStats(in_file):
  """Return stats from file"""
  f = open(in_file, 'r')
  pairs = []
  for l in f:
    line = l.strip()
    # Look for match lines like the following:
    #       METRIC isolated TotalTime_ms (down, scalar) trial_run_0: ['1524.4']
    #       METRIC isolated isolated_walltime (down, scalar) trial_run_0: \
    #         ['167.407445192']
    m = re.match(r"METRIC\s+isolated\s+(\S+).*\['(\d+(?:\.\d+)?)'\]", line)
    if not m:
      continue
    metric = m.group(1)
    if re.match(r'isolated_walltime', metric):
      continue

    value = float(m.group(2))
    pairs.append((metric, value))

  return dict(pairs)


def PrintDash(n):
  tmpstr = ''
  for _ in range(n):
    tmpstr += '-'
  print(tmpstr)


def PrintHeaderCSV(hdr):
  tmpstr = ''
  for i in range(len(hdr)):
    if tmpstr != '':
      tmpstr += ','
    tmpstr += hdr[i]
  print(tmpstr)


def PrintHeader(hdr):
  tot_len = len(hdr)
  PrintDash(tot_len * 15)

  tmpstr = ''
  for i in range(len(hdr)):
    tmpstr += '%15.15s' % hdr[i]

  print(tmpstr)
  PrintDash(tot_len * 15)


def Main(argv):
  """Compare Benchmarks."""
  # Common initializations

  parser = argparse.ArgumentParser()
  parser.add_argument('-c',
                      '--csv',
                      dest='csv_output',
                      action='store_true',
                      default=False,
                      help='Output in csv form.')
  parser.add_argument('args', nargs='+', help='positional arguments: '
                      '<baseline-output-dir> <results-output-dir>')

  options = parser.parse_args(argv[1:])

  # validate args
  if len(options.args) != 2:
    Usage(parser, 'Needs <baseline output dir> <results output dir>')

  base_dir = options.args[0]
  res_dir = options.args[1]

  # find res benchmarks that have results
  resbenches_glob = BENCHDIRS % res_dir
  resbenches = glob.glob(resbenches_glob)

  basebenches_glob = BENCHDIRS % base_dir
  basebenches = glob.glob(basebenches_glob)

  to_compare = []
  for resbench in resbenches:
    tmp = resbench.replace(res_dir, base_dir, 1)
    if tmp in basebenches:
      to_compare.append((resbench, tmp))

  for (resfile, basefile) in to_compare:
    stats = GetStats(resfile)
    basestats = GetStats(basefile)
    # Print a header
    # benchname (remove results.txt), basetime, restime, %speed-up
    hdr = []
    benchname = re.split('/', resfile)[-2:-1][0]
    benchname = benchname.replace('chromeos__', '', 1)
    hdr.append(benchname)
    hdr.append('basetime')
    hdr.append('restime')
    hdr.append('%speed up')
    if options.csv_output:
      PrintHeaderCSV(hdr)
    else:
      PrintHeader(hdr)

    # For geomean computations
    prod = 1.0
    count = 0
    for key in stats.keys():
      if key in basestats.keys():
        # ignore very small values.
        if stats[key] < 0.01:
          continue
        count = count + 1
        prod = prod * (stats[key] / basestats[key])
        speedup = (basestats[key] - stats[key]) / basestats[key]
        speedup = speedup * 100.0
        if options.csv_output:
          print('%s,%f,%f,%f' % (key, basestats[key], stats[key], speedup))
        else:
          print('%15.15s%15.2f%15.2f%14.2f%%' % (key, basestats[key],
                                                 stats[key], speedup))

    prod = math.exp(1.0 / count * math.log(prod))
    prod = (1.0 - prod) * 100
    if options.csv_output:
      print('%s,,,%f' % ('Geomean', prod))
    else:
      print('%15.15s%15.15s%15.15s%14.2f%%' % ('Geomean', '', '', prod))
      print('')
  return 0


if __name__ == '__main__':
  retval = Main(sys.argv)
  sys.exit(retval)
