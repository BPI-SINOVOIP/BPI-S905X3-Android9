#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for client/common_lib/cros/dev_server.py."""

import __builtin__

import httplib
import json
import mox
import os
import StringIO
import time
import unittest
import urllib2

import mock

import common
from autotest_lib.client.bin import utils as bin_utils
from autotest_lib.client.common_lib import android_utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.client.common_lib.cros import retry


def retry_mock(ExceptionToCheck, timeout_min, exception_to_raise=None,
               label=None):
    """A mock retry decorator to use in place of the actual one for testing.

    @param ExceptionToCheck: the exception to check.
    @param timeout_mins: Amount of time in mins to wait before timing out.
    @param exception_to_raise: the exception to raise in retry.retry
    @param label: used in debug messages

    """
    def inner_retry(func):
        """The actual decorator.

        @param func: Function to be called in decorator.

        """
        return func

    return inner_retry


class MockSshResponse(object):
    """An ssh response mocked for testing."""

    def __init__(self, output, exit_status=0):
        self.stdout = output
        self.exit_status = exit_status
        self.stderr = 'SSH connection error occurred.'


class MockSshError(error.CmdError):
    """An ssh error response mocked for testing."""

    def __init__(self):
        self.result_obj = MockSshResponse('error', exit_status=255)


E403 = urllib2.HTTPError(url='',
                         code=httplib.FORBIDDEN,
                         msg='Error 403',
                         hdrs=None,
                         fp=StringIO.StringIO('Expected.'))
E500 = urllib2.HTTPError(url='',
                         code=httplib.INTERNAL_SERVER_ERROR,
                         msg='Error 500',
                         hdrs=None,
                         fp=StringIO.StringIO('Expected.'))
CMD_ERROR = error.CmdError('error_cmd', MockSshError().result_obj)


class RunCallTest(mox.MoxTestBase):
    """Unit tests for ImageServerBase.run_call or DevServer.run_call."""

    def setUp(self):
        """Set up the test"""
        self.test_call = 'http://nothing/test'
        self.hostname = 'nothing'
        self.contents = 'true'
        self.contents_readline = ['file/one', 'file/two']
        self.save_ssh_config = dev_server.ENABLE_SSH_CONNECTION_FOR_DEVSERVER
        super(RunCallTest, self).setUp()
        self.mox.StubOutWithMock(urllib2, 'urlopen')
        self.mox.StubOutWithMock(utils, 'run')

        sleep = mock.patch('time.sleep', autospec=True)
        sleep.start()
        self.addCleanup(sleep.stop)


    def tearDown(self):
        """Tear down the test"""
        dev_server.ENABLE_SSH_CONNECTION_FOR_DEVSERVER = self.save_ssh_config
        super(RunCallTest, self).tearDown()


    def testRunCallHTTPWithDownDevserver(self):
        """Test dev_server.ImageServerBase.run_call using http with arg:
        (call)."""
        dev_server.ENABLE_SSH_CONNECTION_FOR_DEVSERVER = False

        urllib2.urlopen(mox.StrContains(self.test_call)).AndReturn(
                StringIO.StringIO(dev_server.ERR_MSG_FOR_DOWN_DEVSERVER))
        time.sleep(mox.IgnoreArg())
        urllib2.urlopen(mox.StrContains(self.test_call)).AndReturn(
                StringIO.StringIO(self.contents))
        self.mox.ReplayAll()
        response = dev_server.ImageServerBase.run_call(self.test_call)
        self.assertEquals(self.contents, response)


    def testRunCallSSHWithDownDevserver(self):
        """Test dev_server.ImageServerBase.run_call using http with arg:
        (call)."""
        dev_server.ENABLE_SSH_CONNECTION_FOR_DEVSERVER = True
        self.mox.StubOutWithMock(utils, 'get_restricted_subnet')
        utils.get_restricted_subnet(
                self.hostname, utils.RESTRICTED_SUBNETS).AndReturn(
                self.hostname)

        to_return1 = MockSshResponse(dev_server.ERR_MSG_FOR_DOWN_DEVSERVER)
        to_return2 = MockSshResponse(self.contents)
        utils.run(mox.StrContains(self.test_call),
                  timeout=mox.IgnoreArg()).AndReturn(to_return1)
        time.sleep(mox.IgnoreArg())
        utils.run(mox.StrContains(self.test_call),
                  timeout=mox.IgnoreArg()).AndReturn(to_return2)

        self.mox.ReplayAll()
        response = dev_server.ImageServerBase.run_call(self.test_call)
        self.assertEquals(self.contents, response)
        dev_server.ENABLE_SSH_CONNECTION_FOR_DEVSERVER = False


    def testRunCallWithSingleCallHTTP(self):
        """Test dev_server.ImageServerBase.run_call using http with arg:
        (call)."""
        dev_server.ENABLE_SSH_CONNECTION_FOR_DEVSERVER = False

        urllib2.urlopen(mox.StrContains(self.test_call)).AndReturn(
                StringIO.StringIO(self.contents))
        self.mox.ReplayAll()
        response = dev_server.ImageServerBase.run_call(self.test_call)
        self.assertEquals(self.contents, response)


    def testRunCallWithCallAndReadlineHTTP(self):
        """Test dev_server.ImageServerBase.run_call using http with arg:
        (call, readline=True)."""
        dev_server.ENABLE_SSH_CONNECTION_FOR_DEVSERVER = False

        urllib2.urlopen(mox.StrContains(self.test_call)).AndReturn(
                StringIO.StringIO('\n'.join(self.contents_readline)))
        self.mox.ReplayAll()
        response = dev_server.ImageServerBase.run_call(
                self.test_call, readline=True)
        self.assertEquals(self.contents_readline, response)


    def testRunCallWithCallAndTimeoutHTTP(self):
        """Test dev_server.ImageServerBase.run_call using http with args:
        (call, timeout=xxx)."""
        dev_server.ENABLE_SSH_CONNECTION_FOR_DEVSERVER = False

        urllib2.urlopen(mox.StrContains(self.test_call), data=None).AndReturn(
                StringIO.StringIO(self.contents))
        self.mox.ReplayAll()
        response = dev_server.ImageServerBase.run_call(
                self.test_call, timeout=60)
        self.assertEquals(self.contents, response)


    def testRunCallWithSingleCallSSH(self):
        """Test dev_server.ImageServerBase.run_call using ssh with arg:
        (call)."""
        dev_server.ENABLE_SSH_CONNECTION_FOR_DEVSERVER = True
        self.mox.StubOutWithMock(utils, 'get_restricted_subnet')
        utils.get_restricted_subnet(
                self.hostname, utils.RESTRICTED_SUBNETS).AndReturn(
                self.hostname)

        to_return = MockSshResponse(self.contents)
        utils.run(mox.StrContains(self.test_call),
                  timeout=mox.IgnoreArg()).AndReturn(to_return)
        self.mox.ReplayAll()
        response = dev_server.ImageServerBase.run_call(self.test_call)
        self.assertEquals(self.contents, response)


    def testRunCallWithCallAndReadlineSSH(self):
        """Test dev_server.ImageServerBase.run_call using ssh with args:
        (call, readline=True)."""
        dev_server.ENABLE_SSH_CONNECTION_FOR_DEVSERVER = True
        self.mox.StubOutWithMock(utils, 'get_restricted_subnet')
        utils.get_restricted_subnet(
                self.hostname, utils.RESTRICTED_SUBNETS).AndReturn(
                self.hostname)

        to_return = MockSshResponse('\n'.join(self.contents_readline))
        utils.run(mox.StrContains(self.test_call),
                  timeout=mox.IgnoreArg()).AndReturn(to_return)
        self.mox.ReplayAll()
        response = dev_server.ImageServerBase.run_call(
                self.test_call, readline=True)
        self.assertEquals(self.contents_readline, response)


    def testRunCallWithCallAndTimeoutSSH(self):
        """Test dev_server.ImageServerBase.run_call using ssh with args:
        (call, timeout=xxx)."""
        dev_server.ENABLE_SSH_CONNECTION_FOR_DEVSERVER = True
        self.mox.StubOutWithMock(utils, 'get_restricted_subnet')
        utils.get_restricted_subnet(
                self.hostname, utils.RESTRICTED_SUBNETS).AndReturn(
                self.hostname)

        to_return = MockSshResponse(self.contents)
        utils.run(mox.StrContains(self.test_call),
                  timeout=mox.IgnoreArg()).AndReturn(to_return)
        self.mox.ReplayAll()
        response = dev_server.ImageServerBase.run_call(
                self.test_call, timeout=60)
        self.assertEquals(self.contents, response)


    def testRunCallWithExceptionHTTP(self):
        """Test dev_server.ImageServerBase.run_call using http with raising
        exception."""
        dev_server.ENABLE_SSH_CONNECTION_FOR_DEVSERVER = False
        urllib2.urlopen(mox.StrContains(self.test_call)).AndRaise(E500)
        self.mox.ReplayAll()
        self.assertRaises(urllib2.HTTPError,
                          dev_server.ImageServerBase.run_call,
                          self.test_call)


    def testRunCallWithExceptionSSH(self):
        """Test dev_server.ImageServerBase.run_call using ssh with raising
        exception."""
        dev_server.ENABLE_SSH_CONNECTION_FOR_DEVSERVER = True
        self.mox.StubOutWithMock(utils, 'get_restricted_subnet')
        utils.get_restricted_subnet(
                self.hostname, utils.RESTRICTED_SUBNETS).AndReturn(
                self.hostname)

        utils.run(mox.StrContains(self.test_call),
                  timeout=mox.IgnoreArg()).AndRaise(MockSshError())
        self.mox.ReplayAll()
        self.assertRaises(error.CmdError,
                          dev_server.ImageServerBase.run_call,
                          self.test_call)


    def testRunCallByDevServerHTTP(self):
        """Test dev_server.DevServer.run_call, which uses http, and can be
        directly called by CrashServer."""
        urllib2.urlopen(
                mox.StrContains(self.test_call), data=None).AndReturn(
                        StringIO.StringIO(self.contents))
        self.mox.ReplayAll()
        response = dev_server.DevServer.run_call(
               self.test_call, timeout=60)
        self.assertEquals(self.contents, response)


class DevServerTest(mox.MoxTestBase):
    """Unit tests for dev_server.DevServer.

    @var _HOST: fake dev server host address.
    """

    _HOST = 'http://nothing'
    _CRASH_HOST = 'http://nothing-crashed'
    _CONFIG = global_config.global_config


    def setUp(self):
        """Set up the test"""
        super(DevServerTest, self).setUp()
        self.crash_server = dev_server.CrashServer(DevServerTest._CRASH_HOST)
        self.dev_server = dev_server.ImageServer(DevServerTest._HOST)
        self.android_dev_server = dev_server.AndroidBuildServer(
                DevServerTest._HOST)
        self.mox.StubOutWithMock(dev_server.ImageServerBase, 'run_call')
        self.mox.StubOutWithMock(urllib2, 'urlopen')
        self.mox.StubOutWithMock(utils, 'run')
        self.mox.StubOutWithMock(os.path, 'exists')
        # Hide local restricted_subnets setting.
        dev_server.RESTRICTED_SUBNETS = []
        self.mox.StubOutWithMock(dev_server.ImageServer,
                                 '_read_json_response_from_devserver')

        sleep = mock.patch('time.sleep', autospec=True)
        sleep.start()
        self.addCleanup(sleep.stop)


    def testSimpleResolve(self):
        """One devserver, verify we resolve to it."""
        self.mox.StubOutWithMock(dev_server, '_get_dev_server_list')
        self.mox.StubOutWithMock(dev_server.ImageServer, 'devserver_healthy')
        dev_server._get_dev_server_list().MultipleTimes().AndReturn(
                [DevServerTest._HOST])
        dev_server.ImageServer.devserver_healthy(DevServerTest._HOST).AndReturn(
                                                                        True)
        self.mox.ReplayAll()
        devserver = dev_server.ImageServer.resolve('my_build')
        self.assertEquals(devserver.url(), DevServerTest._HOST)


    def testResolveWithFailure(self):
        """Ensure we rehash on a failed ping on a bad_host."""
        self.mox.StubOutWithMock(dev_server, '_get_dev_server_list')
        bad_host, good_host = 'http://bad_host:99', 'http://good_host:8080'
        dev_server._get_dev_server_list().MultipleTimes().AndReturn(
                [bad_host, good_host])
        argument1 = mox.StrContains(bad_host)
        argument2 = mox.StrContains(good_host)

        # Mock out bad ping failure to bad_host by raising devserver exception.
        dev_server.ImageServerBase.run_call(
                argument1, timeout=mox.IgnoreArg()).AndRaise(
                        dev_server.DevServerException())
        # Good host is good.
        dev_server.ImageServerBase.run_call(
                argument2, timeout=mox.IgnoreArg()).AndReturn(
                        '{"free_disk": 1024}')

        self.mox.ReplayAll()
        host = dev_server.ImageServer.resolve(0) # Using 0 as it'll hash to 0.
        self.assertEquals(host.url(), good_host)
        self.mox.VerifyAll()


    def testResolveWithFailureURLError(self):
        """Ensure we rehash on a failed ping using http on a bad_host after
        urlerror."""
        # Set retry.retry to retry_mock for just returning the original
        # method for this test. This is to save waiting time for real retry,
        # which is defined by dev_server.DEVSERVER_SSH_TIMEOUT_MINS.
        # Will reset retry.retry to real retry at the end of this test.
        real_retry = retry.retry
        retry.retry = retry_mock

        self.mox.StubOutWithMock(dev_server, '_get_dev_server_list')
        bad_host, good_host = 'http://bad_host:99', 'http://good_host:8080'
        dev_server._get_dev_server_list().MultipleTimes().AndReturn(
                [bad_host, good_host])
        argument1 = mox.StrContains(bad_host)
        argument2 = mox.StrContains(good_host)

        # Mock out bad ping failure to bad_host by raising devserver exception.
        dev_server.ImageServerBase.run_call(
                argument1, timeout=mox.IgnoreArg()).MultipleTimes().AndRaise(
                        urllib2.URLError('urlopen connection timeout'))

        # Good host is good.
        dev_server.ImageServerBase.run_call(
                argument2, timeout=mox.IgnoreArg()).AndReturn(
                        '{"free_disk": 1024}')

        self.mox.ReplayAll()
        host = dev_server.ImageServer.resolve(0) # Using 0 as it'll hash to 0.
        self.assertEquals(host.url(), good_host)
        self.mox.VerifyAll()

        retry.retry = real_retry


    def testResolveWithManyDevservers(self):
        """Should be able to return different urls with multiple devservers."""
        self.mox.StubOutWithMock(dev_server.ImageServer, 'servers')
        self.mox.StubOutWithMock(dev_server.DevServer, 'devserver_healthy')

        host0_expected = 'http://host0:8080'
        host1_expected = 'http://host1:8082'

        dev_server.ImageServer.servers().MultipleTimes().AndReturn(
                [host0_expected, host1_expected])
        dev_server.ImageServer.devserver_healthy(host0_expected).AndReturn(True)
        dev_server.ImageServer.devserver_healthy(host1_expected).AndReturn(True)

        self.mox.ReplayAll()
        host0 = dev_server.ImageServer.resolve(0)
        host1 = dev_server.ImageServer.resolve(1)
        self.mox.VerifyAll()

        self.assertEqual(host0.url(), host0_expected)
        self.assertEqual(host1.url(), host1_expected)


    def testCmdErrorRetryCollectAULog(self):
        """Devserver should retry _collect_au_log() on CMDError,
        but pass through real exception."""
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(CMD_ERROR)
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(E500)
        self.mox.ReplayAll()
        self.assertFalse(self.dev_server.collect_au_log(
                '100.0.0.0', 100, 'path/'))


    def testURLErrorRetryCollectAULog(self):
        """Devserver should retry _collect_au_log() on URLError,
        but pass through real exception."""
        self.mox.StubOutWithMock(time, 'sleep')

        refused = urllib2.URLError('[Errno 111] Connection refused')
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(refused)
        time.sleep(mox.IgnoreArg())
        dev_server.ImageServerBase.run_call(mox.IgnoreArg()).AndRaise(E403)
        self.mox.ReplayAll()
        self.assertFalse(self.dev_server.collect_au_log(
                '100.0.0.0', 100, 'path/'))


    def testCmdErrorRetryKillAUProcess(self):
        """Devserver should retry _kill_au_process() on CMDError,
        but pass through real exception."""
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(CMD_ERROR)
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(E500)
        self.mox.ReplayAll()
        self.assertFalse(self.dev_server.kill_au_process_for_host(
                '100.0.0.0', 100))


    def testURLErrorRetryKillAUProcess(self):
        """Devserver should retry _kill_au_process() on URLError,
        but pass through real exception."""
        self.mox.StubOutWithMock(time, 'sleep')

        refused = urllib2.URLError('[Errno 111] Connection refused')
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(refused)
        time.sleep(mox.IgnoreArg())
        dev_server.ImageServerBase.run_call(mox.IgnoreArg()).AndRaise(E403)
        self.mox.ReplayAll()
        self.assertFalse(self.dev_server.kill_au_process_for_host(
                '100.0.0.0', 100))


    def testCmdErrorRetryCleanTrackLog(self):
        """Devserver should retry _clean_track_log() on CMDError,
        but pass through real exception."""
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(CMD_ERROR)
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(E500)
        self.mox.ReplayAll()
        self.assertFalse(self.dev_server.clean_track_log('100.0.0.0', 100))


    def testURLErrorRetryCleanTrackLog(self):
        """Devserver should retry _clean_track_log() on URLError,
        but pass through real exception."""
        self.mox.StubOutWithMock(time, 'sleep')

        refused = urllib2.URLError('[Errno 111] Connection refused')
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(refused)
        time.sleep(mox.IgnoreArg())
        dev_server.ImageServerBase.run_call(mox.IgnoreArg()).AndRaise(E403)
        self.mox.ReplayAll()
        self.assertFalse(self.dev_server.clean_track_log('100.0.0.0', 100))


    def _mockWriteFile(self):
        """Mock write content to a file."""
        mock_file = self.mox.CreateMockAnything()
        open(mox.IgnoreArg(), 'w').AndReturn(mock_file)
        mock_file.__enter__().AndReturn(mock_file)
        mock_file.write(mox.IgnoreArg())
        mock_file.__exit__(None, None, None)


    def _preSetupForAutoUpdate(self, **kwargs):
        """Pre-setup for testing auto_update logics and error handling in
        devserver."""
        response1 = (True, 100)
        response2 = (True, 'Completed')
        response3 = {'host_logs': {'a': 'log'}, 'cros_au_log': 'logs'}

        argument1 = mox.And(mox.StrContains(self._HOST),
                            mox.StrContains('cros_au'))
        argument2 = mox.And(mox.StrContains(self._HOST),
                            mox.StrContains('get_au_status'))
        argument3 = mox.And(mox.StrContains(self._HOST),
                            mox.StrContains('collect_cros_au_log'))
        argument4 = mox.And(mox.StrContains(self._HOST),
                            mox.StrContains('handler_cleanup'))
        argument5 = mox.And(mox.StrContains(self._HOST),
                            mox.StrContains('kill_au_proc'))

        retry_error = None
        if 'retry_error' in kwargs:
            retry_error = kwargs['retry_error']

        raised_error = E403
        if 'raised_error' in kwargs:
            raised_error = kwargs['raised_error']

        if 'cros_au_error' in kwargs:
            if retry_error:
                dev_server.ImageServerBase.run_call(argument1).AndRaise(
                        retry_error)
                time.sleep(mox.IgnoreArg())

            if kwargs['cros_au_error']:
                dev_server.ImageServerBase.run_call(argument1).AndRaise(
                        raised_error)
            else:
                dev_server.ImageServerBase.run_call(argument1).AndReturn(
                        json.dumps(response1))

        if 'get_au_status_error' in kwargs:
            if retry_error:
                dev_server.ImageServerBase.run_call(argument2).AndRaise(
                        retry_error)
                time.sleep(mox.IgnoreArg())

            if kwargs['get_au_status_error']:
                dev_server.ImageServerBase.run_call(argument2).AndRaise(
                        raised_error)
            else:
                dev_server.ImageServerBase.run_call(argument2).AndReturn(
                        json.dumps(response2))

        if 'collect_au_log_error' in kwargs:
            if kwargs['collect_au_log_error']:
                dev_server.ImageServerBase.run_call(argument3).AndRaise(
                        raised_error)
            else:
                dev_server.ImageServer._read_json_response_from_devserver(
                        mox.IgnoreArg()).AndReturn(response3)
                dev_server.ImageServerBase.run_call(argument3).AndReturn('log')
                os.path.exists(mox.IgnoreArg()).AndReturn(True)

                # We write two log files: host_log and cros_au_log
                self._mockWriteFile()
                self._mockWriteFile()

        if 'handler_cleanup_error' in kwargs:
            if kwargs['handler_cleanup_error']:
                dev_server.ImageServerBase.run_call(argument4).AndRaise(
                        raised_error)
            else:
                dev_server.ImageServerBase.run_call(argument4).AndReturn('True')

        if 'kill_au_proc_error' in kwargs:
            if kwargs['kill_au_proc_error']:
                dev_server.ImageServerBase.run_call(argument5).AndRaise(
                        raised_error)
            else:
                dev_server.ImageServerBase.run_call(argument5).AndReturn('True')


    def testSuccessfulTriggerAutoUpdate(self):
        """Verify the dev server's auto_update() succeeds."""
        kwargs={'cros_au_error': False, 'get_au_status_error': False,
                'handler_cleanup_error': False}
        self._preSetupForAutoUpdate(**kwargs)

        self.mox.ReplayAll()
        self.dev_server.auto_update('100.0.0.0', '')
        self.mox.VerifyAll()


    def testSuccessfulTriggerAutoUpdateWithCollectingLog(self):
        """Verify the dev server's auto_update() with collecting logs
        succeeds."""
        kwargs={'cros_au_error': False, 'get_au_status_error': False,
                'handler_cleanup_error': False, 'collect_au_log_error': False}
        self.mox.StubOutWithMock(__builtin__, 'open')
        self._preSetupForAutoUpdate(**kwargs)

        self.mox.ReplayAll()
        self.dev_server.auto_update('100.0.0.0', '', log_dir='path/')
        self.mox.VerifyAll()


    def testCrOSAUURLErrorRetryTriggerAutoUpdateSucceed(self):
        """Devserver should retry cros_au() on URLError."""
        self.mox.StubOutWithMock(time, 'sleep')
        refused = urllib2.URLError('[Errno 111] Connection refused')
        kwargs={'retry_error': refused, 'cros_au_error': False,
                'get_au_status_error': False, 'handler_cleanup_error': False,
                'collect_au_log_error': False}
        self.mox.StubOutWithMock(__builtin__, 'open')
        self._preSetupForAutoUpdate(**kwargs)

        self.mox.ReplayAll()
        self.dev_server.auto_update('100.0.0.0', '', log_dir='path/')
        self.mox.VerifyAll()


    def testCrOSAUCmdErrorRetryTriggerAutoUpdateSucceed(self):
        """Devserver should retry cros_au() on CMDError."""
        self.mox.StubOutWithMock(time, 'sleep')
        self.mox.StubOutWithMock(__builtin__, 'open')
        kwargs={'retry_error': CMD_ERROR, 'cros_au_error': False,
                'get_au_status_error': False, 'handler_cleanup_error': False,
                'collect_au_log_error': False}
        self._preSetupForAutoUpdate(**kwargs)

        self.mox.ReplayAll()
        self.dev_server.auto_update('100.0.0.0', '', log_dir='path/')
        self.mox.VerifyAll()


    def testCrOSAUURLErrorRetryTriggerAutoUpdateFail(self):
        """Devserver should retry cros_au() on URLError, but pass through
        real exception."""
        self.mox.StubOutWithMock(time, 'sleep')
        refused = urllib2.URLError('[Errno 111] Connection refused')
        kwargs={'retry_error': refused, 'cros_au_error': True,
                'raised_error': E500}

        for i in range(dev_server.AU_RETRY_LIMIT):
            self._preSetupForAutoUpdate(**kwargs)
            if i < dev_server.AU_RETRY_LIMIT - 1:
                time.sleep(mox.IgnoreArg())

        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.auto_update,
                          '', '')


    def testCrOSAUCmdErrorRetryTriggerAutoUpdateFail(self):
        """Devserver should retry cros_au() on CMDError, but pass through
        real exception."""
        self.mox.StubOutWithMock(time, 'sleep')
        kwargs={'retry_error': CMD_ERROR, 'cros_au_error': True}

        for i in range(dev_server.AU_RETRY_LIMIT):
            self._preSetupForAutoUpdate(**kwargs)
            if i < dev_server.AU_RETRY_LIMIT - 1:
                time.sleep(mox.IgnoreArg())

        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.auto_update,
                          '', '')

    def testBadBuildInAutoUpdate(self):
        """Devsever raises BadBuildException when DUT raises RootfsUpdateError.
        """
        kwargs = {
            'cros_au_error': False, 'get_au_status_error': True,
            'handler_cleanup_error': False,
            'kill_au_proc_error': False,
            'raised_error': urllib2.HTTPError(
                code=httplib.INTERNAL_SERVER_ERROR, msg='', url='', hdrs=None,
                fp=StringIO.StringIO(
                    'RootfsUpdateError: Build xyz failed to boot on dut.'))
        }
        for _ in range(dev_server.AU_RETRY_LIMIT):
            self._preSetupForAutoUpdate(**kwargs)
        self.mox.ReplayAll()
        self.assertRaises(
            dev_server.BadBuildException,
            self.dev_server.auto_update, '100.0.0.0', ''
        )

    def testGetAUStatusErrorInAutoUpdate(self):
        """Verify devserver's auto_update() logics for handling get_au_status
        errors.

        Func auto_update() should call 'handler_cleanup' and 'collect_au_log'
        even if '_trigger_auto_update()' failed.
        """
        self.mox.StubOutWithMock(time, 'sleep')
        self.mox.StubOutWithMock(__builtin__, 'open')
        kwargs={'cros_au_error': False, 'get_au_status_error': True,
                'handler_cleanup_error': False, 'collect_au_log_error': False,
                'kill_au_proc_error': False}

        for i in range(dev_server.AU_RETRY_LIMIT):
            self._preSetupForAutoUpdate(**kwargs)
            if i < dev_server.AU_RETRY_LIMIT - 1:
                time.sleep(mox.IgnoreArg())

        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.auto_update,
                          '100.0.0.0', 'build', log_dir='path/')


    def testCleanUpErrorInAutoUpdate(self):
        """Verify devserver's auto_update() logics for handling handler_cleanup
        errors.

        Func auto_update() should call 'handler_cleanup' and 'collect_au_log'
        no matter '_trigger_auto_update()' succeeds or fails.
        """
        self.mox.StubOutWithMock(time, 'sleep')
        self.mox.StubOutWithMock(__builtin__, 'open')
        kwargs={'cros_au_error': False, 'get_au_status_error': False,
                'handler_cleanup_error': True, 'collect_au_log_error': False,
                'kill_au_proc_error': False}


        for i in range(dev_server.AU_RETRY_LIMIT):
            self._preSetupForAutoUpdate(**kwargs)
            if i < dev_server.AU_RETRY_LIMIT - 1:
                time.sleep(mox.IgnoreArg())

        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.auto_update,
                          '100.0.0.0', 'build', log_dir='path/')


    def testCollectLogErrorInAutoUpdate(self):
        """Verify devserver's auto_update() logics for handling collect_au_log
        errors."""
        self.mox.StubOutWithMock(time, 'sleep')
        kwargs={'cros_au_error': False, 'get_au_status_error': False,
                'handler_cleanup_error': False, 'collect_au_log_error': True,
                'kill_au_proc_error': False}


        for i in range(dev_server.AU_RETRY_LIMIT):
            self._preSetupForAutoUpdate(**kwargs)
            if i < dev_server.AU_RETRY_LIMIT - 1:
                time.sleep(mox.IgnoreArg())

        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.auto_update,
                          '100.0.0.0', 'build', log_dir='path/')


    def testGetAUStatusErrorAndCleanUpErrorInAutoUpdate(self):
        """Verify devserver's auto_update() logics for handling get_au_status
        and handler_cleanup errors.

        Func auto_update() should call 'handler_cleanup' and 'collect_au_log'
        even if '_trigger_auto_update()' fails.
        """
        self.mox.StubOutWithMock(time, 'sleep')
        self.mox.StubOutWithMock(__builtin__, 'open')
        kwargs={'cros_au_error': False, 'get_au_status_error': True,
                'handler_cleanup_error': True, 'collect_au_log_error': False,
                'kill_au_proc_error': False}


        for i in range(dev_server.AU_RETRY_LIMIT):
            self._preSetupForAutoUpdate(**kwargs)
            if i < dev_server.AU_RETRY_LIMIT - 1:
                time.sleep(mox.IgnoreArg())

        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.auto_update,
                          '100.0.0.0', 'build', log_dir='path/')


    def testGetAUStatusErrorAndCleanUpErrorAndCollectLogErrorInAutoUpdate(self):
        """Verify devserver's auto_update() logics for handling get_au_status,
        handler_cleanup, and collect_au_log errors.

        Func auto_update() should call 'handler_cleanup' and 'collect_au_log'
        even if '_trigger_auto_update()' fails.
        """
        self.mox.StubOutWithMock(time, 'sleep')
        kwargs={'cros_au_error': False, 'get_au_status_error': True,
                'handler_cleanup_error': True, 'collect_au_log_error': True,
                'kill_au_proc_error': False}

        for i in range(dev_server.AU_RETRY_LIMIT):
            self._preSetupForAutoUpdate(**kwargs)
            if i < dev_server.AU_RETRY_LIMIT - 1:
                time.sleep(mox.IgnoreArg())

        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.auto_update,
                          '100.0.0.0', 'build', log_dir='path/')


    def testGetAUStatusErrorAndCleanUpErrorAndCollectLogErrorAndKillErrorInAutoUpdate(self):
        """Verify devserver's auto_update() logics for handling get_au_status,
        handler_cleanup, collect_au_log, and kill_au_proc errors.

        Func auto_update() should call 'handler_cleanup' and 'collect_au_log'
        even if '_trigger_auto_update()' fails.
        """
        self.mox.StubOutWithMock(time, 'sleep')

        kwargs={'cros_au_error': False, 'get_au_status_error': True,
                'handler_cleanup_error': True, 'collect_au_log_error': True,
                'kill_au_proc_error': True}

        for i in range(dev_server.AU_RETRY_LIMIT):
            self._preSetupForAutoUpdate(**kwargs)
            if i < dev_server.AU_RETRY_LIMIT - 1:
                time.sleep(mox.IgnoreArg())

        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.auto_update,
                          '100.0.0.0', 'build', log_dir='path/')


    def testSuccessfulTriggerDownloadSync(self):
        """Call the dev server's download method with synchronous=True."""
        name = 'fake/image'
        self.mox.StubOutWithMock(dev_server.ImageServer, '_finish_download')
        argument1 = mox.And(mox.StrContains(self._HOST), mox.StrContains(name),
                            mox.StrContains('stage?'))
        argument2 = mox.And(mox.StrContains(self._HOST), mox.StrContains(name),
                            mox.StrContains('is_staged'))
        dev_server.ImageServerBase.run_call(argument1).AndReturn('Success')
        dev_server.ImageServerBase.run_call(argument2).AndReturn('True')
        self.dev_server._finish_download(name, mox.IgnoreArg(), mox.IgnoreArg())

        # Synchronous case requires a call to finish download.
        self.mox.ReplayAll()
        self.dev_server.trigger_download(name, synchronous=True)
        self.mox.VerifyAll()


    def testSuccessfulTriggerDownloadASync(self):
        """Call the dev server's download method with synchronous=False."""
        name = 'fake/image'
        argument1 = mox.And(mox.StrContains(self._HOST), mox.StrContains(name),
                            mox.StrContains('stage?'))
        argument2 = mox.And(mox.StrContains(self._HOST), mox.StrContains(name),
                            mox.StrContains('is_staged'))
        dev_server.ImageServerBase.run_call(argument1).AndReturn('Success')
        dev_server.ImageServerBase.run_call(argument2).AndReturn('True')

        self.mox.ReplayAll()
        self.dev_server.trigger_download(name, synchronous=False)
        self.mox.VerifyAll()


    def testURLErrorRetryTriggerDownload(self):
        """Should retry on URLError, but pass through real exception."""
        self.mox.StubOutWithMock(time, 'sleep')

        refused = urllib2.URLError('[Errno 111] Connection refused')
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(refused)
        time.sleep(mox.IgnoreArg())
        dev_server.ImageServerBase.run_call(mox.IgnoreArg()).AndRaise(E403)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.trigger_download,
                          '')


    def testErrorTriggerDownload(self):
        """Should call the dev server's download method using http, fail
        gracefully."""
        dev_server.ImageServerBase.run_call(mox.IgnoreArg()).AndRaise(E500)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.trigger_download,
                          '')


    def testForbiddenTriggerDownload(self):
        """Should call the dev server's download method using http,
        get exception."""
        dev_server.ImageServerBase.run_call(mox.IgnoreArg()).AndRaise(E403)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.trigger_download,
                          '')


    def testCmdErrorTriggerDownload(self):
        """Should call the dev server's download method using ssh, retry
        trigger_download when getting error.CmdError, raise exception for
        urllib2.HTTPError."""
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(CMD_ERROR)
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(E500)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.trigger_download,
                          '')


    def testSuccessfulFinishDownload(self):
        """Should successfully call the dev server's finish download method."""
        name = 'fake/image'
        argument1 = mox.And(mox.StrContains(self._HOST),
                            mox.StrContains(name),
                            mox.StrContains('stage?'))
        argument2 = mox.And(mox.StrContains(self._HOST),
                            mox.StrContains(name),
                            mox.StrContains('is_staged'))
        dev_server.ImageServerBase.run_call(argument1).AndReturn('Success')
        dev_server.ImageServerBase.run_call(argument2).AndReturn('True')

        # Synchronous case requires a call to finish download.
        self.mox.ReplayAll()
        self.dev_server.finish_download(name)  # Raises on failure.
        self.mox.VerifyAll()


    def testErrorFinishDownload(self):
        """Should call the dev server's finish download method using http, fail
        gracefully."""
        dev_server.ImageServerBase.run_call(mox.IgnoreArg()).AndRaise(E500)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.finish_download,
                          '')


    def testCmdErrorFinishDownload(self):
        """Should call the dev server's finish download method using ssh,
        retry finish_download when getting error.CmdError, raise exception
        for urllib2.HTTPError."""
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(CMD_ERROR)
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(E500)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.finish_download,
                          '')


    def testListControlFiles(self):
        """Should successfully list control files from the dev server."""
        name = 'fake/build'
        control_files = ['file/one', 'file/two']
        argument = mox.And(mox.StrContains(self._HOST),
                           mox.StrContains(name))
        dev_server.ImageServerBase.run_call(
                argument, readline=True).AndReturn(control_files)

        self.mox.ReplayAll()
        paths = self.dev_server.list_control_files(name)
        self.assertEquals(len(paths), 2)
        for f in control_files:
            self.assertTrue(f in paths)


    def testFailedListControlFiles(self):
        """Should call the dev server's list-files method using http, get
        exception."""
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg(), readline=True).AndRaise(E500)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.list_control_files,
                          '')


    def testExplodingListControlFiles(self):
        """Should call the dev server's list-files method using http, get
        exception."""
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg(), readline=True).AndRaise(E403)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.list_control_files,
                          '')


    def testCmdErrorListControlFiles(self):
        """Should call the dev server's list-files method using ssh, retry
        list_control_files when getting error.CmdError, raise exception for
        urllib2.HTTPError."""
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg(), readline=True).AndRaise(CMD_ERROR)
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg(), readline=True).AndRaise(E500)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.list_control_files,
                          '')

    def testListSuiteControls(self):
        """Should successfully list all contents of control files from the dev
        server."""
        name = 'fake/build'
        control_contents = ['control file one', 'control file two']
        argument = mox.And(mox.StrContains(self._HOST),
                           mox.StrContains(name))
        dev_server.ImageServerBase.run_call(
                argument).AndReturn(json.dumps(control_contents))

        self.mox.ReplayAll()
        file_contents = self.dev_server.list_suite_controls(name)
        self.assertEquals(len(file_contents), 2)
        for f in control_contents:
            self.assertTrue(f in file_contents)


    def testFailedListSuiteControls(self):
        """Should call the dev server's list_suite_controls method using http,
        get exception."""
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(E500)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.list_suite_controls,
                          '')


    def testExplodingListSuiteControls(self):
        """Should call the dev server's list_suite_controls method using http,
        get exception."""
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(E403)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.list_suite_controls,
                          '')


    def testCmdErrorListSuiteControls(self):
        """Should call the dev server's list_suite_controls method using ssh,
        retry list_suite_controls when getting error.CmdError, raise exception
        for urllib2.HTTPError."""
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(CMD_ERROR)
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(E500)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.list_suite_controls,
                          '')


    def testGetControlFile(self):
        """Should successfully get a control file from the dev server."""
        name = 'fake/build'
        file = 'file/one'
        contents = 'Multi-line\nControl File Contents\n'
        argument = mox.And(mox.StrContains(self._HOST),
                            mox.StrContains(name),
                            mox.StrContains(file))
        dev_server.ImageServerBase.run_call(argument).AndReturn(contents)

        self.mox.ReplayAll()
        self.assertEquals(self.dev_server.get_control_file(name, file),
                          contents)


    def testErrorGetControlFile(self):
        """Should try to get the contents of a control file using http, get
        exception."""
        dev_server.ImageServerBase.run_call(mox.IgnoreArg()).AndRaise(E500)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.get_control_file,
                          '', '')


    def testForbiddenGetControlFile(self):
        """Should try to get the contents of a control file using http, get
        exception."""
        dev_server.ImageServerBase.run_call(mox.IgnoreArg()).AndRaise(E403)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.get_control_file,
                          '', '')


    def testCmdErrorGetControlFile(self):
        """Should try to get the contents of a control file using ssh, retry
        get_control_file when getting error.CmdError, raise exception for
        urllib2.HTTPError."""
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(CMD_ERROR)
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(E500)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.get_control_file,
                          '', '')


    def testGetLatestBuild(self):
        """Should successfully return a build for a given target."""
        self.mox.StubOutWithMock(dev_server.ImageServer, 'servers')
        self.mox.StubOutWithMock(dev_server.DevServer, 'devserver_healthy')

        dev_server.ImageServer.servers().AndReturn([self._HOST])
        dev_server.ImageServer.devserver_healthy(self._HOST).AndReturn(True)

        target = 'x86-generic-release'
        build_string = 'R18-1586.0.0-a1-b1514'
        argument = mox.And(mox.StrContains(self._HOST),
                           mox.StrContains(target))
        dev_server.ImageServerBase.run_call(argument).AndReturn(build_string)

        self.mox.ReplayAll()
        build = dev_server.ImageServer.get_latest_build(target)
        self.assertEquals(build_string, build)


    def testGetLatestBuildWithManyDevservers(self):
        """Should successfully return newest build with multiple devservers."""
        self.mox.StubOutWithMock(dev_server.ImageServer, 'servers')
        self.mox.StubOutWithMock(dev_server.DevServer, 'devserver_healthy')

        host0_expected = 'http://host0:8080'
        host1_expected = 'http://host1:8082'

        dev_server.ImageServer.servers().MultipleTimes().AndReturn(
                [host0_expected, host1_expected])

        dev_server.ImageServer.devserver_healthy(host0_expected).AndReturn(True)
        dev_server.ImageServer.devserver_healthy(host1_expected).AndReturn(True)

        target = 'x86-generic-release'
        build_string1 = 'R9-1586.0.0-a1-b1514'
        build_string2 = 'R19-1586.0.0-a1-b3514'
        argument1 = mox.And(mox.StrContains(host0_expected),
                            mox.StrContains(target))
        argument2 = mox.And(mox.StrContains(host1_expected),
                            mox.StrContains(target))
        dev_server.ImageServerBase.run_call(argument1).AndReturn(build_string1)
        dev_server.ImageServerBase.run_call(argument2).AndReturn(build_string2)

        self.mox.ReplayAll()
        build = dev_server.ImageServer.get_latest_build(target)
        self.assertEquals(build_string2, build)


    def testCrashesAreSetToTheCrashServer(self):
        """Should send symbolicate dump rpc calls to crash_server."""
        self.mox.ReplayAll()
        call = self.crash_server.build_call('symbolicate_dump')
        self.assertTrue(call.startswith(self._CRASH_HOST))


    def _stageTestHelper(self, artifacts=[], files=[], archive_url=None):
        """Helper to test combos of files/artifacts/urls with stage call."""
        expected_archive_url = archive_url
        if not archive_url:
            expected_archive_url = 'gs://my_default_url'
            self.mox.StubOutWithMock(dev_server, '_get_image_storage_server')
            dev_server._get_image_storage_server().AndReturn(
                'gs://my_default_url')
            name = 'fake/image'
        else:
            # This is embedded in the archive_url. Not needed.
            name = ''

        argument1 = mox.And(mox.StrContains(expected_archive_url),
                            mox.StrContains(name),
                            mox.StrContains('artifacts=%s' %
                                            ','.join(artifacts)),
                            mox.StrContains('files=%s' % ','.join(files)),
                            mox.StrContains('stage?'))
        argument2 = mox.And(mox.StrContains(expected_archive_url),
                            mox.StrContains(name),
                            mox.StrContains('artifacts=%s' %
                                            ','.join(artifacts)),
                            mox.StrContains('files=%s' % ','.join(files)),
                            mox.StrContains('is_staged'))
        dev_server.ImageServerBase.run_call(argument1).AndReturn('Success')
        dev_server.ImageServerBase.run_call(argument2).AndReturn('True')

        self.mox.ReplayAll()
        self.dev_server.stage_artifacts(name, artifacts, files, archive_url)
        self.mox.VerifyAll()


    def testStageArtifactsBasic(self):
        """Basic functionality to stage artifacts (similar to
        trigger_download)."""
        self._stageTestHelper(artifacts=['full_payload', 'stateful'])


    def testStageArtifactsBasicWithFiles(self):
        """Basic functionality to stage artifacts (similar to
        trigger_download)."""
        self._stageTestHelper(artifacts=['full_payload', 'stateful'],
                              files=['taco_bell.coupon'])


    def testStageArtifactsOnlyFiles(self):
        """Test staging of only file artifacts."""
        self._stageTestHelper(files=['tasty_taco_bell.coupon'])


    def testStageWithArchiveURL(self):
        """Basic functionality to stage artifacts (similar to
        trigger_download)."""
        self._stageTestHelper(files=['tasty_taco_bell.coupon'],
                              archive_url='gs://tacos_galore/my/dir')


    def testStagedFileUrl(self):
        """Sanity tests that the staged file url looks right."""
        devserver_label = 'x86-mario-release/R30-1234.0.0'
        url = self.dev_server.get_staged_file_url('stateful.tgz',
                                                  devserver_label)
        expected_url = '/'.join([self._HOST, 'static', devserver_label,
                                 'stateful.tgz'])
        self.assertEquals(url, expected_url)

        devserver_label = 'something_crazy/that/you_MIGHT/hate'
        url = self.dev_server.get_staged_file_url('chromiumos_image.bin',
                                                  devserver_label)
        expected_url = '/'.join([self._HOST, 'static', devserver_label,
                                 'chromiumos_image.bin'])
        self.assertEquals(url, expected_url)


    def _StageTimeoutHelper(self):
        """Helper class for testing staging timeout."""
        self.mox.StubOutWithMock(dev_server.ImageServer, 'call_and_wait')
        dev_server.ImageServer.call_and_wait(
                call_name='stage',
                artifacts=mox.IgnoreArg(),
                files=mox.IgnoreArg(),
                archive_url=mox.IgnoreArg(),
                error_message=mox.IgnoreArg()).AndRaise(bin_utils.TimeoutError())


    def test_StageArtifactsTimeout(self):
        """Test DevServerException is raised when stage_artifacts timed out."""
        self._StageTimeoutHelper()
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.stage_artifacts,
                          image='fake/image', artifacts=['full_payload'])
        self.mox.VerifyAll()


    def test_TriggerDownloadTimeout(self):
        """Test DevServerException is raised when trigger_download timed out."""
        self._StageTimeoutHelper()
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.trigger_download,
                          image='fake/image')
        self.mox.VerifyAll()


    def test_FinishDownloadTimeout(self):
        """Test DevServerException is raised when finish_download timed out."""
        self._StageTimeoutHelper()
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.finish_download,
                          image='fake/image')
        self.mox.VerifyAll()


    def test_compare_load(self):
        """Test load comparison logic.
        """
        load_high_cpu = {'devserver': 'http://devserver_1:8082',
                         dev_server.DevServer.CPU_LOAD: 100.0,
                         dev_server.DevServer.NETWORK_IO: 1024*1024*1.0,
                         dev_server.DevServer.DISK_IO: 1024*1024.0}
        load_high_network = {'devserver': 'http://devserver_1:8082',
                             dev_server.DevServer.CPU_LOAD: 1.0,
                             dev_server.DevServer.NETWORK_IO: 1024*1024*100.0,
                             dev_server.DevServer.DISK_IO: 1024*1024*1.0}
        load_1 = {'devserver': 'http://devserver_1:8082',
                  dev_server.DevServer.CPU_LOAD: 1.0,
                  dev_server.DevServer.NETWORK_IO: 1024*1024*1.0,
                  dev_server.DevServer.DISK_IO: 1024*1024*2.0}
        load_2 = {'devserver': 'http://devserver_1:8082',
                  dev_server.DevServer.CPU_LOAD: 1.0,
                  dev_server.DevServer.NETWORK_IO: 1024*1024*1.0,
                  dev_server.DevServer.DISK_IO: 1024*1024*1.0}
        self.assertFalse(dev_server._is_load_healthy(load_high_cpu))
        self.assertFalse(dev_server._is_load_healthy(load_high_network))
        self.assertTrue(dev_server._compare_load(load_1, load_2) > 0)


    def _testSuccessfulTriggerDownloadAndroid(self, synchronous=True):
        """Call the dev server's download method with given synchronous
        setting.

        @param synchronous: True to call the download method synchronously.
        """
        target = 'test_target'
        branch = 'test_branch'
        build_id = '123456'
        artifacts = android_utils.AndroidArtifacts.get_artifacts_for_reimage(
                None)
        self.mox.StubOutWithMock(dev_server.AndroidBuildServer,
                                 '_finish_download')
        argument1 = mox.And(mox.StrContains(self._HOST),
                            mox.StrContains(target),
                            mox.StrContains(branch),
                            mox.StrContains(build_id),
                            mox.StrContains('stage?'))
        argument2 = mox.And(mox.StrContains(self._HOST),
                            mox.StrContains(target),
                            mox.StrContains(branch),
                            mox.StrContains(build_id),
                            mox.StrContains('is_staged'))
        dev_server.ImageServerBase.run_call(argument1).AndReturn('Success')
        dev_server.ImageServerBase.run_call(argument2).AndReturn('True')

        if synchronous:
            android_build_info = {'target': target,
                                  'build_id': build_id,
                                  'branch': branch}
            build = dev_server.ANDROID_BUILD_NAME_PATTERN % android_build_info
            self.android_dev_server._finish_download(
                    build, artifacts, '', target=target, build_id=build_id,
                    branch=branch)

        # Synchronous case requires a call to finish download.
        self.mox.ReplayAll()
        self.android_dev_server.trigger_download(
                synchronous=synchronous, target=target, build_id=build_id,
                branch=branch)
        self.mox.VerifyAll()


    def testSuccessfulTriggerDownloadAndroidSync(self):
        """Call the dev server's download method with synchronous=True."""
        self._testSuccessfulTriggerDownloadAndroid(synchronous=True)


    def testSuccessfulTriggerDownloadAndroidAsync(self):
        """Call the dev server's download method with synchronous=False."""
        self._testSuccessfulTriggerDownloadAndroid(synchronous=False)


    def testGetUnrestrictedDevservers(self):
        """Test method get_unrestricted_devservers works as expected."""
        restricted_devserver = 'http://192.168.0.100:8080'
        unrestricted_devserver = 'http://172.1.1.3:8080'
        self.mox.StubOutWithMock(dev_server.ImageServer, 'servers')
        dev_server.ImageServer.servers().AndReturn([restricted_devserver,
                                                    unrestricted_devserver])
        self.mox.ReplayAll()
        self.assertEqual(dev_server.ImageServer.get_unrestricted_devservers(
                                [('192.168.0.0', 24)]),
                         [unrestricted_devserver])


    def testDevserverHealthy(self):
        """Test which types of connections that method devserver_healthy uses
        for different types of DevServer.

        CrashServer always adopts DevServer.run_call.
        ImageServer and AndroidBuildServer use ImageServerBase.run_call.
        """
        argument = mox.StrContains(self._HOST)

        # for testing CrashServer
        self.mox.StubOutWithMock(dev_server.DevServer, 'run_call')
        dev_server.DevServer.run_call(
                argument, timeout=mox.IgnoreArg()).AndReturn(
                        '{"free_disk": 1024}')
        # for testing ImageServer
        dev_server.ImageServerBase.run_call(
                argument, timeout=mox.IgnoreArg()).AndReturn(
                        '{"free_disk": 1024}')
        # for testing AndroidBuildServer
        dev_server.ImageServerBase.run_call(
                argument, timeout=mox.IgnoreArg()).AndReturn(
                        '{"free_disk": 1024}')

        self.mox.ReplayAll()
        self.assertTrue(dev_server.CrashServer.devserver_healthy(self._HOST))
        self.assertTrue(dev_server.ImageServer.devserver_healthy(self._HOST))
        self.assertTrue(
                dev_server.AndroidBuildServer.devserver_healthy(self._HOST))


    def testLocateFile(self):
        """Test locating files for AndriodBuildServer."""
        file_name = 'fake_file'
        artifacts=['full_payload', 'stateful']
        build = 'fake_build'
        argument = mox.And(mox.StrContains(file_name),
                            mox.StrContains(build),
                            mox.StrContains('locate_file'))
        dev_server.ImageServerBase.run_call(argument).AndReturn('file_path')

        self.mox.ReplayAll()
        file_location = 'http://nothing/static/fake_build/file_path'
        self.assertEqual(self.android_dev_server.locate_file(
                file_name, artifacts, build, None), file_location)

    def testCmdErrorLocateFile(self):
        """Test locating files for AndriodBuildServer for retry
        error.CmdError, and raise urllib2.URLError."""
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(CMD_ERROR)
        dev_server.ImageServerBase.run_call(
                mox.IgnoreArg()).AndRaise(E500)
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.trigger_download,
                          '')


    def testGetAvailableDevserversForCrashServer(self):
        """Test method get_available_devservers for CrashServer."""
        crash_servers = ['http://crash_servers1:8080']
        host = '127.0.0.1'
        self.mox.StubOutWithMock(dev_server.CrashServer, 'servers')
        dev_server.CrashServer.servers().AndReturn(crash_servers)
        self.mox.ReplayAll()
        self.assertEqual(dev_server.CrashServer.get_available_devservers(host),
                        (crash_servers, False))


    def testGetAvailableDevserversForImageServer(self):
        """Test method get_available_devservers for ImageServer."""
        unrestricted_host = '100.0.0.99'
        unrestricted_servers = ['http://100.0.0.10:8080',
                                'http://128.0.0.10:8080']
        same_subnet_unrestricted_servers = ['http://100.0.0.10:8080']
        restricted_host = '127.0.0.99'
        restricted_servers = ['http://127.0.0.10:8080']
        all_servers = unrestricted_servers + restricted_servers
        # Set restricted subnets
        restricted_subnets = [('127.0.0.0', 24)]
        self.mox.StubOutWithMock(dev_server.ImageServerBase, 'servers')
        dev_server.ImageServerBase.servers().MultipleTimes().AndReturn(
                all_servers)
        self.mox.ReplayAll()
        # dut in unrestricted subnet shall be offered devserver in the same
        # subnet first, and allow retry.
        self.assertEqual(
                dev_server.ImageServer.get_available_devservers(
                        unrestricted_host, True, restricted_subnets),
                (same_subnet_unrestricted_servers, True))

        # If prefer_local_devserver is set to False, allow any devserver in
        # unrestricted subet to be available, and retry is not allowed.
        self.assertEqual(
                dev_server.ImageServer.get_available_devservers(
                        unrestricted_host, False, restricted_subnets),
                (unrestricted_servers, False))

        # When no hostname is specified, all devservers in unrestricted subnets
        # should be considered, and retry is not allowed.
        self.assertEqual(
                dev_server.ImageServer.get_available_devservers(
                        None, True, restricted_subnets),
                (unrestricted_servers, False))

        # dut in restricted subnet should only be offered devserver in the
        # same restricted subnet, and retry is not allowed.
        self.assertEqual(
                dev_server.ImageServer.get_available_devservers(
                        restricted_host, True, restricted_subnets),
                (restricted_servers, False))


if __name__ == "__main__":
    unittest.main()
