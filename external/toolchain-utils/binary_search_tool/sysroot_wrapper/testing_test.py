#!/usr/bin/env python2
"""Test for sysroot_wrapper bisector.

All files in bad_files will be determined to be bad. This test was made for
chromeos-chrome built for a daisy board, if you are using another package you
will need to change the base_path accordingly.
"""

from __future__ import print_function

import subprocess
import sys
import os

base_path = ('/var/cache/chromeos-chrome/chrome-src-internal/src/out_daisy/'
             'Release/obj/')
bad_files = [
    os.path.join(base_path, 'base/base.cpu.o'), os.path.join(
        base_path, 'base/base.version.o'), os.path.join(base_path,
                                                        'apps/apps.launcher.o')
]

bisect_dir = os.environ.get('BISECT_DIR', '/tmp/sysroot_bisect')


def Main(_):
  for test_file in bad_files:
    test_file = test_file.strip()
    cmd = ['grep', test_file, os.path.join(bisect_dir, 'BAD_SET')]
    ret = subprocess.call(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if not ret:
      return 1
  return 0


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))
