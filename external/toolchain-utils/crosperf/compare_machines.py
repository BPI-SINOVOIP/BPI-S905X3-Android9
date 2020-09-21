# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Module to compare two machines."""

from __future__ import print_function

import os.path
import sys
import argparse

from machine_manager import CrosMachine


def PrintUsage(msg):
  print(msg)
  print('Usage: ')
  print('\n compare_machines.py --chromeos_root=/path/to/chroot/ '
        'machine1 machine2 ...')


def Main(argv):

  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--chromeos_root',
      default='/path/to/chromeos',
      dest='chromeos_root',
      help='ChromeOS root checkout directory')
  parser.add_argument('remotes', nargs=argparse.REMAINDER)

  options = parser.parse_args(argv)

  machine_list = options.remotes
  if len(machine_list) < 2:
    PrintUsage('ERROR: Must specify at least two machines.')
    return 1
  elif not os.path.exists(options.chromeos_root):
    PrintUsage('Error: chromeos_root does not exist %s' % options.chromeos_root)
    return 1

  chroot = options.chromeos_root
  cros_machines = []
  test_machine_checksum = None
  for m in machine_list:
    cm = CrosMachine(m, chroot, 'average')
    cros_machines = cros_machines + [cm]
    test_machine_checksum = cm.machine_checksum

  ret = 0
  for cm in cros_machines:
    print('checksum for %s : %s' % (cm.name, cm.machine_checksum))
    if cm.machine_checksum != test_machine_checksum:
      ret = 1
      print('Machine checksums do not all match')

  if ret == 0:
    print('Machines all match.')

  return ret


if __name__ == '__main__':
  retval = Main(sys.argv[1:])
  sys.exit(retval)
