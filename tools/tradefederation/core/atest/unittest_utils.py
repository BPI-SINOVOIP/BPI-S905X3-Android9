#!/usr/bin/env python
#
# Copyright 2017, The Android Open Source Project
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

"""Utility functions for unit tests."""

import os

import constants
import unittest_constants as uc

def assert_strict_equal(test_class, first, second):
    """Check for strict equality and strict equality of nametuple elements.

    assertEqual considers types equal to their subtypes, but we want to
    not consider set() and frozenset() equal for testing.
    """
    test_class.assertEqual(first, second)
    # allow byte and unicode string equality.
    if not (isinstance(first, basestring) and
            isinstance(second, basestring)):
        test_class.assertIsInstance(first, type(second))
        test_class.assertIsInstance(second, type(first))
    # Recursively check elements of namedtuples for strict equals.
    if isinstance(first, tuple) and hasattr(first, '_fields'):
        # pylint: disable=invalid-name
        for f in first._fields:
            assert_strict_equal(test_class, getattr(first, f),
                                getattr(second, f))

def assert_equal_testinfos(test_class, test_info_a, test_info_b):
    """Check that the passed in TestInfos are equal."""
    # Use unittest.assertEqual to do checks when None is involved.
    if test_info_a is None or test_info_b is None:
        test_class.assertEqual(test_info_a, test_info_b)
        return

    for attr in test_info_a.__dict__:
        test_info_a_attr = getattr(test_info_a, attr)
        test_info_b_attr = getattr(test_info_b, attr)
        test_class.assertEqual(test_info_a_attr, test_info_b_attr,
                               msg=('TestInfo.%s mismatch: %s != %s' %
                                    (attr, test_info_a_attr, test_info_b_attr)))

def assert_equal_testinfo_sets(test_class, test_info_set_a, test_info_set_b):
    """Check that the sets of TestInfos are equal."""
    test_class.assertEqual(len(test_info_set_a), len(test_info_set_b),
                           msg=('mismatch # of TestInfos: %d != %d' %
                                (len(test_info_set_a), len(test_info_set_b))))
    # Iterate over a set and pop them out as you compare them.
    while test_info_set_a:
        test_info_a = test_info_set_a.pop()
        test_info_b_to_remove = None
        for test_info_b in test_info_set_b:
            try:
                assert_equal_testinfos(test_class, test_info_a, test_info_b)
                test_info_b_to_remove = test_info_b
                break
            except AssertionError:
                pass
        if test_info_b_to_remove:
            test_info_set_b.remove(test_info_b_to_remove)
        else:
            # We haven't found a match, raise an assertion error.
            raise AssertionError('No matching TestInfo (%s) in [%s]' %
                                 (test_info_a, ';'.join([str(t) for t in test_info_set_b])))


def isfile_side_effect(value):
    """Mock return values for os.path.isfile."""
    if value == '/%s/%s' % (uc.MODULE_DIR, constants.MODULE_CONFIG):
        return True
    if value.endswith('.java'):
        return True
    if value.endswith(uc.INT_NAME + '.xml'):
        return True
    if value.endswith(uc.GTF_INT_NAME + '.xml'):
        return True
    return False


def realpath_side_effect(path):
    """Mock return values for os.path.realpath."""
    return os.path.join(uc.ROOT, path)
