#!/usr/bin/python
# -*- coding:utf-8 -*-
# Copyright 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Unittests for the config module."""

from __future__ import print_function

import os
import shutil
import sys
import tempfile
import unittest

_path = os.path.realpath(__file__ + '/../..')
if sys.path[0] != _path:
    sys.path.insert(0, _path)
del _path

# We have to import our local modules after the sys.path tweak.  We can't use
# relative imports because this is an executable program, not a module.
# pylint: disable=wrong-import-position
import rh.hooks
import rh.config


class PreSubmitConfigTests(unittest.TestCase):
    """Tests for the PreSubmitConfig class."""

    def setUp(self):
        self.tempdir = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self.tempdir)

    def _write_config(self, data, filename=None):
        """Helper to write out a config file for testing."""
        if filename is None:
            filename = rh.config.PreSubmitConfig.FILENAME
        path = os.path.join(self.tempdir, filename)
        with open(path, 'w') as fp:
            fp.write(data)

    def _write_global_config(self, data):
        """Helper to write out a global config file for testing."""
        self._write_config(
            data, filename=rh.config.PreSubmitConfig.GLOBAL_FILENAME)

    def testMissing(self):
        """Instantiating a non-existent config file should be fine."""
        rh.config.PreSubmitConfig()

    def testEmpty(self):
        """Instantiating an empty config file should be fine."""
        self._write_config('')
        rh.config.PreSubmitConfig(paths=(self.tempdir,))

    def testValid(self):
        """Verify a fully valid file works."""
        self._write_config("""# This be a comment me matey.
[Hook Scripts]
name = script --with "some args"

[Builtin Hooks]
cpplint = true

[Builtin Hooks Options]
cpplint = --some 'more args'

[Options]
ignore_merged_commits = true
""")
        rh.config.PreSubmitConfig(paths=(self.tempdir,))

    def testUnknownSection(self):
        """Reject unknown sections."""
        self._write_config('[BOOGA]')
        self.assertRaises(rh.config.ValidationError, rh.config.PreSubmitConfig,
                          paths=(self.tempdir,))

    def testUnknownBuiltin(self):
        """Reject unknown builtin hooks."""
        self._write_config('[Builtin Hooks]\nbooga = borg!')
        self.assertRaises(rh.config.ValidationError, rh.config.PreSubmitConfig,
                          paths=(self.tempdir,))

    def testEmptyCustomHook(self):
        """Reject empty custom hooks."""
        self._write_config('[Hook Scripts]\nbooga = \t \n')
        self.assertRaises(rh.config.ValidationError, rh.config.PreSubmitConfig,
                          paths=(self.tempdir,))

    def testInvalidIni(self):
        """Reject invalid ini files."""
        self._write_config('[Hook Scripts]\n =')
        self.assertRaises(rh.config.ValidationError, rh.config.PreSubmitConfig,
                          paths=(self.tempdir,))

    def testInvalidString(self):
        """Catch invalid string quoting."""
        self._write_config("""[Hook Scripts]
name = script --'bad-quotes
""")
        self.assertRaises(rh.config.ValidationError, rh.config.PreSubmitConfig,
                          paths=(self.tempdir,))

    def testGlobalConfigs(self):
        """Verify global configs stack properly."""
        self._write_global_config("""[Builtin Hooks]
commit_msg_bug_field = true
commit_msg_changeid_field = true
commit_msg_test_field = false""")
        self._write_config("""[Builtin Hooks]
commit_msg_bug_field = false
commit_msg_test_field = true""")
        config = rh.config.PreSubmitConfig(paths=(self.tempdir,),
                                           global_paths=(self.tempdir,))
        self.assertEqual(config.builtin_hooks,
                         ['commit_msg_changeid_field', 'commit_msg_test_field'])


if __name__ == '__main__':
    unittest.main()
