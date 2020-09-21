#!/usr/bin/env python
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Unittest for ini_editor.py."""


import logging
import os
import re
import tempfile
import unittest

from ini_editor import Ini

SAMPLE_INI="""\
[output_source]
library=builtin
label=source
purpose=playback
disable=(equal? output_jack "HDMI")
output_0={src:0}
output_1={src:1}

[output_sink]
library=builtin
label=sink
purpose=playback
disable=(equal? output_jack "HDMI")
input_0={dst:0}
input_1={dst:1}

[drc]
library=builtin
label=drc
input_0={src:0}
input_1={src:1}
output_2={intermediate:0}
output_3={intermediate:1}
input_4=0       ; f
input_5=0       ; enable
input_6=-29     ; threshold
input_7=3       ; knee
input_8=6.677   ; ratio
input_9=0.02    ; attack
input_10=0.2     ; release
input_11=-7      ; boost

[eq2]
library=builtin
label=eq2
input_0={intermediate:0}
input_1={intermediate:1}
output_2={dst:0}
output_3={dst:1}
input_4=6       ; peaking
input_5=380     ; freq
input_6=3       ; Q
input_7=-10     ; gain
input_8=6       ; peaking
input_9=450     ; freq
input_10=3       ; Q
input_11=-12     ; gain
"""

SAMPLE_INI_DRC="""\
[drc]
library=builtin
label=drc
input_0={src:0}
input_1={src:1}
output_2={intermediate:0}
output_3={intermediate:1}
input_4=0       ; f
input_5=0       ; enable
input_6=-29     ; threshold
input_7=3       ; knee
input_8=6.677   ; ratio
input_9=0.02    ; attack
input_10=0.2     ; release
input_11=-7      ; boost

"""

SAMPLE_INI_DRC_INSERTED="""\
[drc]
library=builtin
label=drc
input_0={src:0}
input_1={src:1}
output_2={intermediate:0}
output_3={intermediate:1}
input_4=1       ; new_parameter
input_5=0       ; f
input_6=0       ; enable
input_7=-29     ; threshold
input_8=3       ; knee
input_9=6.677   ; ratio
input_10=0.02    ; attack
input_11=0.2     ; release
input_12=-7      ; boost

"""


class IniTest(unittest.TestCase):
  """Unittest for Ini class."""
  def setUp(self):
    self.ini_file = tempfile.NamedTemporaryFile(prefix='ini_editor_unittest')
    with open(self.ini_file.name, 'w') as f:
      f.write(SAMPLE_INI)
    self.ini = Ini(self.ini_file.name)

  def tearDown(self):
    self.ini_file.close()

  def testPrint(self):
    """Unittest for Print method of Ini class."""
    output = self.ini.Print()
    self.assertEqual(output.getvalue(), SAMPLE_INI)

  def testHasSection(self):
    """Unittest for HasSection method of Ini class."""
    self.assertTrue(self.ini.HasSection('drc'))
    self.assertFalse(self.ini.HasSection('eq1'))

  def testPrintSection(self):
    """Unittest for PrintSection method of Ini class."""
    output = self.ini.PrintSection('drc')
    self.assertEqual(output.getvalue(), SAMPLE_INI_DRC)

  def testInsertLineToSection(self):
    """Unittest for InsertLineToSection method of Ini class."""
    self.ini.InsertLineToSection('drc', 'input_4=1       ; new_parameter')
    output = self.ini.PrintSection('drc')
    self.assertEqual(output.getvalue(), SAMPLE_INI_DRC_INSERTED)


if __name__ == '__main__':
  logging.basicConfig(
     format='%(asctime)s:%(levelname)s:%(filename)s:%(lineno)d:%(message)s',
     level=logging.DEBUG)
  unittest.main()
