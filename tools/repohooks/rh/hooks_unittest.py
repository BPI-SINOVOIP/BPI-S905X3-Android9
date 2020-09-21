#!/usr/bin/python
# -*- coding:utf-8 -*-
# Copyright 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Unittests for the hooks module."""

from __future__ import print_function

import os
import sys
import unittest

import mock

_path = os.path.realpath(__file__ + '/../..')
if sys.path[0] != _path:
    sys.path.insert(0, _path)
del _path

# We have to import our local modules after the sys.path tweak.  We can't use
# relative imports because this is an executable program, not a module.
# pylint: disable=wrong-import-position
import rh
import rh.hooks
import rh.config


class HooksDocsTests(unittest.TestCase):
    """Make sure all hook features are documented.

    Note: These tests are a bit hokey in that they parse README.md.  But they
    get the job done, so that's all that matters right?
    """

    def setUp(self):
        self.readme = os.path.join(os.path.dirname(os.path.dirname(
            os.path.realpath(__file__))), 'README.md')

    def _grab_section(self, section):
        """Extract the |section| text out of the readme."""
        ret = []
        in_section = False
        for line in open(self.readme):
            if not in_section:
                # Look for the section like "## [Tool Paths]".
                if line.startswith('#') and line.lstrip('#').strip() == section:
                    in_section = True
            else:
                # Once we hit the next section (higher or lower), break.
                if line[0] == '#':
                    break
                ret.append(line)
        return ''.join(ret)

    def testBuiltinHooks(self):
        """Verify builtin hooks are documented."""
        data = self._grab_section('[Builtin Hooks]')
        for hook in rh.hooks.BUILTIN_HOOKS:
            self.assertIn('* `%s`:' % (hook,), data,
                          msg='README.md missing docs for hook "%s"' % (hook,))

    def testToolPaths(self):
        """Verify tools are documented."""
        data = self._grab_section('[Tool Paths]')
        for tool in rh.hooks.TOOL_PATHS:
            self.assertIn('* `%s`:' % (tool,), data,
                          msg='README.md missing docs for tool "%s"' % (tool,))

    def testPlaceholders(self):
        """Verify placeholder replacement vars are documented."""
        data = self._grab_section('Placeholders')
        for var in rh.hooks.Placeholders.vars():
            self.assertIn('* `${%s}`:' % (var,), data,
                          msg='README.md missing docs for var "%s"' % (var,))


class PlaceholderTests(unittest.TestCase):
    """Verify behavior of replacement variables."""

    def setUp(self):
        self._saved_environ = os.environ.copy()
        os.environ.update({
            'PREUPLOAD_COMMIT_MESSAGE': 'commit message',
            'PREUPLOAD_COMMIT': '5c4c293174bb61f0f39035a71acd9084abfa743d',
        })
        self.replacer = rh.hooks.Placeholders()

    def tearDown(self):
        os.environ.clear()
        os.environ.update(self._saved_environ)

    def testVars(self):
        """Light test for the vars inspection generator."""
        ret = list(self.replacer.vars())
        self.assertGreater(len(ret), 4)
        self.assertIn('PREUPLOAD_COMMIT', ret)

    @mock.patch.object(rh.git, 'find_repo_root', return_value='/ ${BUILD_OS}')
    def testExpandVars(self, _m):
        """Verify the replacement actually works."""
        input_args = [
            # Verify ${REPO_ROOT} is updated, but not REPO_ROOT.
            # We also make sure that things in ${REPO_ROOT} are not double
            # expanded (which is why the return includes ${BUILD_OS}).
            '${REPO_ROOT}/some/prog/REPO_ROOT/ok',
            # Verify lists are merged rather than inserted.  In this case, the
            # list is empty, but we'd hit an error still if we saw [] in args.
            '${PREUPLOAD_FILES}',
            # Verify values with whitespace don't expand into multiple args.
            '${PREUPLOAD_COMMIT_MESSAGE}',
            # Verify multiple values get replaced.
            '${PREUPLOAD_COMMIT}^${PREUPLOAD_COMMIT_MESSAGE}',
            # Unknown vars should be left alone.
            '${THIS_VAR_IS_GOOD}',
        ]
        output_args = self.replacer.expand_vars(input_args)
        exp_args = [
            '/ ${BUILD_OS}/some/prog/REPO_ROOT/ok',
            'commit message',
            '5c4c293174bb61f0f39035a71acd9084abfa743d^commit message',
            '${THIS_VAR_IS_GOOD}',
        ]
        self.assertEqual(output_args, exp_args)

    def testTheTester(self):
        """Make sure we have a test for every variable."""
        for var in self.replacer.vars():
            self.assertIn('test%s' % (var,), dir(self),
                          msg='Missing unittest for variable %s' % (var,))

    def testPREUPLOAD_COMMIT_MESSAGE(self):
        """Verify handling of PREUPLOAD_COMMIT_MESSAGE."""
        self.assertEqual(self.replacer.get('PREUPLOAD_COMMIT_MESSAGE'),
                         'commit message')

    def testPREUPLOAD_COMMIT(self):
        """Verify handling of PREUPLOAD_COMMIT."""
        self.assertEqual(self.replacer.get('PREUPLOAD_COMMIT'),
                         '5c4c293174bb61f0f39035a71acd9084abfa743d')

    def testPREUPLOAD_FILES(self):
        """Verify handling of PREUPLOAD_FILES."""
        self.assertEqual(self.replacer.get('PREUPLOAD_FILES'), [])

    @mock.patch.object(rh.git, 'find_repo_root', return_value='/repo!')
    def testREPO_ROOT(self, m):
        """Verify handling of REPO_ROOT."""
        self.assertEqual(self.replacer.get('REPO_ROOT'), m.return_value)

    @mock.patch.object(rh.hooks, '_get_build_os_name', return_value='vapier os')
    def testBUILD_OS(self, m):
        """Verify handling of BUILD_OS."""
        self.assertEqual(self.replacer.get('BUILD_OS'), m.return_value)


class HookOptionsTests(unittest.TestCase):
    """Verify behavior of HookOptions object."""

    @mock.patch.object(rh.hooks, '_get_build_os_name', return_value='vapier os')
    def testExpandVars(self, m):
        """Verify expand_vars behavior."""
        # Simple pass through.
        args = ['who', 'goes', 'there ?']
        self.assertEqual(args, rh.hooks.HookOptions.expand_vars(args))

        # At least one replacement.  Most real testing is in PlaceholderTests.
        args = ['who', 'goes', 'there ?', '${BUILD_OS} is great']
        exp_args = ['who', 'goes', 'there ?', '%s is great' % (m.return_value,)]
        self.assertEqual(exp_args, rh.hooks.HookOptions.expand_vars(args))

    def testArgs(self):
        """Verify args behavior."""
        # Verify initial args to __init__ has higher precedent.
        args = ['start', 'args']
        options = rh.hooks.HookOptions('hook name', args, {})
        self.assertEqual(options.args(), args)
        self.assertEqual(options.args(default_args=['moo']), args)

        # Verify we fall back to default_args.
        args = ['default', 'args']
        options = rh.hooks.HookOptions('hook name', [], {})
        self.assertEqual(options.args(), [])
        self.assertEqual(options.args(default_args=args), args)

    def testToolPath(self):
        """Verify tool_path behavior."""
        options = rh.hooks.HookOptions('hook name', [], {
            'cpplint': 'my cpplint',
        })
        # Check a builtin (and not overridden) tool.
        self.assertEqual(options.tool_path('pylint'), 'pylint')
        # Check an overridden tool.
        self.assertEqual(options.tool_path('cpplint'), 'my cpplint')
        # Check an unknown tool fails.
        self.assertRaises(AssertionError, options.tool_path, 'extra_tool')


class UtilsTests(unittest.TestCase):
    """Verify misc utility functions."""

    def testRunCommand(self):
        """Check _run_command behavior."""
        # Most testing is done against the utils.RunCommand already.
        # pylint: disable=protected-access
        ret = rh.hooks._run_command(['true'])
        self.assertEqual(ret.returncode, 0)

    def testBuildOs(self):
        """Check _get_build_os_name behavior."""
        # Just verify it returns something and doesn't crash.
        # pylint: disable=protected-access
        ret = rh.hooks._get_build_os_name()
        self.assertTrue(isinstance(ret, str))
        self.assertNotEqual(ret, '')

    def testGetHelperPath(self):
        """Check get_helper_path behavior."""
        # Just verify it doesn't crash.  It's a dirt simple func.
        ret = rh.hooks.get_helper_path('booga')
        self.assertTrue(isinstance(ret, str))
        self.assertNotEqual(ret, '')



@mock.patch.object(rh.utils, 'run_command')
@mock.patch.object(rh.hooks, '_check_cmd', return_value=['check_cmd'])
class BuiltinHooksTests(unittest.TestCase):
    """Verify the builtin hooks."""

    def setUp(self):
        self.project = rh.Project(name='project-name', dir='/.../repo/dir',
                                  remote='remote')
        self.options = rh.hooks.HookOptions('hook name', [], {})

    def _test_commit_messages(self, func, accept, msgs):
        """Helper for testing commit message hooks.

        Args:
          func: The hook function to test.
          accept: Whether all the |msgs| should be accepted.
          msgs: List of messages to test.
        """
        for desc in msgs:
            ret = func(self.project, 'commit', desc, (), options=self.options)
            if accept:
                self.assertEqual(
                    ret, None, msg='Should have accepted: {{{%s}}}' % (desc,))
            else:
                self.assertNotEqual(
                    ret, None, msg='Should have rejected: {{{%s}}}' % (desc,))

    def _test_file_filter(self, mock_check, func, files):
        """Helper for testing hooks that filter by files and run external tools.

        Args:
          mock_check: The mock of _check_cmd.
          func: The hook function to test.
          files: A list of files that we'd check.
        """
        # First call should do nothing as there are no files to check.
        ret = func(self.project, 'commit', 'desc', (), options=self.options)
        self.assertEqual(ret, None)
        self.assertFalse(mock_check.called)

        # Second call should include some checks.
        diff = [rh.git.RawDiffEntry(file=x) for x in files]
        ret = func(self.project, 'commit', 'desc', diff, options=self.options)
        self.assertEqual(ret, mock_check.return_value)

    def testTheTester(self, _mock_check, _mock_run):
        """Make sure we have a test for every hook."""
        for hook in rh.hooks.BUILTIN_HOOKS:
            self.assertIn('test_%s' % (hook,), dir(self),
                          msg='Missing unittest for builtin hook %s' % (hook,))

    def test_checkpatch(self, mock_check, _mock_run):
        """Verify the checkpatch builtin hook."""
        ret = rh.hooks.check_checkpatch(
            self.project, 'commit', 'desc', (), options=self.options)
        self.assertEqual(ret, mock_check.return_value)

    def test_clang_format(self, mock_check, _mock_run):
        """Verify the clang_format builtin hook."""
        ret = rh.hooks.check_clang_format(
            self.project, 'commit', 'desc', (), options=self.options)
        self.assertEqual(ret, mock_check.return_value)

    def test_google_java_format(self, mock_check, _mock_run):
        """Verify the google_java_format builtin hook."""
        ret = rh.hooks.check_google_java_format(
            self.project, 'commit', 'desc', (), options=self.options)
        self.assertEqual(ret, mock_check.return_value)

    def test_commit_msg_bug_field(self, _mock_check, _mock_run):
        """Verify the commit_msg_bug_field builtin hook."""
        # Check some good messages.
        self._test_commit_messages(
            rh.hooks.check_commit_msg_bug_field, True, (
                'subj\n\nBug: 1234\n',
                'subj\n\nBug: 1234\nChange-Id: blah\n',
            ))

        # Check some bad messages.
        self._test_commit_messages(
            rh.hooks.check_commit_msg_bug_field, False, (
                'subj',
                'subj\n\nBUG=1234\n',
                'subj\n\nBUG: 1234\n',
            ))

    def test_commit_msg_changeid_field(self, _mock_check, _mock_run):
        """Verify the commit_msg_changeid_field builtin hook."""
        # Check some good messages.
        self._test_commit_messages(
            rh.hooks.check_commit_msg_changeid_field, True, (
                'subj\n\nChange-Id: I1234\n',
            ))

        # Check some bad messages.
        self._test_commit_messages(
            rh.hooks.check_commit_msg_changeid_field, False, (
                'subj',
                'subj\n\nChange-Id: 1234\n',
                'subj\n\nChange-ID: I1234\n',
            ))

    def test_commit_msg_test_field(self, _mock_check, _mock_run):
        """Verify the commit_msg_test_field builtin hook."""
        # Check some good messages.
        self._test_commit_messages(
            rh.hooks.check_commit_msg_test_field, True, (
                'subj\n\nTest: i did done dood it\n',
            ))

        # Check some bad messages.
        self._test_commit_messages(
            rh.hooks.check_commit_msg_test_field, False, (
                'subj',
                'subj\n\nTEST=1234\n',
                'subj\n\nTEST: I1234\n',
            ))

    def test_cpplint(self, mock_check, _mock_run):
        """Verify the cpplint builtin hook."""
        self._test_file_filter(mock_check, rh.hooks.check_cpplint,
                               ('foo.cpp', 'foo.cxx'))

    def test_gofmt(self, mock_check, _mock_run):
        """Verify the gofmt builtin hook."""
        # First call should do nothing as there are no files to check.
        ret = rh.hooks.check_gofmt(
            self.project, 'commit', 'desc', (), options=self.options)
        self.assertEqual(ret, None)
        self.assertFalse(mock_check.called)

        # Second call will have some results.
        diff = [rh.git.RawDiffEntry(file='foo.go')]
        ret = rh.hooks.check_gofmt(
            self.project, 'commit', 'desc', diff, options=self.options)
        self.assertNotEqual(ret, None)

    def test_jsonlint(self, mock_check, _mock_run):
        """Verify the jsonlint builtin hook."""
        # First call should do nothing as there are no files to check.
        ret = rh.hooks.check_json(
            self.project, 'commit', 'desc', (), options=self.options)
        self.assertEqual(ret, None)
        self.assertFalse(mock_check.called)

        # TODO: Actually pass some valid/invalid json data down.

    def test_pylint(self, mock_check, _mock_run):
        """Verify the pylint builtin hook."""
        self._test_file_filter(mock_check, rh.hooks.check_pylint,
                               ('foo.py',))

    def test_xmllint(self, mock_check, _mock_run):
        """Verify the xmllint builtin hook."""
        self._test_file_filter(mock_check, rh.hooks.check_xmllint,
                               ('foo.xml',))


if __name__ == '__main__':
    unittest.main()
