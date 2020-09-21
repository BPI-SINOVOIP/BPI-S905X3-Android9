#!/usr/bin/python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Script to build ChromeOS benchmarks

Inputs:
    chromeos_root
    toolchain_root
    board
    [chromeos/cpu/<benchname> |
     chromeos/browser/[pagecycler|sunspider] |
     chromeos/startup]

    This script assumes toolchain has already been built in toolchain_root.

    chromeos/cpu/<benchname>
       - Execute bench.py script within chroot to build benchmark
       - Copy build results to perflab-bin

    chromeos/startup
       - Call build_chromeos to build image.
       - Copy image to perflab-bin

    chromeos/browser/*
       - Call build_chromebrowser to build image with new browser
       - Copy image to perflab-bin

"""

from __future__ import print_function

__author__ = 'bjanakiraman@google.com (Bhaskar Janakiraman)'

import argparse
import os
import sys
import re

import build_chromeos
from cros_utils import command_executer
from cros_utils import logger

KNOWN_BENCHMARKS = [
    'chromeos/startup', 'chromeos/browser/pagecycler',
    'chromeos/browser/sunspider', 'chromeos/browser/v8bench',
    'chromeos/cpu/bikjmp'
]

# Commands to build CPU benchmarks.

CPU_BUILDCMD_CLEAN = ('cd /usr/local/toolchain_root/third_party/android_bench/'
                      'v2_0/CLOSED_SOURCE/%s; python ../../scripts/bench.py '
                      '--toolchain=/usr/bin --action=clean;')

CPU_BUILDCMD_BUILD = ('cd /usr/local/toolchain_root/third_party/android_bench/'
                      'v2_0/CLOSED_SOURCE/%s; python ../../scripts/bench.py '
                      '--toolchain=/usr/bin --add_cflags=%s --add_ldflags=%s '
                      '--makeopts=%s --action=build')

# Common initializations
cmd_executer = command_executer.GetCommandExecuter()


def Usage(parser, message):
  print('ERROR: %s' % message)
  parser.print_help()
  sys.exit(0)


def CreateRunsh(destdir, benchmark):
  """Create run.sh script to run benchmark.

  Perflab needs a run.sh that runs the benchmark.
  """
  run_cmd = os.path.dirname(os.path.abspath(__file__)) + '/run_benchmarks.py'
  contents = '#!/bin/sh\n%s $@ %s\n' % (run_cmd, benchmark)
  runshfile = destdir + '/run.sh'
  f = open(runshfile, 'w')
  f.write(contents)
  f.close()
  retval = cmd_executer.RunCommand('chmod +x %s' % runshfile)
  return retval


def CreateBinaryCopy(sourcedir, destdir, copy=None):
  """Create links in perflab-bin/destdir/* to sourcedir/*, instead of copies

  Args:
    sourcedir: directory from which to copy.
    destdir: directory to which to copy.
    copy: when none, make soft links to everything under sourcedir, otherwise
          copy all to destdir.
          TODO: remove this parameter if it's determined that CopyFiles can use
                rsync -L.
  """
  retval = 0
  # check if sourcedir exists
  if not os.path.exists(sourcedir):
    logger.GetLogger().LogError('benchmark results %s does not exist.' %
                                sourcedir)
    return 1

  # Deal with old copies - save off old ones for now.
  # Note - if its a link, it doesn't save anything.
  if os.path.exists(destdir):
    command = 'rm -rf %s.old' % destdir
    retval = cmd_executer.RunCommand(command)
    if retval != 0:
      return retval
    command = 'mv %s %s.old' % (destdir, destdir)
    retval = cmd_executer.RunCommand(command)
    if retval != 0:
      return retval
  os.makedirs(destdir)
  sourcedir = os.path.abspath(sourcedir)
  if copy is None:
    command = 'ln -s %s/* %s' % (sourcedir, destdir)
  else:
    command = 'cp -fr %s/* %s' % (sourcedir, destdir)
  retval = cmd_executer.RunCommand(command)
  return retval


def Main(argv):
  """Build ChromeOS."""
  # Common initializations

  parser = argparse.ArgumentParser()
  parser.add_argument('-c',
                      '--chromeos_root',
                      dest='chromeos_root',
                      help='Target directory for ChromeOS installation.')
  parser.add_argument('-t',
                      '--toolchain_root',
                      dest='toolchain_root',
                      help='This is obsolete. Do not use.')
  parser.add_argument('-r',
                      '--third_party',
                      dest='third_party',
                      help='The third_party dir containing android '
                           'benchmarks.')
  parser.add_argument('-C',
                      '--clean',
                      dest='clean',
                      action='store_true',
                      default=False,
                      help='Clean up build.')
  parser.add_argument('-B',
                      '--build',
                      dest='build',
                      action='store_true',
                      default=False,
                      help='Build benchmark.')
  parser.add_argument('-O',
                      '--only_copy',
                      dest='only_copy',
                      action='store_true',
                      default=False,
                      help='Only copy to perflab-bin - no builds.')
  parser.add_argument('--workdir',
                      dest='workdir',
                      default='.',
                      help='Work directory for perflab outputs.')
  parser.add_argument('--clobber_chroot',
                      dest='clobber_chroot',
                      action='store_true',
                      help='Delete the chroot and start fresh',
                      default=False)
  parser.add_argument('--clobber_board',
                      dest='clobber_board',
                      action='store_true',
                      help='Delete the board and start fresh',
                      default=False)
  parser.add_argument('--cflags',
                      dest='cflags',
                      default='',
                      help='CFLAGS for the ChromeOS packages')
  parser.add_argument('--cxxflags',
                      dest='cxxflags',
                      default='',
                      help='CXXFLAGS for the ChromeOS packages')
  parser.add_argument('--ldflags',
                      dest='ldflags',
                      default='',
                      help='LDFLAGS for the ChromeOS packages')
  parser.add_argument('--makeopts',
                      dest='makeopts',
                      default='',
                      help='Make options for the ChromeOS packages')
  parser.add_argument('--board',
                      dest='board',
                      help='ChromeOS target board, e.g. x86-generic')
  # Leftover positional arguments
  parser.add_argument('args', nargs='+', help='benchmarks')

  options = parser.parse_args(argv[1:])

  # validate args
  for arg in options.args:
    if arg not in KNOWN_BENCHMARKS:
      logger.GetLogger().LogFatal('Bad benchmark %s specified' % arg)

  if options.chromeos_root is None:
    Usage(parser, '--chromeos_root must be set')

  if options.board is None:
    Usage(parser, '--board must be set')

  if options.toolchain_root:
    logger.GetLogger().LogWarning('--toolchain_root should not be set')

  options.chromeos_root = os.path.expanduser(options.chromeos_root)
  options.workdir = os.path.expanduser(options.workdir)

  retval = 0
  if options.third_party:
    third_party = options.third_party
  else:
    third_party = '%s/../../../third_party' % os.path.dirname(__file__)
    third_party = os.path.realpath(third_party)
  for arg in options.args:
    # CPU benchmarks
    if re.match('chromeos/cpu', arg):
      comps = re.split('/', arg)
      benchname = comps[2]

      tec_options = []
      if third_party:
        tec_options.append('--third_party=%s' % third_party)
      if options.clean:
        retval = cmd_executer.ChrootRunCommand(options.chromeos_root,
                                               CPU_BUILDCMD_CLEAN % benchname,
                                               tec_options=tec_options)
        logger.GetLogger().LogErrorIf(retval,
                                      'clean of benchmark %s failed.' % arg)
      if options.build:
        retval = cmd_executer.ChrootRunCommand(
            options.chromeos_root,
            CPU_BUILDCMD_BUILD % (benchname, options.cflags, options.ldflags,
                                  options.makeopts),
            tec_options=tec_options)
        logger.GetLogger().LogErrorIf(retval,
                                      'Build of benchmark %s failed.' % arg)
      if retval == 0 and (options.build or options.only_copy):
        benchdir = ('%s/android_bench/v2_0/CLOSED_SOURCE/%s' %
                    (third_party, benchname))
        linkdir = '%s/perflab-bin/%s' % (options.workdir, arg)

        # For cpu/*, we need to copy (not symlinks) of all the contents,
        # because they are part of the test fixutre.
        retval = CreateBinaryCopy(benchdir, linkdir, True)
        if retval != 0:
          return retval
        retval = CreateRunsh(linkdir, arg)
        if retval != 0:
          return retval
    elif re.match('chromeos/startup', arg):
      if options.build:
        # Clean for chromeos/browser and chromeos/startup is a Nop
        # since builds are always from scratch.
        build_args = [
            os.path.dirname(os.path.abspath(__file__)) + '/build_chromeos.py',
            '--chromeos_root=' + options.chromeos_root,
            '--board=' + options.board, '--cflags=' + options.cflags,
            '--cxxflags=' + options.cxxflags, '--ldflags=' + options.ldflags,
            '--clobber_board'
        ]
        retval = build_chromeos.Main(build_args)
        logger.GetLogger().LogErrorIf(retval, 'Build of ChromeOS failed.')
      if retval == 0 and (options.build or options.only_copy):
        benchdir = '%s/src/build/images/%s/latest' % (options.chromeos_root,
                                                      options.board)
        linkdir = '%s/perflab-bin/%s' % (options.workdir, arg)
        retval = CreateBinaryCopy(benchdir, linkdir)
        if retval != 0:
          return retval
        CreateRunsh(linkdir, arg)
        if retval != 0:
          return retval
    elif re.match('chromeos/browser', arg):
      if options.build:
        # For now, re-build os. TBD: Change to call build_browser
        build_args = [os.path.dirname(os.path.abspath(__file__)) +
                      '/build_chrome_browser.py',
                      '--chromeos_root=' + options.chromeos_root,
                      '--board=' + options.board, '--cflags=' + options.cflags,
                      '--cxxflags=' + options.cxxflags,
                      '--ldflags=' + options.ldflags]
        retval = build_chromeos.Main(build_args)
        logger.GetLogger().LogErrorIf(retval, 'Build of ChromeOS failed.')
      if retval == 0 and (options.build or options.only_copy):
        benchdir = '%s/src/build/images/%s/latest' % (options.chromeos_root,
                                                      options.board)
        linkdir = '%s/perflab-bin/%s' % (options.workdir, arg)
        retval = CreateBinaryCopy(benchdir, linkdir)
        if retval != 0:
          return retval
        retval = CreateRunsh(linkdir, arg)
        if retval != 0:
          return retval

  return 0


if __name__ == '__main__':
  sys.exit(Main(sys.argv))
