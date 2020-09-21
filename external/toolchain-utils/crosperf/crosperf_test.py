#!/usr/bin/env python2

# Copyright 2011 Google Inc. All Rights Reserved.
"""Test for crosperf."""

from __future__ import print_function

import os
import tempfile
import unittest
import crosperf
from cros_utils.file_utils import FileUtils

EXPERIMENT_FILE_1 = """
  board: x86-alex
  remote: chromeos-alex3

  benchmark: PageCycler {
    iterations: 3
  }

  image1 {
    chromeos_image: /usr/local/google/cros_image1.bin
  }

  image2 {
    chromeos_image: /usr/local/google/cros_image2.bin
  }
  """


class CrosPerfTest(unittest.TestCase):
  """Class to test Crosperf."""

  def testDryRun(self):
    filehandle, filename = tempfile.mkstemp()
    os.write(filehandle, EXPERIMENT_FILE_1)
    crosperf.Main(['', filename, '--dry_run'])
    os.remove(filename)


if __name__ == '__main__':
  FileUtils.Configure(True)
  unittest.main()
