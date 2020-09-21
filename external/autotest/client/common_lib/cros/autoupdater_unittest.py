#!/usr/bin/python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mox
import os
import unittest

import common
import time

import autoupdater
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.test_utils import mock

class TestAutoUpdater(mox.MoxTestBase):
    """Test autoupdater module."""


    def testParseBuildFromUpdateUrlwithUpdate(self):
        """Test that we properly parse the build from an update_url."""
        update_url = ('http://172.22.50.205:8082/update/lumpy-release/'
                      'R27-3837.0.0')
        expected_value = 'lumpy-release/R27-3837.0.0'
        self.assertEqual(autoupdater.url_to_image_name(update_url),
                         expected_value)


    def testCheckVersion_1(self):
        """Test version check methods work for any build.

        Test two methods used to check version, check_version and
        check_version_to_confirm_install, for:
        1. trybot paladin build.
        update version: trybot-lumpy-paladin/R27-3837.0.0-b123
        booted version: 3837.0.2013_03_21_1340

        """
        update_url = ('http://172.22.50.205:8082/update/trybot-lumpy-paladin/'
                      'R27-1111.0.0-b123')
        updater = autoupdater.ChromiumOSUpdater(
                update_url, host=self.mox.CreateMockAnything())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '1111.0.2013_03_21_1340')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '1111.0.0-rc1')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn('1111.0.0')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.0-pgo-generate')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())


    def testCheckVersion_2(self):
        """Test version check methods work for any build.

        Test two methods used to check version, check_version and
        check_version_to_confirm_install, for:
        2. trybot release build.
        update version: trybot-lumpy-release/R27-3837.0.0-b456
        booted version: 3837.0.0

        """
        update_url = ('http://172.22.50.205:8082/update/trybot-lumpy-release/'
                      'R27-2222.0.0-b456')
        updater = autoupdater.ChromiumOSUpdater(
                update_url, host=self.mox.CreateMockAnything())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '2222.0.2013_03_21_1340')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '2222.0.0-rc1')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn('2222.0.0')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.0-pgo-generate')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())


    def testCheckVersion_3(self):
        """Test version check methods work for any build.

        Test two methods used to check version, check_version and
        check_version_to_confirm_install, for:
        3. buildbot official release build.
        update version: lumpy-release/R27-3837.0.0
        booted version: 3837.0.0

        """
        update_url = ('http://172.22.50.205:8082/update/lumpy-release/'
                      'R27-3333.0.0')
        updater = autoupdater.ChromiumOSUpdater(
                update_url, host=self.mox.CreateMockAnything())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '3333.0.2013_03_21_1340')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '3333.0.0-rc1')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn('3333.0.0')
        self.mox.ReplayAll()

        self.assertTrue(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.0-pgo-generate')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())


    def testCheckVersion_4(self):
        """Test version check methods work for any build.

        Test two methods used to check version, check_version and
        check_version_to_confirm_install, for:
        4. non-official paladin rc build.
        update version: lumpy-paladin/R27-3837.0.0-rc7
        booted version: 3837.0.0-rc7

        """
        update_url = ('http://172.22.50.205:8082/update/lumpy-paladin/'
                      'R27-4444.0.0-rc7')
        updater = autoupdater.ChromiumOSUpdater(
                update_url, host=self.mox.CreateMockAnything())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.2013_03_21_1340')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '4444.0.0-rc7')
        self.mox.ReplayAll()

        self.assertTrue(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn('4444.0.0')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.0-pgo-generate')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())


    def testCheckVersion_5(self):
        """Test version check methods work for any build.

        Test two methods used to check version, check_version and
        check_version_to_confirm_install, for:
        5. chrome-perf build.
        update version: lumpy-chrome-perf/R28-3837.0.0-b2996
        booted version: 3837.0.0

        """
        update_url = ('http://172.22.50.205:8082/update/lumpy-chrome-perf/'
                      'R28-4444.0.0-b2996')
        updater = autoupdater.ChromiumOSUpdater(
                update_url, host=self.mox.CreateMockAnything())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.2013_03_21_1340')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '4444.0.0-rc7')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn('4444.0.0')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.0-pgo-generate')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())


    def testCheckVersion_6(self):
        """Test version check methods work for any build.

        Test two methods used to check version, check_version and
        check_version_to_confirm_install, for:
        6. pgo-generate build.
        update version: lumpy-release-pgo-generate/R28-3837.0.0-b2996
        booted version: 3837.0.0-pgo-generate

        """
        update_url = ('http://172.22.50.205:8082/update/lumpy-release-pgo-'
                      'generate/R28-4444.0.0-b2996')
        updater = autoupdater.ChromiumOSUpdater(
                update_url, host=self.mox.CreateMockAnything())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.0-2013_03_21_1340')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '4444.0.0-rc7')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn('4444.0.0')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.0-pgo-generate')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())


    def testCheckVersion_7(self):
        """Test version check methods work for a test-ap build.

        Test two methods used to check version, check_version and
        check_version_to_confirm_install, for:
        6. test-ap build.
        update version: trybot-stumpy-test-ap/R46-7298.0.0-b23
        booted version: 7298.0.0

        """
        update_url = ('http://100.107.160.2:8082/update/trybot-stumpy-test-api'
                      '/R46-7298.0.0-b23')
        updater = autoupdater.ChromiumOSUpdater(
                update_url, host=self.mox.CreateMockAnything())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '7298.0.2015_07_24_1640')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '7298.0.2015_07_24_1640')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn('7298.0.0')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '7298.0.0')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())


    def _host_run_for_update(self, cmd, exception=None,
                             bad_update_status=False):
        """Helper function for AU tests.

        @param host: the test host
        @param cmd: the command to be recorded
        @param exception: the exception to be recorded, or None
        """
        if exception:
            self.host.run(command=cmd).AndRaise(exception)
        else:
            result = self.mox.CreateMockAnything()
            if bad_update_status:
                # Pick randomly one unexpected status
                result.stdout = 'UPDATE_STATUS_UPDATED_NEED_REBOOT'
            else:
                result.stdout = 'UPDATE_STATUS_IDLE'
            result.status = 0
            self.host.run(command=cmd).AndReturn(result)


    def testTriggerUpdate(self):
        """Tests that we correctly handle updater errors."""
        update_url = 'http://server/test/url'
        self.host = self.mox.CreateMockAnything()
        self.mox.StubOutWithMock(self.host, 'run')
        self.mox.StubOutWithMock(autoupdater.ChromiumOSUpdater,
                                 'get_last_update_error')
        self.host.hostname = 'test_host'
        updater_control_bin = '/usr/bin/update_engine_client'
        test_url = 'http://server/test/url'
        expected_wait_cmd = ('%s -status | grep CURRENT_OP' %
                             updater_control_bin)
        expected_cmd = ('%s --check_for_update --omaha_url=%s' %
                        (updater_control_bin, test_url))
        self.mox.StubOutWithMock(time, "sleep")
        UPDATE_ENGINE_RETRY_WAIT_TIME=5

        # Generic SSH Error.
        cmd_result_255 = self.mox.CreateMockAnything()
        cmd_result_255.exit_status = 255

        # Command Failed Error
        cmd_result_1 = self.mox.CreateMockAnything()
        cmd_result_1.exit_status = 1

        # Error 37
        cmd_result_37 = self.mox.CreateMockAnything()
        cmd_result_37.exit_status = 37

        updater = autoupdater.ChromiumOSUpdater(update_url, host=self.host)

        # (SUCCESS) Expect one wait command and one status command.
        self._host_run_for_update(expected_wait_cmd)
        self._host_run_for_update(expected_cmd)

        # (SUCCESS) Test with one retry to wait for update-engine.
        self._host_run_for_update(expected_wait_cmd, exception=
                error.AutoservRunError('non-zero status', cmd_result_1))
        time.sleep(UPDATE_ENGINE_RETRY_WAIT_TIME)
        self._host_run_for_update(expected_wait_cmd)
        self._host_run_for_update(expected_cmd)

        # (SUCCESS) One-time SSH timeout, then success on retry.
        self._host_run_for_update(expected_wait_cmd)
        self._host_run_for_update(expected_cmd, exception=
                error.AutoservSSHTimeout('ssh timed out', cmd_result_255))
        self._host_run_for_update(expected_cmd)

        # (SUCCESS) One-time ERROR 37, then success.
        self._host_run_for_update(expected_wait_cmd)
        self._host_run_for_update(expected_cmd, exception=
                error.AutoservRunError('ERROR_CODE=37', cmd_result_37))
        self._host_run_for_update(expected_cmd)

        # (FAILURE) Bad status of update engine.
        self._host_run_for_update(expected_wait_cmd)
        self._host_run_for_update(expected_cmd, bad_update_status=True,
                                  exception=error.InstallError(
                                      'host is not in installable state'))

        # (FAILURE) Two-time SSH timeout.
        self._host_run_for_update(expected_wait_cmd)
        self._host_run_for_update(expected_cmd, exception=
                error.AutoservSSHTimeout('ssh timed out', cmd_result_255))
        self._host_run_for_update(expected_cmd, exception=
                error.AutoservSSHTimeout('ssh timed out', cmd_result_255))

        # (FAILURE) SSH Permission Error
        self._host_run_for_update(expected_wait_cmd)
        self._host_run_for_update(expected_cmd, exception=
                error.AutoservSshPermissionDeniedError('no permission',
                                                       cmd_result_255))

        # (FAILURE) Other ssh failure
        self._host_run_for_update(expected_wait_cmd)
        self._host_run_for_update(expected_cmd, exception=
                error.AutoservSshPermissionDeniedError('no permission',
                                                       cmd_result_255))
        # (FAILURE) Other error
        self._host_run_for_update(expected_wait_cmd)
        self._host_run_for_update(expected_cmd, exception=
                error.AutoservRunError("unknown error", cmd_result_1))

        self.mox.ReplayAll()

        # Expect success
        updater.trigger_update()
        updater.trigger_update()
        updater.trigger_update()
        updater.trigger_update()

        # Expect errors as listed above
        self.assertRaises(autoupdater.RootFSUpdateError, updater.trigger_update)
        self.assertRaises(autoupdater.RootFSUpdateError, updater.trigger_update)
        self.assertRaises(autoupdater.RootFSUpdateError, updater.trigger_update)
        self.assertRaises(autoupdater.RootFSUpdateError, updater.trigger_update)
        self.assertRaises(autoupdater.RootFSUpdateError, updater.trigger_update)

        self.mox.VerifyAll()


    def testUpdateStateful(self):
        """Tests that we call the stateful update script with the correct args.
        """
        self.mox.StubOutWithMock(autoupdater.ChromiumOSUpdater, '_run')
        self.mox.StubOutWithMock(autoupdater.ChromiumOSUpdater,
                                 'get_stateful_update_script')
        update_url = ('http://172.22.50.205:8082/update/lumpy-chrome-perf/'
                      'R28-4444.0.0-b2996')
        static_update_url = ('http://172.22.50.205:8082/static/'
                             'lumpy-chrome-perf/R28-4444.0.0-b2996')

        # Test with clobber=False.
        autoupdater.ChromiumOSUpdater.get_stateful_update_script().AndReturn(
                autoupdater.ChromiumOSUpdater.REMOTE_STATEFUL_UPDATE_PATH)
        autoupdater.ChromiumOSUpdater._run(
                mox.And(
                        mox.StrContains(
                                autoupdater.ChromiumOSUpdater.
                                REMOTE_STATEFUL_UPDATE_PATH),
                        mox.StrContains(static_update_url),
                        mox.Not(mox.StrContains('--stateful_change=clean'))),
                timeout=mox.IgnoreArg())

        self.mox.ReplayAll()
        updater = autoupdater.ChromiumOSUpdater(update_url)
        updater.update_stateful(clobber=False)
        self.mox.VerifyAll()

        # Test with clobber=True.
        self.mox.ResetAll()
        autoupdater.ChromiumOSUpdater.get_stateful_update_script().AndReturn(
                autoupdater.ChromiumOSUpdater.REMOTE_STATEFUL_UPDATE_PATH)
        autoupdater.ChromiumOSUpdater._run(
                mox.And(
                        mox.StrContains(
                                autoupdater.ChromiumOSUpdater.
                                REMOTE_STATEFUL_UPDATE_PATH),
                        mox.StrContains(static_update_url),
                        mox.StrContains('--stateful_change=clean')),
                timeout=mox.IgnoreArg())
        self.mox.ReplayAll()
        updater = autoupdater.ChromiumOSUpdater(update_url)
        updater.update_stateful(clobber=True)
        self.mox.VerifyAll()


    def testGetStatefulUpdateScript(self):
        """ Test that get_stateful_update_script look for stateful_update.

        Check get_stateful_update_script is trying hard to find
        stateful_update and assert if it can't.

        """
        update_url = ('http://172.22.50.205:8082/update/lumpy-chrome-perf/'
                      'R28-4444.0.0-b2996')
        script_loc = os.path.join(autoupdater.STATEFUL_UPDATE_PATH,
                                  autoupdater.STATEFUL_UPDATE_SCRIPT)
        self.god = mock.mock_god()
        self.god.stub_function(os.path, 'exists')
        host = self.mox.CreateMockAnything()
        updater = autoupdater.ChromiumOSUpdater(update_url, host=host)
        os.path.exists.expect_call(script_loc).and_return(False)
        host.path_exists('/usr/local/bin/stateful_update').AndReturn(False)

        self.mox.ReplayAll()
        # No existing files, no URL, we should assert.
        self.assertRaises(
                autoupdater.ChromiumOSError,
                updater.get_stateful_update_script)
        self.mox.VerifyAll()

        # No existing files, but stateful URL, we will try.
        self.mox.ResetAll()
        os.path.exists.expect_call(script_loc).and_return(True)
        host.send_file(
                script_loc,
                '/tmp/stateful_update', delete_dest=True).AndReturn(True)
        self.mox.ReplayAll()
        self.assertEqual(
                updater.get_stateful_update_script(),
                '/tmp/stateful_update')
        self.mox.VerifyAll()


    def testRollbackRootfs(self):
        """Tests that we correctly rollback the rootfs when requested."""
        self.mox.StubOutWithMock(autoupdater.ChromiumOSUpdater, '_run')
        self.mox.StubOutWithMock(autoupdater.ChromiumOSUpdater,
                                 '_verify_update_completed')
        host = self.mox.CreateMockAnything()
        update_url = 'http://server/test/url'
        host.hostname = 'test_host'

        can_rollback_cmd = ('/usr/bin/update_engine_client --can_rollback')
        rollback_cmd = ('/usr/bin/update_engine_client --rollback '
                        '--follow')

        updater = autoupdater.ChromiumOSUpdater(update_url, host=host)

        # Return an old build which shouldn't call can_rollback.
        updater.host.get_release_version().AndReturn('1234.0.0')
        autoupdater.ChromiumOSUpdater._run(rollback_cmd)
        autoupdater.ChromiumOSUpdater._verify_update_completed()

        self.mox.ReplayAll()
        updater.rollback_rootfs(powerwash=True)
        self.mox.VerifyAll()

        self.mox.ResetAll()
        cmd_result_1 = self.mox.CreateMockAnything()
        cmd_result_1.exit_status = 1

        # Rollback but can_rollback says we can't -- return an error.
        updater.host.get_release_version().AndReturn('5775.0.0')
        autoupdater.ChromiumOSUpdater._run(can_rollback_cmd).AndRaise(
                error.AutoservRunError('can_rollback failed', cmd_result_1))
        self.mox.ReplayAll()
        self.assertRaises(autoupdater.RootFSUpdateError,
                          updater.rollback_rootfs, True)
        self.mox.VerifyAll()

        self.mox.ResetAll()
        # Rollback >= version blacklisted.
        updater.host.get_release_version().AndReturn('5775.0.0')
        autoupdater.ChromiumOSUpdater._run(can_rollback_cmd)
        autoupdater.ChromiumOSUpdater._run(rollback_cmd)
        autoupdater.ChromiumOSUpdater._verify_update_completed()
        self.mox.ReplayAll()
        updater.rollback_rootfs(powerwash=True)
        self.mox.VerifyAll()


if __name__ == '__main__':
  unittest.main()
