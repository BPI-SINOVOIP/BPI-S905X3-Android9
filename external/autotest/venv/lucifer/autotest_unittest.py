# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for autotest.py."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import sys

import mock
import pytest
import subprocess32

from lucifer import autotest


@pytest.mark.slow
def test_monkeypatch():
    """Test monkeypatch()."""
    common_file = subprocess32.check_output(
            [sys.executable, '-m',
             'lucifer.cmd.test.autotest_monkeypatcher'])
    assert common_file.rstrip() == '<removed>'


@pytest.mark.parametrize('fullname,expected', [
        ('autotest_lib.common', True),
        ('autotest_lib.server.common', True),
        ('autotest_lib.server', False),
        ('some_lib.common', False),
])
def test__CommonRemovingFinder_find_module(fullname, expected):
    """Test _CommonRemovingFinder.find_module()."""
    finder = autotest._CommonRemovingFinder()
    got = finder.find_module(fullname)
    assert got == (finder if expected else None)


@pytest.mark.parametrize('name,expected', [
        ('scheduler.models', 'autotest_lib.scheduler.models'),
])
def test_load(name, expected):
    """Test load()."""
    with mock.patch('importlib.import_module', autospec=True) \
         as import_module, \
         mock.patch.object(autotest, '_setup_done', True):
        autotest.load(name)
        import_module.assert_called_once_with(expected)


def test_load_without_patch_fails():
    """Test load() without patch."""
    with mock.patch.object(autotest, '_setup_done', False):
        with pytest.raises(ImportError):
            autotest.load('asdf')


@pytest.mark.parametrize('name,expected', [
        ('constants', 'chromite.lib.constants'),
])
def test_chromite_load(name, expected):
    """Test load()."""
    with mock.patch('importlib.import_module', autospec=True) \
         as import_module, \
         mock.patch.object(autotest, '_setup_done', True):
        autotest.chromite_load(name)
        import_module.assert_called_once_with(expected)


@pytest.mark.parametrize('name', [
        'google.protobuf.internal.well_known_types',
])
def test_deps_load(name):
    """Test load()."""
    with mock.patch('importlib.import_module', autospec=True) \
         as import_module, \
         mock.patch.object(autotest, '_setup_done', True):
        autotest.deps_load(name)
        import_module.assert_called_once_with(name)
