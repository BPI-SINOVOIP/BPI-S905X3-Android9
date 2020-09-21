#
# Copyright 2008 Google Inc. All Rights Reserved.

"""Test for cli."""

import os
import sys
import unittest

import common
from autotest_lib.cli import atest, rpc
from autotest_lib.frontend.afe import rpc_client_lib
from autotest_lib.frontend.afe.json_rpc import proxy
from autotest_lib.client.common_lib.test_utils import mock
from autotest_lib.client.common_lib import autotemp

CLI_USING_PDB = False
CLI_UT_DEBUG = False


class ExitException(Exception):
    """Junk that should be removed."""
    pass

def create_file(content):
    """Create a temporary file for testing.

    @param content: string contents for file.

    @return: Instance of autotemp.tempfile with specified contents.
    """
    file_temp = autotemp.tempfile(unique_id='cli_mock', text=True)
    os.write(file_temp.fd, content)
    return file_temp


class cli_unittest(unittest.TestCase):
    """General mocks and setup / teardown for testing the atest cli.
    """
    def setUp(self):
        """Setup mocks for rpc calls and system exit.
        """
        super(cli_unittest, self).setUp()
        self.god = mock.mock_god(debug=CLI_UT_DEBUG, ut=self)
        self.god.stub_class_method(rpc.afe_comm, 'run')
        self.god.stub_function(sys, 'exit')

        def stub_authorization_headers(*args, **kwargs):
            """No auth headers required for testing."""
            return {}
        self.god.stub_with(rpc_client_lib, 'authorization_headers',
                           stub_authorization_headers)


    def tearDown(self):
        """Remove mocks.
        """
        # Unstub first because super may need exit
        self.god.unstub_all()
        super(cli_unittest, self).tearDown()


    def assertEqualNoOrder(self, x, y, message=None):
        """Assert x and y contain the same elements.

        @param x: list like object for comparison.
        @param y: second list like object for comparison
        @param message: Message for AssertionError if x and y contain different
                        elements.

        @raises: AssertionError
        """
        self.assertEqual(set(x), set(y), message)


    def assertWords(self, string, to_find=[], not_in=[]):
        """Assert the string contains all of the set of words to_find and none
        of the set not_in.

        @param string: String to search.
        @param to_find: List of strings that must be in string.
        @param not_in: List of strings that must NOT be in string.

        @raises: AssertionError
        """
        for word in to_find:
            self.assert_(string.find(word) >= 0,
                         "Could not find '%s' in: %s" % (word, string))
        for word in not_in:
            self.assert_(string.find(word) < 0,
                         "Found (and shouldn't have) '%s' in: %s" % (word,
                                                                     string))


    def _check_output(self, out='', out_words_ok=[], out_words_no=[],
                      err='', err_words_ok=[], err_words_no=[]):
        if out_words_ok or out_words_no:
            self.assertWords(out, out_words_ok, out_words_no)
        else:
            self.assertEqual('', out)

        if err_words_ok or err_words_no:
            self.assertWords(err, err_words_ok, err_words_no)
        else:
            self.assertEqual('', err)


    def assertOutput(self, obj, results,
                     out_words_ok=[], out_words_no=[],
                     err_words_ok=[], err_words_no=[]):
        """Assert that obj's output writes the expected strings to std(out/err).

        An empty list for out_words_ok or err_words_ok means that the stdout
        or stderr (respectively) must be empty.

        @param obj: Command object (such as atest_add_or_remove).
        @param results: Results of command for obj.output to format.
        @param out_words_ok: List of strings that must be in stdout.
        @param out_words_no: List of strings that must NOT be in stdout.
        @param err_words_ok: List of strings that must be in stderr.
        @param err_words_no: List of strings that must NOT be in stderr.

        @raises: AssertionError
        """
        self.god.mock_io()
        obj.output(results)
        obj.show_all_failures()
        (out, err) = self.god.unmock_io()
        self._check_output(out, out_words_ok, out_words_no,
                           err, err_words_ok, err_words_no)


    def mock_rpcs(self, rpcs):
        """Expect and mock the results of a list of RPCs.

        @param rpcs: A list of tuples, each representing one RPC:
                     (op, args(dict), success, expected)
        """
        for (op, dargs, success, expected) in rpcs:
            comm = rpc.afe_comm.run
            if success:
                comm.expect_call(op, **dargs).and_return(expected)
            else:
                (comm.expect_call(op, **dargs).
                 and_raises(proxy.JSONRPCException(expected)))


    def run_cmd(self, argv, rpcs=[], exit_code=None,
                out_words_ok=[], out_words_no=[],
                err_words_ok=[], err_words_no=[]):
        """Run an atest command with arguments.

        An empty list for out_words_ok or err_words_ok means that the stdout
        or stderr (respectively) must be empty.

        @param argv: List of command and arguments as strings.
        @param rpcs: List of rpcs to expect the command to perform.
        @param exit_code: Expected exit code of the command (if not 0).
        @param out_words_ok: List of strings to expect in stdout.
        @param out_words_no: List of strings that must not be in stdout.
        @param err_words_ok: List of strings to expect in stderr.
        @param err_words_no: List of strings that must not be in stderr.

        @raises: AssertionError or CheckPlaybackError.

        @returns: stdout, stderr
        """
        sys.argv = argv

        self.mock_rpcs(rpcs)

        if not (CLI_USING_PDB and CLI_UT_DEBUG):
            self.god.mock_io()
        if exit_code is not None:
            sys.exit.expect_call(exit_code).and_raises(ExitException)
            self.assertRaises(ExitException, atest.main)
        else:
            atest.main()
        (out, err) = self.god.unmock_io()
        self.god.check_playback()
        self._check_output(out, out_words_ok, out_words_no,
                           err, err_words_ok, err_words_no)
        return (out, err)

