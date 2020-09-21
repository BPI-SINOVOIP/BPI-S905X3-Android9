# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base


class policy_ManagedBookmarks(enterprise_policy_base.EnterprisePolicyTest):
    """Test effect of ManagedBookmarks policy on Chrome OS behavior.

    This test verifies the behavior of Chrome OS for a range of valid values
    of the ManagedBookmarks user policy, as defined by three test cases:
    NotSet_NotShown, SingleBookmark_Shown, and MultiBookmarks_Shown.

    When not set, the policy value is undefined. This induces the default
    behavior of not showing the managed bookmarks folder, which is equivalent
    to what is seen by an un-managed user.

    When one or more bookmarks are specified by the policy, then the Managed
    Bookmarks folder is shown, and the specified bookmarks within it.

    """
    version = 1

    POLICY_NAME = 'ManagedBookmarks'
    SINGLE_BOOKMARK = [{'name': 'Google',
                        'url': 'https://www.google.com/'}]

    MULTI_BOOKMARK = [{'name': 'Google',
                       'url': 'https://www.google.com/'},
                      {'name': 'CNN',
                       'url': 'http://www.cnn.com/'},
                      {'name': 'IRS',
                       'url': 'http://www.irs.gov/'}]

    SUPPORTING_POLICIES = {
        'BookmarkBarEnabled': True
    }

    # Dictionary of test case names and policy values.
    TEST_CASES = {
        'NotSet_NotShown': None,
        'SingleBookmark_Shown': SINGLE_BOOKMARK,
        'MultipleBookmarks_Shown': MULTI_BOOKMARK
    }

    def _test_managed_bookmarks(self, policy_value):
        """Verify CrOS enforces ManagedBookmarks policy.

        When ManagedBookmarks is not set, the UI shall not show the managed
        bookmarks folder nor its contents. When set to one or more bookmarks
        the UI shows the folder and its contents.

        @param policy_value: policy value for this case.

        """
        managed_bookmarks_are_shown = self._are_bookmarks_shown(policy_value)
        if policy_value is None:
            if managed_bookmarks_are_shown:
                raise error.TestFail('Managed Bookmarks should be hidden.')
        else:
            if not managed_bookmarks_are_shown:
                raise error.TestFail('Managed Bookmarks should be shown.')

    def _are_bookmarks_shown(self, policy_bookmarks):
        """Check whether managed bookmarks are shown in the UI.

        @param policy_bookmarks: bookmarks expected.
        @returns: True if the managed bookmarks are shown.

        """
        # Extract dictionary of folders shown in bookmark tree.
        tab = self._open_boomark_manager_to_folder(0)
        cmd = 'document.getElementsByClassName("tree-item");'
        tree_items = self.get_elements_from_page(tab, cmd)

        # Scan bookmark tree for a folder with the domain-name in title.
        domain_name = self.username.split('@')[1]
        folder_title = domain_name + ' bookmarks'
        for bookmark_element in tree_items.itervalues():
            bookmark_node = bookmark_element['bookmarkNode']
            bookmark_title = bookmark_node['title']
            if bookmark_title == folder_title:
                folder_id = bookmark_node['id'].encode('ascii', 'ignore')
                break
        else:
            tab.Close()
            return False
        tab.Close()

        # Extract list of bookmarks shown in bookmark list-pane.
        tab = self._open_boomark_manager_to_folder(folder_id)
        cmd = '''
            var bookmarks = [];
            var listPane = document.getElementById("list-pane");
            var labels = listPane.getElementsByClassName("label");
            for (var i = 0; i < labels.length; i++) {
               bookmarks.push(labels[i].textContent);
            }
            bookmarks;
        '''
        bookmark_items = self.get_elements_from_page(tab, cmd)
        tab.Close()

        # Get list of expected bookmarks as set by policy.
        bookmarks_expected = None
        if policy_bookmarks:
            bookmarks_expected = [bmk['name'] for bmk in policy_bookmarks]

        # Compare bookmarks shown vs expected.
        if bookmark_items != bookmarks_expected:
            raise error.TestFail('Bookmarks shown are not correct: %s '
                                 '(expected: %s)' %
                                 (bookmark_items, bookmarks_expected))
        return True

    def _open_boomark_manager_to_folder(self, folder_number):
        """Open bookmark manager page and select specified folder.

        @param folder_number: folder to select when opening page.
        @returns: tab loaded with bookmark manager page.

        """
        # Open Bookmark Manager with specified folder selected.
        bmp_url = ('chrome://bookmarks/#%s' % folder_number)
        tab = self.navigate_to_url(bmp_url)

        # Wait until list.reload() is defined on page.
        tab.WaitForJavaScriptCondition(
            "typeof bmm.list.reload == 'function'", timeout=60)
        time.sleep(1)  # Allow JS to run after function is defined.
        return tab

    def run_once(self, case):
        """Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.

        """
        case_value = self.TEST_CASES[case]
        self.SUPPORTING_POLICIES[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=self.SUPPORTING_POLICIES)
        self._test_managed_bookmarks(case_value)
