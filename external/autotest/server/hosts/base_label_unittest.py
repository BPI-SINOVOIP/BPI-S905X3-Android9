#!/usr/bin/python
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mock
import unittest

import common

from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server import utils
from autotest_lib.server.hosts import base_label

 # pylint: disable=missing-docstring


class TestBaseLabel(base_label.BaseLabel):
    """TestBaseLabel is used for testing/validating BaseLabel methods."""

    _NAME = 'base_label'

    def exists(self, host):
        return host.exists


class TestBaseLabels(base_label.BaseLabel):
    """
    TestBaseLabels is used for testing/validating BaseLabel methods.

    This is a variation of BaseLabel with multiple labels for _NAME
    to ensure we handle a label that contains a list of labels for
    its _NAME attribute.
    """

    _NAME = ['base_label_1' , 'base_label_2']


class TestStringPrefixLabel(base_label.StringPrefixLabel):
    """
    TestBaseLabels is used for testing/validating StringPrefixLabel methods.

    This test class is to check that we properly construct the prefix labels
    with the label passed in during construction.
    """

    _NAME = 'prefix'

    def __init__(self, label='postfix'):
        self.label_to_return = label


    def generate_labels(self, _):
        return [self.label_to_return]


class TestLabelList(base_label.StringLabel):
    """TestLabelList is used to validate a label consisting of other labels."""

    _LABEL_LIST = [TestBaseLabel(), TestStringPrefixLabel()]

    def generate_labels(self, host):
        labels = []
        for label_detectors in self._LABEL_LIST:
            labels.extend(label_detectors.get(host))
        return labels


class MockAFEHost(utils.EmptyAFEHost):

    def __init__(self, labels=[], attributes={}):
        self.labels = labels
        self.attributes = attributes


class MockHost(object):

    def __init__(self, exists=True, afe_host=None):
        self.hostname = 'hostname'
        self.exists = exists
        self._afe_host = afe_host


class BaseLabelUnittests(unittest.TestCase):
    """Unittest for testing base_label.BaseLabel."""

    def setUp(self):
        self.test_base_label = TestBaseLabel()
        self.test_base_labels = TestBaseLabels()


    def test_generate_labels(self):
        """Let's make sure generate_labels() returns the labels expected."""
        self.assertEqual(self.test_base_label.generate_labels(None),
                         [self.test_base_label._NAME])


    def test_get(self):
        """Let's make sure the logic in get() works as expected."""
        # We should get labels here.
        self.assertEqual(self.test_base_label.get(MockHost(exists=True)),
                         [self.test_base_label._NAME])
        # We should get nothing here.
        self.assertEqual(self.test_base_label.get(MockHost(exists=False)),
                         [])


    def test_get_all_labels(self):
        """Check that we get the expected labels for get_all_labels()."""
        prefix_tbl, full_tbl = self.test_base_label.get_all_labels()
        prefix_tbls, full_tbls = self.test_base_labels.get_all_labels()

        # We want to check that we always get a list of labels regardless if
        # the label class attribute _NAME is a list or a string.
        self.assertEqual(full_tbl, set([self.test_base_label._NAME]))
        self.assertEqual(full_tbls, set(self.test_base_labels._NAME))

        # We want to make sure we get nothing on the prefix_* side of things
        # since BaseLabel shouldn't be a prefix for any label.
        self.assertEqual(prefix_tbl, set())
        self.assertEqual(prefix_tbls, set())


class StringPrefixLabelUnittests(unittest.TestCase):
    """Unittest for testing base_label.StringPrefixLabel."""

    def setUp(self):
        self.postfix_label = 'postfix_label'
        self.test_label = TestStringPrefixLabel(label=self.postfix_label)


    def test_get(self):
        """Let's make sure that the labels we get are prefixed."""
        self.assertEqual(self.test_label.get(None),
                         ['%s:%s' % (self.test_label._NAME,
                                     self.postfix_label)])


    def test_get_all_labels(self):
        """Check that we only get prefix labels and no full labels."""
        prefix_labels, postfix_labels = self.test_label.get_all_labels()
        self.assertEqual(prefix_labels, set(['%s:' % self.test_label._NAME]))
        self.assertEqual(postfix_labels, set())


class LabelRetrieverUnittests(unittest.TestCase):
    """Unittest for testing base_label.LabelRetriever."""

    def setUp(self):
        label_list = [TestStringPrefixLabel(), TestBaseLabel()]
        self.retriever = base_label.LabelRetriever(label_list)
        self.retriever._populate_known_labels(label_list)
        self.retriever_label_list = base_label.LabelRetriever([TestLabelList()])
        self.retriever_label_list._populate_known_labels([TestLabelList()])


    def test_populate_known_labels(self):
        """Check that _populate_known_labels() works as expected."""
        full_names = set([TestBaseLabel._NAME])
        prefix_names = set(['%s:' % TestStringPrefixLabel._NAME])
        # Check on a normal retriever.
        self.assertEqual(self.retriever.label_full_names, full_names)
        self.assertEqual(self.retriever.label_prefix_names, prefix_names)

        # Check on a retriever that has a label with a label list.
        self.assertEqual(self.retriever_label_list.label_full_names, full_names)
        self.assertEqual(self.retriever_label_list.label_prefix_names,
                         prefix_names)


    def test_is_known_label(self):
        """Check _is_known_label() detects/skips the right labels."""
        # This will be a list of tuples of label and expected return bool.
        # Make sure Full matches match correctly
        labels_to_check = [(TestBaseLabel._NAME, True),
                           ('%s:' % TestStringPrefixLabel._NAME, True),
                           # Make sure partial matches fail.
                           (TestBaseLabel._NAME[:2], False),
                           ('%s:' % TestStringPrefixLabel._NAME[:2], False),
                           ('no_label_match', False)]

        for label, expected_known in labels_to_check:
            self.assertEqual(self.retriever._is_known_label(label),
                             expected_known)


    @mock.patch.object(frontend_wrappers, 'RetryingAFE')
    def test_update_labels(self, mock_retry_afe):
        """Check that we add/remove the expected labels in update_labels()."""
        label_to_add = 'label_to_add'
        label_to_remove = 'prefix:label_to_remove'
        mock_afe = mock.MagicMock()
        mockhost = MockHost(afe_host=MockAFEHost(
                labels=[label_to_remove, TestBaseLabel._NAME]))
        expected_remove_labels = [label_to_remove]
        expected_add_labels = ['%s:%s' % (TestStringPrefixLabel._NAME,
                                          label_to_add)]

        retriever = base_label.LabelRetriever(
                [TestStringPrefixLabel(label=label_to_add),
                 TestBaseLabel()])

        retriever.update_labels(mockhost)

        # Check that we removed the right labels
        mock_afe.run.has_calls('host_remove_labels',
                               id=mockhost.hostname,
                               labels=expected_remove_labels)

        # Check that we added the right labels
        mock_afe.run.has_calls('host_add_labels',
                               id=mockhost.hostname,
                               labels=expected_add_labels)


if __name__ == '__main__':
    unittest.main()

