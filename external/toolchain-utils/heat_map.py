#!/usr/bin/env python2
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Wrapper to generate heat maps for chrome."""

from __future__ import print_function

import argparse
import shutil
import os
import sys
import tempfile
from sets import Set

from cros_utils import command_executer


def IsARepoRoot(directory):
  """Returns True if directory is the root of a repo checkout."""
  return os.path.exists(os.path.join(directory, '.repo'))


class HeatMapProducer(object):
  """Class to produce heat map."""

  def __init__(self, chromeos_root, perf_data, page_size, binary):
    self.chromeos_root = os.path.realpath(chromeos_root)
    self.perf_data = os.path.realpath(perf_data)
    self.page_size = page_size
    self.dir = os.path.dirname(os.path.realpath(__file__))
    self.binary = binary
    self.tempDir = ''
    self.ce = command_executer.GetCommandExecuter()
    self.loading_address = None
    self.temp_perf = ''
    self.temp_perf_inchroot = ''
    self.perf_report = ''

  def copyFileToChroot(self):
    self.tempDir = tempfile.mkdtemp(prefix=os.path.join(self.chromeos_root,
                                                        'src/'))
    self.temp_perf = os.path.join(self.tempDir, 'perf.data')
    shutil.copy2(self.perf_data, self.temp_perf)
    self.temp_perf_inchroot = os.path.join('~/trunk/src',
                                           os.path.basename(self.tempDir))

  def getPerfReport(self):
    cmd = ('cd %s; perf report -D -i perf.data > perf_report.txt' %
           self.temp_perf_inchroot)
    retval = self.ce.ChrootRunCommand(self.chromeos_root, cmd)
    if retval:
      raise RuntimeError('Failed to generate perf report')
    self.perf_report = os.path.join(self.tempDir, 'perf_report.txt')

  def getBinaryBaseAddress(self):
    cmd = 'grep PERF_RECORD_MMAP %s | grep "%s$"' % (self.perf_report,
                                                     self.binary)
    retval, output, _ = self.ce.RunCommandWOutput(cmd)
    if retval:
      raise RuntimeError('Failed to run grep to get base address')
    baseAddresses = Set()
    for line in output.strip().split('\n'):
      head = line.split('[')[2]
      address = head.split('(')[0]
      baseAddresses.add(address)
    if len(baseAddresses) > 1:
      raise RuntimeError(
          'Multiple base address found, please disable ASLR and collect '
          'profile again')
    if not len(baseAddresses):
      raise RuntimeError('Could not find the base address in the profile')
    self.loading_address = baseAddresses.pop()

  def RemoveFiles(self):
    shutil.rmtree(self.tempDir)
    if os.path.isfile(os.path.join(os.getcwd(), 'out.txt')):
      os.remove(os.path.join(os.getcwd(), 'out.txt'))
    if os.path.isfile(os.path.join(os.getcwd(), 'inst-histo.txt')):
      os.remove(os.path.join(os.getcwd(), 'inst-histo.txt'))

  def getHeatmap(self):
    if not self.loading_address:
      return
    heatmap_script = os.path.join(self.dir, 'perf-to-inst-page.sh')
    cmd = '{0} {1} {2} {3} {4}'.format(heatmap_script, self.binary,
                                       self.perf_report, self.loading_address,
                                       self.page_size)
    retval = self.ce.RunCommand(cmd)
    if retval:
      raise RuntimeError('Failed to run script to generate heatmap')


def main(argv):
  """Parse the options.

  Args:
    argv: The options with which this script was invoked.

  Returns:
    0 unless an exception is raised.
  """
  parser = argparse.ArgumentParser()

  parser.add_argument(
      '--chromeos_root',
      dest='chromeos_root',
      required=True,
      help='ChromeOS root to use for generate heatmaps.')
  parser.add_argument(
      '--perf_data', dest='perf_data', required=True, help='The raw perf data.')
  parser.add_argument(
      '--binary',
      dest='binary',
      required=False,
      help='The name of the binary.',
      default='chrome')
  parser.add_argument(
      '--page_size',
      dest='page_size',
      required=False,
      help='The page size for heat maps.',
      default=4096)
  options = parser.parse_args(argv)

  if not IsARepoRoot(options.chromeos_root):
    parser.error('% does not contain .repo dir.' % options.chromeos_root)

  if not os.path.isfile(options.perf_data):
    parser.error('Cannot find perf_data: %s.' % options.perf_data)

  heatmap_producer = HeatMapProducer(options.chromeos_root, options.perf_data,
                                     options.page_size, options.binary)
  try:
    heatmap_producer.copyFileToChroot()
    heatmap_producer.getPerfReport()
    heatmap_producer.getBinaryBaseAddress()
    heatmap_producer.getHeatmap()
    print('\nheat map and time histgram genereated in the current directory '
          'with name heat_map.png and timeline.png accordingly.')
  except RuntimeError, e:
    print(e)
  finally:
    heatmap_producer.RemoveFiles()


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
