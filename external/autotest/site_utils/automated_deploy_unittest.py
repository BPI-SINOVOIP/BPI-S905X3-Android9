#!/usr/bin/python
# Copyright (c) 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for automated_deploy.py."""

from __future__ import print_function

import mock
import unittest

import common
from autotest_lib.site_utils import automated_deploy as ad
from autotest_lib.site_utils.lib import infra


class AutomatedDeployTest(unittest.TestCase):
    """Test automated_deploy with commands mocked out."""

    GIT_LOG_FOR_COMMITS = '''123 foo
456 bar'''

    PUSH_LOG = '''Total 0 (delta 0), reused 0 (delta 0)
remote: Processing changes: done
To https:TEST_URL
123..456  prod -> prod'''

    def setUp(self):
        infra.chdir = mock.MagicMock()


    @mock.patch.object(infra, 'local_runner')
    def testUpdateProdBranchWithNoNewChanges(self, run_cmd):
        """Test update_prod_branch when there exist no new changes.

        @param run_cmd: Mock of infra.local_runner call used.
        """
        run_cmd.return_value = None
        self.assertEqual(ad.update_prod_branch('test', 'test_dir', '123'), None)
        expect_cmds = [
            mock.call('git log prod..123 --oneline', stream_output=True)]
        run_cmd.assert_has_calls(expect_cmds)


    @mock.patch.object(infra, 'local_runner')
    def testUpdateProdBranchRebaseToCorrectHash(self, run_cmd):
        """Test whether update_prod_branch can rebase to the correct hash.

        @param run_cmd: Mock of infra.local_runner call used.
        """
        run_cmd.side_effect = [self.GIT_LOG_FOR_COMMITS, None, self.PUSH_LOG]
        ad.update_prod_branch('test', 'test_dir', '123')
        expect_cmds = [
            mock.call('git log prod..123 --oneline', stream_output=True),
            mock.call('git rebase 123 prod', stream_output=True),
            mock.call('git push origin prod', stream_output=True)]
        run_cmd.assert_has_calls(expect_cmds)


    @mock.patch.object(infra, 'local_runner')
    def testUpdateProdBranchRebaseToProdNext(self, run_cmd):
        """Test whether rebase to prod-next branch when the hash is not given.

        @param run_cmd: Mock of infra.local_runner call used.
        """
        run_cmd.side_effect = [self.GIT_LOG_FOR_COMMITS, None, self.PUSH_LOG]
        ad.update_prod_branch('test', 'test_dir', None)
        expect_cmds = [
            mock.call('git log prod..origin/prod-next --oneline',
                      stream_output=True),
            mock.call('git rebase origin/prod-next prod',
                      stream_output=True),
            mock.call('git push origin prod', stream_output=True)]
        run_cmd.assert_has_calls(expect_cmds)


    @mock.patch.object(infra, 'local_runner')
    def testUpdateProdBranchParseCommitRange(self, run_cmd):
        """Test to grep the pushed commit range from the normal push log.

        @param run_cmd: Mock of infra.local_runner call used.
        """
        run_cmd.side_effect = [self.GIT_LOG_FOR_COMMITS, None, self.PUSH_LOG]
        self.assertEqual(ad.update_prod_branch('test', 'test_dir', None),
                         '123..456')


    @mock.patch.object(infra, 'local_runner')
    def testUpdateProdBranchFailToParseCommitRange(self, run_cmd):
        """Test to grep the pushed commit range from the failed push log.

        @param run_cmd: Mock of infra.local_runner call used.
        """
        run_cmd.side_effect = [self.GIT_LOG_FOR_COMMITS, None,
                               'Fail to push prod branch']
        with self.assertRaises(ad.AutoDeployException):
             ad.update_prod_branch('test', 'test_dir', None)


    @mock.patch.object(infra, 'local_runner')
    def testGetPushedCommits(self, run_cmd):
        """Test automated_deploy.get_pushed_commits.

        @param run_cmd: Mock of infra.local_runner call used.
        """
        autotest_commits_logs = '123: autotest: cl_1\n456: autotest: cl_2\n'
        chromite_commits_logs = '789: test_cl_1\n'
        fake_commits_logs = autotest_commits_logs + chromite_commits_logs
        run_cmd.return_value = fake_commits_logs

        #Test to get pushed commits for autotest repo.
        repo = 'autotest'
        expect_git_log_cmd = 'git log --oneline 123..789'
        expect_display_cmd = expect_git_log_cmd + ' | grep autotest'
        expect_return = ('\n%s:\n%s\n%s\n' %
                         (repo, expect_display_cmd, autotest_commits_logs))
        actual_return = ad.get_pushed_commits(repo, 'test', '123..789')

        run_cmd.assert_called_with(expect_git_log_cmd, stream_output=True)
        self.assertEqual(expect_return, actual_return)

        #Test to get pushed commits for chromite repo.
        repo = 'chromite'
        expect_git_log_cmd = 'git log --oneline 123..789'
        expect_return = ('\n%s:\n%s\n%s\n' %
                         (repo, expect_git_log_cmd, fake_commits_logs))
        actual_return = ad.get_pushed_commits(repo, 'test', '123..789')

        run_cmd.assert_called_with(expect_git_log_cmd, stream_output=True)
        self.assertEqual(expect_return, actual_return)


if __name__ == '__main__':
    unittest.main()
