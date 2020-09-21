#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import mox

import common
from autotest_lib.server.cros.dynamic_suite import reporting_utils


class TestMergeBugTemplate(mox.MoxTestBase):
    """Test bug can be properly merged and validated."""
    def test_validate_success(self):
        """Test a valid bug can be verified successfully."""
        bug_template= {}
        bug_template['owner'] = 'someone@company.com'
        reporting_utils.BugTemplate.validate_bug_template(bug_template)


    def test_validate_success(self):
        """Test a valid bug can be verified successfully."""
        # Bug template must be a dictionary.
        bug_template = ['test']
        self.assertRaises(reporting_utils.InvalidBugTemplateException,
                          reporting_utils.BugTemplate.validate_bug_template,
                          bug_template)

        # Bug template must contain value for essential attribute, e.g., owner.
        bug_template= {'no-owner': 'user1'}
        self.assertRaises(reporting_utils.InvalidBugTemplateException,
                          reporting_utils.BugTemplate.validate_bug_template,
                          bug_template)

        # Bug template must contain value for essential attribute, e.g., owner.
        bug_template= {'owner': 'invalid_email_address'}
        self.assertRaises(reporting_utils.InvalidBugTemplateException,
                          reporting_utils.BugTemplate.validate_bug_template,
                          bug_template)

        # Check unexpected attributes.
        bug_template= {}
        bug_template['random tag'] = 'test'
        self.assertRaises(reporting_utils.InvalidBugTemplateException,
                          reporting_utils.BugTemplate.validate_bug_template,
                          bug_template)

        # Value for cc must be a list
        bug_template= {}
        bug_template['cc'] = 'test'
        self.assertRaises(reporting_utils.InvalidBugTemplateException,
                          reporting_utils.BugTemplate.validate_bug_template,
                          bug_template)

        # Value for labels must be a list
        bug_template= {}
        bug_template['labels'] = 'test'
        self.assertRaises(reporting_utils.InvalidBugTemplateException,
                          reporting_utils.BugTemplate.validate_bug_template,
                          bug_template)


    def test_merge_success(self):
        """Test test and suite bug templates can be merged successfully."""
        test_bug_template = {
            'labels': ['l1'],
            'owner': 'user1@chromium.org',
            'status': 'Assigned',
            'title': None,
            'cc': ['cc1@chromium.org', 'cc2@chromium.org']
        }
        suite_bug_template = {
            'labels': ['l2'],
            'owner': 'user2@chromium.org',
            'status': 'Fixed',
            'summary': 'This is a short summary for suite bug',
            'title': 'Title for suite bug',
            'cc': ['cc2@chromium.org', 'cc3@chromium.org']
        }
        bug_template = reporting_utils.BugTemplate(suite_bug_template)
        merged_bug_template = bug_template.finalize_bug_template(
                test_bug_template)
        self.assertEqual(merged_bug_template['owner'],
                         test_bug_template['owner'],
                         'Value in test bug template should prevail.')

        self.assertEqual(merged_bug_template['title'],
                         suite_bug_template['title'],
                         'If an attribute has value None in test bug template, '
                         'use the value given in suite bug template.')

        self.assertEqual(merged_bug_template['summary'],
                         suite_bug_template['summary'],
                         'If an attribute does not exist in test bug template, '
                         'but exists in suite bug template, it should be '
                         'included in the merged template.')

        self.assertEqual(merged_bug_template['cc'],
                         test_bug_template['cc'] + suite_bug_template['cc'],
                         'List values for an attribute should be merged.')

        self.assertEqual(merged_bug_template['labels'],
                         test_bug_template['labels'] +
                         suite_bug_template['labels'],
                         'List values for an attribute should be merged.')

        test_bug_template['owner'] = ''
        test_bug_template['cc'] = ['']
        suite_bug_template['owner'] = ''
        suite_bug_template['cc'] = ['']
        bug_template = reporting_utils.BugTemplate(suite_bug_template)
        merged_bug_template = bug_template.finalize_bug_template(
                test_bug_template)
        self.assertFalse('owner' in merged_bug_template,
                         'owner should be removed from the merged template.')
        self.assertFalse('cc' in merged_bug_template,
                         'cc should be removed from the merged template.')


if __name__ == '__main__':
    unittest.main()
