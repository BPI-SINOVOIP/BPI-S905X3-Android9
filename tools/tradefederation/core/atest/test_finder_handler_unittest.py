#!/usr/bin/env python
#
# Copyright 2018, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Unittests for test_finder_handler."""

import unittest
import mock

import atest_error
import test_finder_handler
from test_finders import test_info
from test_finders import test_finder_base

#pylint: disable=protected-access
REF_TYPE = test_finder_handler._REFERENCE_TYPE

_EXAMPLE_FINDER_A = 'EXAMPLE_A'


#pylint: disable=no-self-use
@test_finder_base.find_method_register
class ExampleFinderA(test_finder_base.TestFinderBase):
    """Example finder class A."""
    NAME = _EXAMPLE_FINDER_A
    _TEST_RUNNER = 'TEST_RUNNER'

    @test_finder_base.register()
    def registered_find_method_from_example_finder(self, test):
        """Registered Example find method."""
        if test == 'ExampleFinderATrigger':
            return test_info.TestInfo(test_name=test,
                                      test_runner=self._TEST_RUNNER,
                                      build_targets=set())
        return None

    def unregistered_find_method_from_example_finder(self, _test):
        """Unregistered Example find method, should never be called."""
        raise atest_error.ShouldNeverBeCalledError()


_TEST_FINDERS_PATCH = {
    ExampleFinderA,
}


_FINDER_INSTANCES = {
    _EXAMPLE_FINDER_A: ExampleFinderA(),
}


class TestFinderHandlerUnittests(unittest.TestCase):
    """Unit tests for test_finder_handler.py"""

    def setUp(self):
        """Set up for testing."""
        # pylint: disable=invalid-name
        # This is so we can see the full diffs when there are mismatches.
        self.maxDiff = None
        self.empty_mod_info = None
        # We want to control the finders we return.
        mock.patch('test_finder_handler._get_test_finders',
                   lambda: _TEST_FINDERS_PATCH).start()
        # Since we're going to be comparing instance objects, we'll need to keep
        # track of the objects so they align.
        mock.patch('test_finder_handler._get_finder_instance_dict',
                   lambda x: _FINDER_INSTANCES).start()
        # We want to mock out the default find methods to make sure we got all
        # the methods we expect.
        mock.patch('test_finder_handler._get_default_find_methods',
                   lambda x, y: [test_finder_base.Finder(
                       _FINDER_INSTANCES[_EXAMPLE_FINDER_A],
                       ExampleFinderA.unregistered_find_method_from_example_finder)]).start()

    def tearDown(self):
        """Tear down."""
        mock.patch.stopall()

    def test_get_test_reference_types(self):
        """Test _get_test_reference_types parses reference types correctly."""
        self.assertEqual(
            test_finder_handler._get_test_reference_types('ModuleOrClassName'),
            [REF_TYPE.INTEGRATION, REF_TYPE.MODULE, REF_TYPE.CLASS]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('Module_or_Class_name'),
            [REF_TYPE.INTEGRATION, REF_TYPE.MODULE, REF_TYPE.CLASS]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('some.package'),
            [REF_TYPE.PACKAGE]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('fully.q.Class'),
            [REF_TYPE.QUALIFIED_CLASS]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('Integration.xml'),
            [REF_TYPE.INTEGRATION_FILE_PATH]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('SomeClass.java'),
            [REF_TYPE.MODULE_FILE_PATH]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('Android.mk'),
            [REF_TYPE.MODULE_FILE_PATH]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('Android.bp'),
            [REF_TYPE.MODULE_FILE_PATH]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('module:Class'),
            [REF_TYPE.INTEGRATION, REF_TYPE.MODULE_CLASS]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('module:f.q.Class'),
            [REF_TYPE.INTEGRATION, REF_TYPE.MODULE_CLASS]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('module:a.package'),
            [REF_TYPE.MODULE_PACKAGE]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('.'),
            [REF_TYPE.INTEGRATION_FILE_PATH, REF_TYPE.MODULE_FILE_PATH]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('..'),
            [REF_TYPE.INTEGRATION_FILE_PATH, REF_TYPE.MODULE_FILE_PATH]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('./rel/path/to/test'),
            [REF_TYPE.INTEGRATION_FILE_PATH, REF_TYPE.MODULE_FILE_PATH]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('rel/path/to/test'),
            [REF_TYPE.INTEGRATION_FILE_PATH, REF_TYPE.MODULE_FILE_PATH,
             REF_TYPE.INTEGRATION]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('/abs/path/to/test'),
            [REF_TYPE.INTEGRATION_FILE_PATH, REF_TYPE.MODULE_FILE_PATH]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('int/test'),
            [REF_TYPE.INTEGRATION_FILE_PATH, REF_TYPE.MODULE_FILE_PATH,
             REF_TYPE.INTEGRATION]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('int/test:fully.qual.Class#m'),
            [REF_TYPE.INTEGRATION_FILE_PATH, REF_TYPE.MODULE_FILE_PATH,
             REF_TYPE.INTEGRATION]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('int/test:Class#method'),
            [REF_TYPE.INTEGRATION_FILE_PATH, REF_TYPE.MODULE_FILE_PATH,
             REF_TYPE.INTEGRATION]
        )
        self.assertEqual(
            test_finder_handler._get_test_reference_types('int_name_no_slash:Class#m'),
            [REF_TYPE.INTEGRATION, REF_TYPE.MODULE_CLASS]
        )

    def test_get_registered_find_methods(self):
        """Test that we get the registered find methods."""
        empty_mod_info = None
        example_finder_a_instance = test_finder_handler._get_finder_instance_dict(
            empty_mod_info)[_EXAMPLE_FINDER_A]
        should_equal = [
            test_finder_base.Finder(
                example_finder_a_instance,
                ExampleFinderA.registered_find_method_from_example_finder)]
        should_not_equal = [
            test_finder_base.Finder(
                example_finder_a_instance,
                ExampleFinderA.unregistered_find_method_from_example_finder)]
        # Let's make sure we see the registered method.
        self.assertEqual(
            should_equal,
            test_finder_handler._get_registered_find_methods(empty_mod_info)
        )
        # Make sure we don't see the unregistered method here.
        self.assertNotEqual(
            should_not_equal,
            test_finder_handler._get_registered_find_methods(empty_mod_info)
        )

    def test_get_find_methods_for_test(self):
        """Test that we get the find methods we expect."""
        # Let's see that we get the unregistered and registered find methods in
        # the order we expect.
        test = ''
        registered_find_methods = [
            test_finder_base.Finder(
                _FINDER_INSTANCES[_EXAMPLE_FINDER_A],
                ExampleFinderA.registered_find_method_from_example_finder)]
        default_find_methods = [
            test_finder_base.Finder(
                _FINDER_INSTANCES[_EXAMPLE_FINDER_A],
                ExampleFinderA.unregistered_find_method_from_example_finder)]
        should_equal = registered_find_methods + default_find_methods
        self.assertEqual(
            should_equal,
            test_finder_handler.get_find_methods_for_test(self.empty_mod_info,
                                                          test))


if __name__ == '__main__':
    unittest.main()
