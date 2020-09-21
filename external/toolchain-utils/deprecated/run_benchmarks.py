#!/usr/bin/python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Script to run ChromeOS benchmarks

Inputs:
    chromeos_root
    board
    [chromeos/cpu/<benchname>|
     chromeos/browser/[pagecycler|sunspider]|
     chromeos/startup]
    hostname/IP of Chromeos machine

    chromeos/cpu/<benchname>
       - Read run script rules from bench.mk perflab-bin, copy benchmark to
       host, run
       and return results.

    chromeos/startup
       - Re-image host with image in perflab-bin
       - Call run_tests to run startup test, gather results.
       - Restore host back to what it was.

    chromeos/browser/*
       - Call build_chromebrowser to build image with new browser
       - Copy image to perflab-bin

"""

from __future__ import print_function

__author__ = 'bjanakiraman@google.com (Bhaskar Janakiraman)'

import argparse
import os
import re
import sys

import image_chromeos
import run_tests
from cros_utils import command_executer
from cros_utils import logger

# pylint: disable=anomalous-backslash-in-string

KNOWN_BENCHMARKS = [
    'chromeos/startup', 'chromeos/browser/pagecycler',
    'chromeos/browser/sunspider', 'chromeos/browser/v8bench',
    'chromeos/cpu/bikjmp'
]

name_map = {
    'pagecycler': 'Page',
    'sunspider': 'SunSpider',
    'v8bench': 'V8Bench',
    'startup': 'BootPerfServer'
}

# Run command template

# Common initializations
cmd_executer = command_executer.GetCommandExecuter()


def Usage(parser, message):
  print('ERROR: %s' % message)
  parser.print_help()
  sys.exit(0)


def RunBrowserBenchmark(chromeos_root, board, bench, machine):
  """Run browser benchmarks.

  Args:
    chromeos_root: ChromeOS src dir
    board: Board being tested
    bench: Name of benchmark (chromeos/browser/*)
    machine: name of chromeos machine
  """
  benchname = re.split('/', bench)[2]
  benchname = name_map[benchname]
  ret = run_tests.RunRemoteTests(chromeos_root, machine, board, benchname)
  return ret


def RunStartupBenchmark(chromeos_root, board, machine):
  """Run browser benchmarks.

  Args:
    chromeos_root: ChromeOS src dir
    board: Board being tested
    machine: name of chromeos machine
  """
  benchname = 'startup'
  benchname = name_map[benchname]
  ret = run_tests.RunRemoteTests(chromeos_root, machine, board, benchname)
  return ret


def RunCpuBenchmark(chromeos_root, bench, workdir, machine):
  """Run CPU benchmark.

  Args:
    chromeos_root: ChromeOS src dir
    bench: Name of benchmark
    workdir: directory containing benchmark directory
    machine: name of chromeos machine

  Returns:
    status: 0 on success
  """

  benchname = re.split('/', bench)[2]
  benchdir = '%s/%s' % (workdir, benchname)

  # Delete any existing run directories on machine.
  # Since this has exclusive access to the machine,
  # we do not worry about duplicates.
  args = 'rm -rf /tmp/%s' % benchname
  retv = cmd_executer.CrosRunCommand(args,
                                     chromeos_root=chromeos_root,
                                     machine=machine)
  if retv:
    return retv

  # Copy benchmark directory.
  retv = cmd_executer.CopyFiles(benchdir,
                                '/tmp/' + benchname,
                                chromeos_root=chromeos_root,
                                dest_machine=machine,
                                dest_cros=True)
  if retv:
    return retv

  # Parse bench.mk to extract run flags.

  benchmk_file = open('%s/bench.mk' % benchdir, 'r')
  for line in benchmk_file:
    line.rstrip()
    if re.match('^run_cmd', line):
      line = re.sub('^run_cmd.*\${PERFLAB_PATH}', './out', line)
      line = re.sub('\${PERFLAB_INPUT}', './data', line)
      run_cmd = line
      break

  # Execute on remote machine
  # Capture output and process it.
  sshargs = "'cd /tmp/%s;" % benchname
  sshargs += "time -p %s'" % run_cmd
  cmd_executer.CrosRunCommand(sshargs,
                              chromeos_root=chromeos_root,
                              machine=machine)

  return retv


def Main(argv):
  """Build ChromeOS."""
  # Common initializations

  parser = argparse.ArgumentParser()
  parser.add_argument('-c',
                      '--chromeos_root',
                      dest='chromeos_root',
                      help='Target directory for ChromeOS installation.')
  parser.add_argument('-m',
                      '--machine',
                      dest='machine',
                      help='The chromeos host machine.')
  parser.add_argument('--workdir',
                      dest='workdir',
                      default='./perflab-bin',
                      help='Work directory for perflab outputs.')
  parser.add_argument('--board',
                      dest='board',
                      help='ChromeOS target board, e.g. x86-generic')
  parser.add_argumen('args', margs='+', help='Benchmarks to run.')

  options = parser.parse_args(argv[1:])

  # validate args
  for arg in options.args:
    if arg not in KNOWN_BENCHMARKS:
      logger.GetLogger().LogFatal('Bad benchmark %s specified' % arg)

  if options.chromeos_root is None:
    Usage(parser, '--chromeos_root must be set')

  if options.board is None:
    Usage(parser, '--board must be set')

  if options.machine is None:
    Usage(parser, '--machine must be set')

  found_err = 0
  retv = 0
  for arg in options.args:
    # CPU benchmarks
    comps = re.split('/', arg)
    if re.match('chromeos/cpu', arg):
      benchname = comps[2]
      print('RUNNING %s' % benchname)
      retv = RunCpuBenchmark(options.chromeos_root, arg, options.workdir,
                             options.machine)
      if not found_err:
        found_err = retv
    elif re.match('chromeos/startup', arg):
      benchname = comps[1]
      image_args = [
          os.path.dirname(os.path.abspath(__file__)) + '/image_chromeos.py',
          '--chromeos_root=' + options.chromeos_root,
          '--remote=' + options.machine, '--image=' + options.workdir + '/' +
          benchname + '/chromiumos_image.bin'
      ]
      logger.GetLogger().LogOutput('Reimaging machine %s' % options.machine)
      image_chromeos.Main(image_args)

      logger.GetLogger().LogOutput('Running %s' % arg)
      retv = RunStartupBenchmark(options.chromeos_root, options.board,
                                 options.machine)
      if not found_err:
        found_err = retv
    elif re.match('chromeos/browser', arg):
      benchname = comps[2]
      image_args = [
          os.path.dirname(os.path.abspath(__file__)) + '/image_chromeos.py',
          '--chromeos_root=' + options.chromeos_root,
          '--remote=' + options.machine, '--image=' + options.workdir + '/' +
          benchname + '/chromiumos_image.bin'
      ]
      logger.GetLogger().LogOutput('Reimaging machine %s' % options.machine)
      image_chromeos.Main(image_args)

      logger.GetLogger().LogOutput('Running %s' % arg)
      retv = RunBrowserBenchmark(options.chromeos_root, options.board, arg,
                                 options.machine)
      if not found_err:
        found_err = retv

  return found_err


if __name__ == '__main__':
  retval = Main(sys.argv)
  sys.exit(retval)
