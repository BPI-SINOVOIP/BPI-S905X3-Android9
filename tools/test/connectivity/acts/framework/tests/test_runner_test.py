#!/usr/bin/env python3
#
#   Copyright 2017 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from mock import Mock
import unittest
import tempfile

from acts import keys
from acts import test_runner

import mock_controller


class TestRunnerTest(unittest.TestCase):
    def setUp(self):
        self.tmp_dir = tempfile.mkdtemp()
        self.base_mock_test_config = {
            "testbed": {
                "name": "SampleTestBed",
            },
            "logpath": self.tmp_dir,
            "cli_args": None,
            "testpaths": ["./"],
            "icecream": 42,
            "extra_param": "haha"
        }

    def create_mock_context(self):
        context = Mock()
        context.__exit__ = Mock()
        context.__enter__ = Mock()
        return context

    def create_test_classes(self, class_names):
        return {
            class_name: Mock(return_value=self.create_mock_context())
            for class_name in class_names
        }

    def test_class_name_pattern_single(self):
        class_names = ['test_class_1', 'test_class_2']
        pattern = 'test*1'
        tr = test_runner.TestRunner(self.base_mock_test_config, [(pattern,
                                                                  None)])

        test_classes = self.create_test_classes(class_names)
        tr.import_test_modules = Mock(return_value=test_classes)
        tr.run()
        self.assertTrue(test_classes[class_names[0]].called)
        self.assertFalse(test_classes[class_names[1]].called)

    def test_class_name_pattern_multi(self):
        class_names = ['test_class_1', 'test_class_2', 'other_name']
        pattern = 'test_class*'
        tr = test_runner.TestRunner(self.base_mock_test_config, [(pattern,
                                                                  None)])

        test_classes = self.create_test_classes(class_names)
        tr.import_test_modules = Mock(return_value=test_classes)
        tr.run()
        self.assertTrue(test_classes[class_names[0]].called)
        self.assertTrue(test_classes[class_names[1]].called)
        self.assertFalse(test_classes[class_names[2]].called)

    def test_class_name_pattern_question_mark(self):
        class_names = ['test_class_1', 'test_class_12']
        pattern = 'test_class_?'
        tr = test_runner.TestRunner(self.base_mock_test_config, [(pattern,
                                                                  None)])

        test_classes = self.create_test_classes(class_names)
        tr.import_test_modules = Mock(return_value=test_classes)
        tr.run()
        self.assertTrue(test_classes[class_names[0]].called)
        self.assertFalse(test_classes[class_names[1]].called)

    def test_class_name_pattern_char_seq(self):
        class_names = ['test_class_1', 'test_class_2', 'test_class_3']
        pattern = 'test_class_[1357]'
        tr = test_runner.TestRunner(self.base_mock_test_config, [(pattern,
                                                                  None)])

        test_classes = self.create_test_classes(class_names)
        tr.import_test_modules = Mock(return_value=test_classes)
        tr.run()
        self.assertTrue(test_classes[class_names[0]].called)
        self.assertFalse(test_classes[class_names[1]].called)
        self.assertTrue(test_classes[class_names[2]].called)


if __name__ == "__main__":
    unittest.main()
