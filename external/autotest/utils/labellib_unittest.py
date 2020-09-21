#!/usr/bin/python2.7
#
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import copy
import unittest

import common
from autotest_lib.utils import labellib


class KeyvalLabelTestCase(unittest.TestCase):
    """Tests for basic KeyvalLabel functions."""

    def test_parse_keyval_label(self):
        got = labellib.parse_keyval_label('pool:suites')
        self.assertEqual(got, labellib.KeyvalLabel('pool', 'suites'))

    def test_parse_keyval_label_with_multiple_colons(self):
        got = labellib.parse_keyval_label('pool:suites:penthouse')
        self.assertEqual(got, labellib.KeyvalLabel('pool', 'suites:penthouse'))

    def test_parse_keyval_label_raises(self):
        with self.assertRaises(ValueError):
            labellib.parse_keyval_label('webcam')

    def test_format_keyval_label(self):
        got = labellib.format_keyval_label(
                labellib.KeyvalLabel('pool', 'suites'))
        self.assertEqual(got, 'pool:suites')

    def test_format_keyval_label_with_colon_in_value(self):
        got = labellib.format_keyval_label(
                labellib.KeyvalLabel('pool', 'suites:penthouse'))
        self.assertEqual(got, 'pool:suites:penthouse')


class LabelsMappingTestCase(unittest.TestCase):
    """Tests for LabelsMapping class."""

    def test_getlabels(self):
        labels = ['webcam', 'pool:suites']
        mapping = labellib.LabelsMapping(labels)
        self.assertEqual(mapping.getlabels(), labels)

    def test_init_and_getlabels_should_remove_duplicates(self):
        labels = ['webcam', 'pool:suites', 'pool:party']
        mapping = labellib.LabelsMapping(labels)
        self.assertEqual(mapping.getlabels(), ['webcam', 'pool:suites'])

    def test_init_and_getlabels_should_move_plain_labels_first(self):
        labels = ['ohse:tsubame', 'webcam']
        mapping = labellib.LabelsMapping(labels)
        self.assertEqual(mapping.getlabels(), ['webcam', 'ohse:tsubame'])

    def test_init_and_getlabels_should_preserve_plain_label_order(self):
        labels = ['webcam', 'exec', 'method']
        mapping = labellib.LabelsMapping(labels)
        self.assertEqual(mapping.getlabels(), ['webcam', 'exec', 'method'])

    def test_init_and_getlabels_should_preserve_keyval_label_order(self):
        labels = ['class:protecta', 'method:metafalica', 'exec:chronicle_key']
        mapping = labellib.LabelsMapping(labels)
        self.assertEqual(mapping.getlabels(), labels)

    def test_init_should_not_mutate_labels(self):
        labels = ['class:protecta', 'exec:chronicle_key', 'method:metafalica']
        input_labels = copy.deepcopy(labels)
        mapping = labellib.LabelsMapping(input_labels)
        mapping['class'] = 'distllista'
        self.assertEqual(input_labels, labels)

    def test_init_mutated_arg_should_not_affect_mapping(self):
        labels = ['class:protecta', 'exec:chronicle_key', 'method:metafalica']
        mapping = labellib.LabelsMapping(labels)
        original_mapping = copy.deepcopy(mapping)
        labels.pop()
        self.assertEqual(mapping, original_mapping)

    def test_duplicate_keys_should_take_first(self):
        labels = ['webcam', 'pool:party', 'pool:suites']
        mapping = labellib.LabelsMapping(labels)
        self.assertEqual(mapping['pool'], 'party')

    def test_getitem(self):
        labels = ['webcam', 'pool:suites']
        mapping = labellib.LabelsMapping(labels)
        self.assertEqual(mapping['pool'], 'suites')

    def test_in(self):
        labels = ['webcam', 'pool:suites']
        mapping = labellib.LabelsMapping(labels)
        self.assertIn('pool', mapping)

    def test_setitem(self):
        labels = ['webcam']
        mapping = labellib.LabelsMapping(labels)
        mapping['pool'] = 'suites'
        self.assertEqual(mapping['pool'], 'suites')

    def test_setitem_to_none_should_delete(self):
        labels = ['webcam', 'pool:suites']
        mapping = labellib.LabelsMapping(labels)
        mapping['pool'] = None
        self.assertNotIn('pool', mapping)

    def test_setitem_to_none_with_missing_key_should_noop(self):
        labels = ['webcam', 'pool:suites']
        mapping = labellib.LabelsMapping(labels)
        mapping['foo'] = None
        self.assertNotIn('foo', mapping)

    def test_delitem(self):
        labels = ['webcam', 'pool:suites']
        mapping = labellib.LabelsMapping(labels)
        del mapping['pool']
        self.assertNotIn('pool', mapping)

    def test_iter(self):
        labels = ['webcam', 'pool:suites']
        mapping = labellib.LabelsMapping(labels)
        self.assertEqual(list(iter(mapping)), ['pool'])

    def test_len(self):
        labels = ['webcam', 'pool:suites']
        mapping = labellib.LabelsMapping(labels)
        self.assertEqual(len(mapping), 1)


class CrosVersionTestCase(unittest.TestCase):
    """Tests for CrosVersion functions."""

    def test_parse_cros_version_without_rc(self):
        got = labellib.parse_cros_version('lumpy-release/R27-3773.0.0')
        self.assertEqual(got, labellib.CrosVersion('lumpy-release', 'lumpy',
                                                   'R27', '3773.0.0', None))

    def test_parse_cros_version_with_rc(self):
        got = labellib.parse_cros_version('lumpy-release/R27-3773.0.0-rc1')
        self.assertEqual(got, labellib.CrosVersion('lumpy-release', 'lumpy',
                                                   'R27', '3773.0.0', 'rc1'))

    def test_parse_cros_version_raises(self):
        with self.assertRaises(ValueError):
            labellib.parse_cros_version('foo')

    def test_format_cros_version_without_rc(self):
        got = labellib.format_cros_version(
                labellib.CrosVersion('lumpy-release', 'lumpy', 'R27',
                                     '3773.0.0', None))
        self.assertEqual(got, 'lumpy-release/R27-3773.0.0')

    def test_format_cros_version_with_rc(self):
        got = labellib.format_cros_version(
                labellib.CrosVersion('lumpy-release', 'lumpy',  'R27',
                                     '3773.0.0', 'rc1'))
        self.assertEqual(got, 'lumpy-release/R27-3773.0.0-rc1')

    def test_parse_cros_version_for_board(self):
        test_builds = ['lumpy-release/R27-3773.0.0-rc1',
                       'trybot-lumpy-paladin/R27-3773.0.0',
                       'lumpy-pre-cq/R27-3773.0.0-rc1',
                       'lumpy-test-ap/R27-3773.0.0-rc1',
                       'lumpy-toolchain/R27-3773.0.0-rc1',
                       ]
        for build in test_builds:
            cros_version = labellib.parse_cros_version(build)
            self.assertEqual(cros_version.board, 'lumpy')
            self.assertEqual(cros_version.milestone, 'R27')

        build = 'trybot-lumpy-a-pre-cq/R27-3773.0.0-rc1'
        cros_version = labellib.parse_cros_version(build)
        self.assertEqual(cros_version.board, 'lumpy-a')
        self.assertEqual(cros_version.milestone, 'R27')

        build = 'trybot-lumpy_a-pre-cq/R27-3773.0.0-rc1'
        cros_version = labellib.parse_cros_version(build)
        self.assertEqual(cros_version.board, 'lumpy_a')
        self.assertEqual(cros_version.milestone, 'R27')


if __name__ == '__main__':
    unittest.main()
