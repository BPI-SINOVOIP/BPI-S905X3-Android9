# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base


class policy_EditBookmarksEnabled(enterprise_policy_base.EnterprisePolicyTest):
    """Test effect of EditBookmarksEnabled policy on Chrome OS behavior.

    This test verifies the behavior of Chrome OS for all valid values of the
    EditBookmarksEnabled user policy: True, False, and not set. 'Not set'
    means that the policy value is undefined. This should induce the default
    behavior, equivalent to what is seen by an un-managed user.

    When set True or not set, bookmarks can be added, removed, or modified.
    When set False, bookmarks cannot be added, removed, or modified, though
    existing bookmarks (if any) are still available.
    """
    version = 1

    POLICY_NAME = 'EditBookmarksEnabled'
    BOOKMARKS = [{"name": "Google",
                  "url": "https://www.google.com/"},
                 {"name": "CNN",
                  "url": "http://www.cnn.com/"},
                 {"name": "IRS",
                  "url": "http://www.irs.gov/"}]
    SUPPORTING_POLICIES = {
        'BookmarkBarEnabled': True,
        'ManagedBookmarks': BOOKMARKS
    }

    # Dictionary of named test cases and policy data.
    TEST_CASES = {
        'True_Enable': True,
        'False_Disable': False,
        'NotSet_Enable': None
    }

    def _is_add_bookmark_disabled(self):
        """Check whether add-new-bookmark-command menu item is disabled.

        @returns: True if add-new-bookmarks-command is disabled.
        """
        tab = self.navigate_to_url('chrome://bookmarks/#1')

        # Wait until list.reload() is defined on bmm page.
        tab.WaitForJavaScriptCondition(
            "typeof bmm.list.reload == 'function'", timeout=60)
        time.sleep(1)  # Allow JS to run after reload function is defined.

        # Check if add-new-bookmark menu command has disabled property.
        is_disabled = tab.EvaluateJavaScript(
            '$("add-new-bookmark-command").disabled;')
        logging.info('add-new-bookmark-command is disabled: %s', is_disabled)
        tab.Close()
        return is_disabled

    def _test_edit_bookmarks_enabled(self, policy_value):
        """Verify CrOS enforces EditBookmarksEnabled policy.

        When EditBookmarksEnabled is true or not set, the UI allows the user
        to add bookmarks. When false, the UI does not allow the user to add
        bookmarks.

        Warning: When the 'Bookmark Editing' setting on the CPanel User
        Settings page is set to 'Enable bookmark editing', then the
        EditBookmarksEnabled policy on the client will be not set. Thus, to
        verify the 'Enable bookmark editing' choice from a production or
        staging DMS, use case=NotSet_Enable.

        @param policy_value: policy value for this case.
        """
        add_bookmark_is_disabled = self._is_add_bookmark_disabled()
        if policy_value == True or policy_value == None:
            if add_bookmark_is_disabled:
                raise error.TestFail('Add Bookmark should be enabled.')
        else:
            if not add_bookmark_is_disabled:
                raise error.TestFail('Add Bookmark should be disabled.')

    def run_once(self, case):
        """Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.
        """
        case_value = self.TEST_CASES[case]
        self.SUPPORTING_POLICIES[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=self.SUPPORTING_POLICIES)
        self._test_edit_bookmarks_enabled(case_value)
