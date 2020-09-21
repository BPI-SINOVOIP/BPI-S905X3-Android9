#!/usr/bin/python
#
# Copyright (c) 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for frontend/afe/moblab_rpc_interface.py."""

import __builtin__
# The boto module is only available/used in Moblab for validation of cloud
# storage access. The module is not available in the test lab environment,
# and the import error is handled.
import ConfigParser
import mox
import StringIO
import unittest

import common

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import lsbrelease_utils
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import frontend_test_utils
from autotest_lib.frontend.afe import moblab_rpc_interface
from autotest_lib.frontend.afe import rpc_utils
from autotest_lib.server import utils
from autotest_lib.server.hosts import moblab_host
from autotest_lib.client.common_lib import utils as common_lib_utils


class MoblabRpcInterfaceTest(mox.MoxTestBase,
                             frontend_test_utils.FrontendTestMixin):
    """Unit tests for functions in moblab_rpc_interface.py."""

    def setUp(self):
        super(MoblabRpcInterfaceTest, self).setUp()
        self._frontend_common_setup(fill_data=False)


    def tearDown(self):
        self._frontend_common_teardown()


    def setIsMoblab(self, is_moblab):
        """Set utils.is_moblab result.

        @param is_moblab: Value to have utils.is_moblab to return.
        """
        self.mox.StubOutWithMock(utils, 'is_moblab')
        utils.is_moblab().AndReturn(is_moblab)


    def _mockReadFile(self, path, lines=[]):
        """Mock out reading a file line by line.

        @param path: Path of the file we are mock reading.
        @param lines: lines of the mock file that will be returned when
                      readLine() is called.
        """
        mockFile = self.mox.CreateMockAnything()
        for line in lines:
            mockFile.readline().AndReturn(line)
        mockFile.readline()
        mockFile.close()
        open(path).AndReturn(mockFile)


    def testMoblabOnlyDecorator(self):
        """Ensure the moblab only decorator gates functions properly."""
        self.setIsMoblab(False)
        self.mox.ReplayAll()
        self.assertRaises(error.RPCException,
                          moblab_rpc_interface.get_config_values)


    def testGetConfigValues(self):
        """Ensure that the config object is properly converted to a dict."""
        self.setIsMoblab(True)
        config_mock = self.mox.CreateMockAnything()
        moblab_rpc_interface._CONFIG = config_mock
        config_mock.get_sections().AndReturn(['section1', 'section2'])
        config_mock.config = self.mox.CreateMockAnything()
        config_mock.config.items('section1').AndReturn([('item1', 'value1'),
                                                        ('item2', 'value2')])
        config_mock.config.items('section2').AndReturn([('item3', 'value3'),
                                                        ('item4', 'value4')])

        rpc_utils.prepare_for_serialization(
            {'section1' : [('item1', 'value1'),
                           ('item2', 'value2')],
             'section2' : [('item3', 'value3'),
                           ('item4', 'value4')]})
        self.mox.ReplayAll()
        moblab_rpc_interface.get_config_values()


    def testUpdateConfig(self):
        """Ensure that updating the config works as expected."""
        self.setIsMoblab(True)
        moblab_rpc_interface.os = self.mox.CreateMockAnything()

        self.mox.StubOutWithMock(__builtin__, 'open')
        self._mockReadFile(global_config.DEFAULT_CONFIG_FILE)

        self.mox.StubOutWithMock(lsbrelease_utils, 'is_moblab')
        lsbrelease_utils.is_moblab().AndReturn(True)

        self._mockReadFile(global_config.DEFAULT_MOBLAB_FILE,
                           ['[section1]', 'item1: value1'])

        moblab_rpc_interface.os = self.mox.CreateMockAnything()
        moblab_rpc_interface.os.path = self.mox.CreateMockAnything()
        moblab_rpc_interface.os.path.exists(
                moblab_rpc_interface._CONFIG.shadow_file).AndReturn(
                True)
        mockShadowFile = self.mox.CreateMockAnything()
        mockShadowFileContents = StringIO.StringIO()
        mockShadowFile.__enter__().AndReturn(mockShadowFileContents)
        mockShadowFile.__exit__(mox.IgnoreArg(), mox.IgnoreArg(),
                                mox.IgnoreArg())
        open(moblab_rpc_interface._CONFIG.shadow_file,
             'w').AndReturn(mockShadowFile)
        moblab_rpc_interface.os.system('sudo reboot')

        self.mox.ReplayAll()
        moblab_rpc_interface.update_config_handler(
                {'section1' : [('item1', 'value1'),
                               ('item2', 'value2')],
                 'section2' : [('item3', 'value3'),
                               ('item4', 'value4')]})

        # item1 should not be in the new shadow config as its updated value
        # matches the original config's value.
        self.assertEquals(
                mockShadowFileContents.getvalue(),
                '[section2]\nitem3 = value3\nitem4 = value4\n\n'
                '[section1]\nitem2 = value2\n\n')


    def testResetConfig(self):
        """Ensure that reset opens the shadow_config file for writing."""
        self.setIsMoblab(True)
        config_mock = self.mox.CreateMockAnything()
        moblab_rpc_interface._CONFIG = config_mock
        config_mock.shadow_file = 'shadow_config.ini'
        self.mox.StubOutWithMock(__builtin__, 'open')
        mockFile = self.mox.CreateMockAnything()
        file_contents = self.mox.CreateMockAnything()
        mockFile.__enter__().AndReturn(file_contents)
        mockFile.__exit__(mox.IgnoreArg(), mox.IgnoreArg(), mox.IgnoreArg())
        open(config_mock.shadow_file, 'w').AndReturn(mockFile)
        moblab_rpc_interface.os = self.mox.CreateMockAnything()
        moblab_rpc_interface.os.system('sudo reboot')
        self.mox.ReplayAll()
        moblab_rpc_interface.reset_config_settings()


    def testSetLaunchControlKey(self):
        """Ensure that the Launch Control key path supplied is copied correctly.
        """
        self.setIsMoblab(True)
        launch_control_key = '/tmp/launch_control'
        moblab_rpc_interface.os = self.mox.CreateMockAnything()
        moblab_rpc_interface.os.path = self.mox.CreateMockAnything()
        moblab_rpc_interface.os.path.exists(launch_control_key).AndReturn(
                True)
        moblab_rpc_interface.shutil = self.mox.CreateMockAnything()
        moblab_rpc_interface.shutil.copyfile(
                launch_control_key,
                moblab_host.MOBLAB_LAUNCH_CONTROL_KEY_LOCATION)
        moblab_rpc_interface.os.system('sudo restart moblab-devserver-init')
        self.mox.ReplayAll()
        moblab_rpc_interface.set_launch_control_key(launch_control_key)


    def testGetNetworkInfo(self):
        """Ensure the network info is properly converted to a dict."""
        self.setIsMoblab(True)

        self.mox.StubOutWithMock(moblab_rpc_interface, '_get_network_info')
        moblab_rpc_interface._get_network_info().AndReturn(('10.0.0.1', True))
        self.mox.StubOutWithMock(rpc_utils, 'prepare_for_serialization')

        rpc_utils.prepare_for_serialization(
               {'is_connected': True, 'server_ips': ['10.0.0.1']})
        self.mox.ReplayAll()
        moblab_rpc_interface.get_network_info()
        self.mox.VerifyAll()


    def testGetNetworkInfoWithNoIp(self):
        """Queries network info with no public IP address."""
        self.setIsMoblab(True)

        self.mox.StubOutWithMock(moblab_rpc_interface, '_get_network_info')
        moblab_rpc_interface._get_network_info().AndReturn((None, False))
        self.mox.StubOutWithMock(rpc_utils, 'prepare_for_serialization')

        rpc_utils.prepare_for_serialization(
               {'is_connected': False})
        self.mox.ReplayAll()
        moblab_rpc_interface.get_network_info()
        self.mox.VerifyAll()


    def testGetNetworkInfoWithNoConnectivity(self):
        """Queries network info with public IP address but no connectivity."""
        self.setIsMoblab(True)

        self.mox.StubOutWithMock(moblab_rpc_interface, '_get_network_info')
        moblab_rpc_interface._get_network_info().AndReturn(('10.0.0.1', False))
        self.mox.StubOutWithMock(rpc_utils, 'prepare_for_serialization')

        rpc_utils.prepare_for_serialization(
               {'is_connected': False, 'server_ips': ['10.0.0.1']})
        self.mox.ReplayAll()
        moblab_rpc_interface.get_network_info()
        self.mox.VerifyAll()


    def testGetCloudStorageInfo(self):
        """Ensure the cloud storage info is properly converted to a dict."""
        self.setIsMoblab(True)
        config_mock = self.mox.CreateMockAnything()
        moblab_rpc_interface._CONFIG = config_mock
        config_mock.get_config_value(
            'CROS', 'image_storage_server').AndReturn('gs://bucket1')
        config_mock.get_config_value(
            'CROS', 'results_storage_server', default=None).AndReturn(
                    'gs://bucket2')
        self.mox.StubOutWithMock(moblab_rpc_interface, '_get_boto_config')
        moblab_rpc_interface._get_boto_config().AndReturn(config_mock)
        config_mock.sections().AndReturn(['Credentials', 'b'])
        config_mock.options('Credentials').AndReturn(
            ['gs_access_key_id', 'gs_secret_access_key'])
        config_mock.get(
            'Credentials', 'gs_access_key_id').AndReturn('key')
        config_mock.get(
            'Credentials', 'gs_secret_access_key').AndReturn('secret')
        rpc_utils.prepare_for_serialization(
                {
                    'gs_access_key_id': 'key',
                    'gs_secret_access_key' : 'secret',
                    'use_existing_boto_file': True,
                    'image_storage_server' : 'gs://bucket1',
                    'results_storage_server' : 'gs://bucket2'
                })
        self.mox.ReplayAll()
        moblab_rpc_interface.get_cloud_storage_info()
        self.mox.VerifyAll()


    def testValidateCloudStorageInfo(self):
        """ Ensure the cloud storage info validation flow."""
        self.setIsMoblab(True)
        cloud_storage_info = {
            'use_existing_boto_file': False,
            'gs_access_key_id': 'key',
            'gs_secret_access_key': 'secret',
            'image_storage_server': 'gs://bucket1',
            'results_storage_server': 'gs://bucket2'}
        self.mox.StubOutWithMock(moblab_rpc_interface,
            '_run_bucket_performance_test')
        moblab_rpc_interface._run_bucket_performance_test(
            'key', 'secret', 'gs://bucket1').AndReturn((True, None))
        rpc_utils.prepare_for_serialization({'status_ok': True })
        self.mox.ReplayAll()
        moblab_rpc_interface.validate_cloud_storage_info(cloud_storage_info)
        self.mox.VerifyAll()


    def testGetBucketNameFromUrl(self):
        """Gets bucket name from bucket URL."""
        self.assertEquals(
            'bucket_name-123',
            moblab_rpc_interface._get_bucket_name_from_url(
                    'gs://bucket_name-123'))
        self.assertEquals(
            'bucket_name-123',
            moblab_rpc_interface._get_bucket_name_from_url(
                    'gs://bucket_name-123/'))
        self.assertEquals(
            'bucket_name-123',
            moblab_rpc_interface._get_bucket_name_from_url(
                    'gs://bucket_name-123/a/b/c'))
        self.assertIsNone(moblab_rpc_interface._get_bucket_name_from_url(
            'bucket_name-123/a/b/c'))


    def testGetShadowConfigFromPartialUpdate(self):
        """Tests getting shadow configuration based on partial upate."""
        partial_config = {
                'section1': [
                    ('opt1', 'value1'),
                    ('opt2', 'value2'),
                    ('opt3', 'value3'),
                    ('opt4', 'value4'),
                    ]
                }
        shadow_config_str = "[section1]\nopt2 = value2_1\nopt4 = value4_1"
        shadow_config = ConfigParser.ConfigParser()
        shadow_config.readfp(StringIO.StringIO(shadow_config_str))
        original_config = self.mox.CreateMockAnything()
        self.mox.StubOutWithMock(moblab_rpc_interface, '_read_original_config')
        self.mox.StubOutWithMock(moblab_rpc_interface, '_read_raw_config')
        moblab_rpc_interface._read_original_config().AndReturn(original_config)
        moblab_rpc_interface._read_raw_config(
                moblab_rpc_interface._CONFIG.shadow_file).AndReturn(shadow_config)
        original_config.get_config_value(
                'section1', 'opt1',
                allow_blank=True, default='').AndReturn('value1')
        original_config.get_config_value(
                'section1', 'opt2',
                allow_blank=True, default='').AndReturn('value2')
        original_config.get_config_value(
                'section1', 'opt3',
                allow_blank=True, default='').AndReturn('blah')
        original_config.get_config_value(
                'section1', 'opt4',
                allow_blank=True, default='').AndReturn('blah')
        self.mox.ReplayAll()
        shadow_config = moblab_rpc_interface._get_shadow_config_from_partial_update(
                partial_config)
        # opt1 same as the original.
        self.assertFalse(shadow_config.has_option('section1', 'opt1'))
        # opt2 reverts back to original
        self.assertFalse(shadow_config.has_option('section1', 'opt2'))
        # opt3 is updated from original.
        self.assertEquals('value3', shadow_config.get('section1', 'opt3'))
        # opt3 in shadow but updated again.
        self.assertEquals('value4', shadow_config.get('section1', 'opt4'))
        self.mox.VerifyAll()


    def testGetShadowConfigFromPartialUpdateWithNewSection(self):
        """
        Test getting shadown configuration based on partial update with new section.
        """
        partial_config = {
                'section2': [
                    ('opt5', 'value5'),
                    ('opt6', 'value6'),
                    ],
                }
        shadow_config_str = "[section1]\nopt2 = value2_1\n"
        shadow_config = ConfigParser.ConfigParser()
        shadow_config.readfp(StringIO.StringIO(shadow_config_str))
        original_config = self.mox.CreateMockAnything()
        self.mox.StubOutWithMock(moblab_rpc_interface, '_read_original_config')
        self.mox.StubOutWithMock(moblab_rpc_interface, '_read_raw_config')
        moblab_rpc_interface._read_original_config().AndReturn(original_config)
        moblab_rpc_interface._read_raw_config(
            moblab_rpc_interface._CONFIG.shadow_file).AndReturn(shadow_config)
        original_config.get_config_value(
                'section2', 'opt5',
                allow_blank=True, default='').AndReturn('value5')
        original_config.get_config_value(
                'section2', 'opt6',
                allow_blank=True, default='').AndReturn('blah')
        self.mox.ReplayAll()
        shadow_config = moblab_rpc_interface._get_shadow_config_from_partial_update(
                partial_config)
        # opt2 is still in shadow
        self.assertEquals('value2_1', shadow_config.get('section1', 'opt2'))
        # opt5 is not changed.
        self.assertFalse(shadow_config.has_option('section2', 'opt5'))
        # opt6 is updated.
        self.assertEquals('value6', shadow_config.get('section2', 'opt6'))
        self.mox.VerifyAll()

    def testGetBuildsForInDirectory(self):
        config_mock = self.mox.CreateMockAnything()
        moblab_rpc_interface._CONFIG = config_mock
        config_mock.get_config_value(
            'CROS', 'image_storage_server').AndReturn('gs://bucket1/')
        self.mox.StubOutWithMock(common_lib_utils, 'run')
        output = self.mox.CreateMockAnything()
        self.mox.StubOutWithMock(StringIO, 'StringIO', use_mock_anything=True)
        StringIO.StringIO().AndReturn(output)
        output.getvalue().AndReturn(
        """gs://bucket1/dummy/R53-8480.0.0/\ngs://bucket1/dummy/R53-8530.72.0/\n
        gs://bucket1/dummy/R54-8712.0.0/\ngs://bucket1/dummy/R54-8717.0.0/\n
        gs://bucket1/dummy/R55-8759.0.0/\n
        gs://bucket1/dummy/R55-8760.0.0-b5849/\n
        gs://bucket1/dummy/R56-8995.0.0/\ngs://bucket1/dummy/R56-9001.0.0/\n
        gs://bucket1/dummy/R57-9202.66.0/\ngs://bucket1/dummy/R58-9331.0.0/\n
        gs://bucket1/dummy/R58-9334.15.0/\ngs://bucket1/dummy/R58-9334.17.0/\n
        gs://bucket1/dummy/R58-9334.18.0/\ngs://bucket1/dummy/R58-9334.19.0/\n
        gs://bucket1/dummy/R58-9334.22.0/\ngs://bucket1/dummy/R58-9334.28.0/\n
        gs://bucket1/dummy/R58-9334.3.0/\ngs://bucket1/dummy/R58-9334.30.0/\n
        gs://bucket1/dummy/R58-9334.36.0/\ngs://bucket1/dummy/R58-9334.55.0/\n
        gs://bucket1/dummy/R58-9334.6.0/\ngs://bucket1/dummy/R58-9334.7.0/\n
        gs://bucket1/dummy/R58-9334.9.0/\ngs://bucket1/dummy/R59-9346.0.0/\n
        gs://bucket1/dummy/R59-9372.0.0/\ngs://bucket1/dummy/R59-9387.0.0/\n
        gs://bucket1/dummy/R59-9436.0.0/\ngs://bucket1/dummy/R59-9452.0.0/\n
        gs://bucket1/dummy/R59-9453.0.0/\ngs://bucket1/dummy/R59-9455.0.0/\n
        gs://bucket1/dummy/R59-9460.0.0/\ngs://bucket1/dummy/R59-9460.11.0/\n
        gs://bucket1/dummy/R59-9460.16.0/\ngs://bucket1/dummy/R59-9460.25.0/\n
        gs://bucket1/dummy/R59-9460.8.0/\ngs://bucket1/dummy/R59-9460.9.0/\n
        gs://bucket1/dummy/R60-9472.0.0/\ngs://bucket1/dummy/R60-9491.0.0/\n
        gs://bucket1/dummy/R60-9492.0.0/\ngs://bucket1/dummy/R60-9497.0.0/\n
        gs://bucket1/dummy/R60-9500.0.0/""")

        output.close()

        self.mox.StubOutWithMock(moblab_rpc_interface.GsUtil, 'get_gsutil_cmd')
        moblab_rpc_interface.GsUtil.get_gsutil_cmd().AndReturn(
            '/path/to/gsutil')

        common_lib_utils.run('/path/to/gsutil',
                             args=('ls', 'gs://bucket1/dummy'),
                             stdout_tee=mox.IgnoreArg()).AndReturn(output)
        self.mox.ReplayAll()
        expected_results = ['dummy/R60-9500.0.0', 'dummy/R60-9497.0.0',
            'dummy/R60-9492.0.0', 'dummy/R60-9491.0.0', 'dummy/R60-9472.0.0',
            'dummy/R59-9460.25.0', 'dummy/R59-9460.16.0', 'dummy/R59-9460.11.0',
            'dummy/R59-9460.9.0', 'dummy/R59-9460.8.0', 'dummy/R58-9334.55.0',
            'dummy/R58-9334.36.0', 'dummy/R58-9334.30.0', 'dummy/R58-9334.28.0',
            'dummy/R58-9334.22.0']
        actual_results = moblab_rpc_interface._get_builds_for_in_directory(
            "dummy",3, 5)
        self.assertEquals(expected_results, actual_results)
        self.mox.VerifyAll()

    def testRunBucketPerformanceTestFail(self):
        self.mox.StubOutWithMock(moblab_rpc_interface.GsUtil, 'get_gsutil_cmd')
        moblab_rpc_interface.GsUtil.get_gsutil_cmd().AndReturn(
            '/path/to/gsutil')
        self.mox.StubOutWithMock(common_lib_utils, 'run')
        common_lib_utils.run('/path/to/gsutil',
                  args=(
                  '-o', 'Credentials:gs_access_key_id=key',
                  '-o', 'Credentials:gs_secret_access_key=secret',
                  'perfdiag', '-s', '1K',
                  '-o', 'testoutput',
                  '-n', '10',
                  'gs://bucket1')).AndRaise(
            error.CmdError("fakecommand", common_lib_utils.CmdResult(),
                           "xxxxxx<Error>yyyyyyyyyy</Error>"))

        self.mox.ReplayAll()
        self.assertRaisesRegexp(
            moblab_rpc_interface.BucketPerformanceTestException,
            '<Error>yyyyyyyyyy',
            moblab_rpc_interface._run_bucket_performance_test,
            'key', 'secret', 'gs://bucket1', '1K', '10', 'testoutput')
        self.mox.VerifyAll()

    def testEnableNotificationUsingCredentialsInBucketFail(self):
        config_mock = self.mox.CreateMockAnything()
        moblab_rpc_interface._CONFIG = config_mock
        config_mock.get_config_value(
            'CROS', 'image_storage_server').AndReturn('gs://bucket1/')

        self.mox.StubOutWithMock(moblab_rpc_interface.GsUtil, 'get_gsutil_cmd')
        moblab_rpc_interface.GsUtil.get_gsutil_cmd().AndReturn(
            '/path/to/gsutil')

        self.mox.StubOutWithMock(common_lib_utils, 'run')
        common_lib_utils.run('/path/to/gsutil',
            args=('cp', 'gs://bucket1/pubsub-key-do-not-delete.json',
            '/tmp')).AndRaise(
                error.CmdError("fakecommand", common_lib_utils.CmdResult(), ""))
        self.mox.ReplayAll()
        moblab_rpc_interface._enable_notification_using_credentials_in_bucket()

    def testEnableNotificationUsingCredentialsInBucketSuccess(self):
        config_mock = self.mox.CreateMockAnything()
        moblab_rpc_interface._CONFIG = config_mock
        config_mock.get_config_value(
            'CROS', 'image_storage_server').AndReturn('gs://bucket1/')

        self.mox.StubOutWithMock(moblab_rpc_interface.GsUtil, 'get_gsutil_cmd')
        moblab_rpc_interface.GsUtil.get_gsutil_cmd().AndReturn(
            '/path/to/gsutil')

        self.mox.StubOutWithMock(common_lib_utils, 'run')
        common_lib_utils.run('/path/to/gsutil',
            args=('cp', 'gs://bucket1/pubsub-key-do-not-delete.json',
            '/tmp'))
        moblab_rpc_interface.shutil = self.mox.CreateMockAnything()
        moblab_rpc_interface.shutil.copyfile(
                '/tmp/pubsub-key-do-not-delete.json',
                moblab_host.MOBLAB_SERVICE_ACCOUNT_LOCATION)
        self.mox.StubOutWithMock(moblab_rpc_interface, '_update_partial_config')
        moblab_rpc_interface._update_partial_config(
            {'CROS': [(moblab_rpc_interface._CLOUD_NOTIFICATION_ENABLED, True)]}
        )
        self.mox.ReplayAll()
        moblab_rpc_interface._enable_notification_using_credentials_in_bucket()

    def testInstallSystemUpdate(self):
        update_engine_client = moblab_rpc_interface._UPDATE_ENGINE_CLIENT

        self.mox.StubOutWithMock(moblab_rpc_interface.subprocess, 'check_call')
        moblab_rpc_interface.subprocess.check_call(['sudo',
                update_engine_client, '--update'])
        moblab_rpc_interface.subprocess.check_call(['sudo',
                update_engine_client, '--is_reboot_needed'])

        self.mox.StubOutWithMock(moblab_rpc_interface.subprocess, 'call')
        moblab_rpc_interface.subprocess.call(['sudo', update_engine_client,
                '--reboot'])

        self.mox.ReplayAll()
        moblab_rpc_interface._install_system_update()

    def testInstallSystemUpdateError(self):
        update_engine_client = moblab_rpc_interface._UPDATE_ENGINE_CLIENT

        error_message = ('ERROR_CODE=37\n'
            'ERROR_MESSAGE=ErrorCode::kOmahaErrorInHTTPResponse')

        self.mox.StubOutWithMock(moblab_rpc_interface.subprocess, 'check_call')
        moblab_rpc_interface.subprocess.check_call(['sudo',
                update_engine_client, '--update']).AndRaise(
                    moblab_rpc_interface.subprocess.CalledProcessError(1,
                        'sudo'))

        self.mox.StubOutWithMock(moblab_rpc_interface.subprocess,
                'check_output')
        moblab_rpc_interface.subprocess.check_output(['sudo',
                update_engine_client, '--last_attempt_error']).AndReturn(
                error_message)

        self.mox.ReplayAll()
        try:
            moblab_rpc_interface._install_system_update()
        except moblab_rpc_interface.error.RPCException as e:
            self.assertEquals(str(e), error_message)


    def testGetSystemUpdateStatus(self):
        update_engine_client = moblab_rpc_interface._UPDATE_ENGINE_CLIENT
        update_status = ('LAST_CHECKED_TIME=1516753795\n'
                         'PROGRESS=0.220121\n'
                         'CURRENT_OP=UPDATE_STATUS_DOWNLOADING\n'
                         'NEW_VERSION=10032.89.0\n'
                         'NEW_SIZE=782805733')

        self.mox.StubOutWithMock(moblab_rpc_interface.subprocess,
                'check_output')
        moblab_rpc_interface.subprocess.check_output(['sudo',
                update_engine_client, '--status']).AndReturn(
                        update_status)

        self.mox.ReplayAll()
        output = moblab_rpc_interface._get_system_update_status()

        self.assertEquals(output['PROGRESS'], '0.220121')
        self.assertEquals(output['CURRENT_OP'], 'UPDATE_STATUS_DOWNLOADING')
        self.assertEquals(output['NEW_VERSION'], '10032.89.0')
        self.assertEquals(output['NEW_SIZE'], '782805733')

    def testCheckForSystemUpdate(self):
        update_engine_client = moblab_rpc_interface._UPDATE_ENGINE_CLIENT

        self.mox.StubOutWithMock(moblab_rpc_interface.subprocess, 'call')
        moblab_rpc_interface.subprocess.call(['sudo', update_engine_client,
                '--check_for_update'])

        self.mox.StubOutWithMock(moblab_rpc_interface,
                '_get_system_update_status')
        for i in range(0,4):
            moblab_rpc_interface._get_system_update_status().AndReturn(
                    dict({'CURRENT_OP': 'UPDATE_STATUS_CHECKING_FOR_UPDATE'})
            )
        moblab_rpc_interface._get_system_update_status().AndReturn(
                dict({'CURRENT_OP': 'UPDATE_STATUS_DOWNLOADING'})
        )
        self.mox.ReplayAll()
        moblab_rpc_interface._check_for_system_update()

    def testGetConnectedDutBoardModels(self):
        # setting up mocks for 2 duts with different boards and models
        mock_minnie_labels = [
            self.mox.CreateMockAnything(),
            self.mox.CreateMockAnything(),
        ]
        mock_minnie_labels[0].name = 'board:veyron_minnie'
        mock_minnie_labels[1].name = 'model:veyron_minnie'
        mock_minnie = self.mox.CreateMockAnything()
        mock_minnie.label_list = mock_minnie_labels

        mock_bruce_labels = [
            self.mox.CreateMockAnything(),
            self.mox.CreateMockAnything()
        ]
        mock_bruce_labels[0].name = 'board:carl'
        mock_bruce_labels[1].name = 'model:bruce'
        mock_bruce = self.mox.CreateMockAnything()
        mock_bruce.label_list = mock_bruce_labels
        hosts = [mock_minnie, mock_bruce]

        # stub out the host query calls
        self.mox.StubOutWithMock(moblab_rpc_interface.rpc_utils,
                'get_host_query')
        moblab_rpc_interface.rpc_utils.get_host_query(
                (), False, True, {}).AndReturn(hosts)

        self.mox.StubOutWithMock(moblab_rpc_interface.models.Host.objects,
                'populate_relationships'),
        moblab_rpc_interface.models.Host.objects.populate_relationships(hosts,
                moblab_rpc_interface.models.Label, 'label_list')

        expected = [{
            'model': 'bruce',
            'board': 'carl'
        },
        {
            'model': 'veyron_minnie',
            'board': 'veyron_minnie'
        }]

        self.mox.ReplayAll()
        output = moblab_rpc_interface._get_connected_dut_board_models()
        self.assertEquals(output, expected)
        # test sorting
        self.assertEquals(output[0]['model'], 'bruce')

    def testAllDutConnections(self):
        leases = {
            '192.168.0.20': '3c:52:82:5f:15:20',
            '192.168.0.30': '3c:52:82:5f:15:21'
        }

        # stub out all of the multiprocessing
        mock_value = self.mox.CreateMockAnything()
        mock_value.value = True
        mock_process = self.mox.CreateMockAnything()

        for key in leases:
            mock_process.start()
        for key in leases:
            mock_process.join()

        self.mox.StubOutWithMock(
                moblab_rpc_interface, 'multiprocessing')

        for key in leases:
            moblab_rpc_interface.multiprocessing.Value(
                    mox.IgnoreArg()).AndReturn(mock_value)
            moblab_rpc_interface.multiprocessing.Process(
                    target=mox.IgnoreArg(), args=mox.IgnoreArg()).AndReturn(
                        mock_process)

        self.mox.ReplayAll()

        expected = {
            '192.168.0.20': {
                'mac_address': '3c:52:82:5f:15:20',
                'ssh_connection_ok': True
            },
            '192.168.0.30': {
                'mac_address': '3c:52:82:5f:15:21',
                'ssh_connection_ok': True
            }
        }

        connected_duts = moblab_rpc_interface._test_all_dut_connections(leases)
        self.assertDictEqual(expected, connected_duts)

    def testAllDutConnectionsFailure(self):
        leases = {
            '192.168.0.20': '3c:52:82:5f:15:20',
            '192.168.0.30': '3c:52:82:5f:15:21'
        }

        # stub out all of the multiprocessing
        mock_value = self.mox.CreateMockAnything()
        mock_value.value = False
        mock_process = self.mox.CreateMockAnything()

        for key in leases:
            mock_process.start()
        for key in leases:
            mock_process.join()

        self.mox.StubOutWithMock(
                moblab_rpc_interface, 'multiprocessing')

        for key in leases:
            moblab_rpc_interface.multiprocessing.Value(
                    mox.IgnoreArg()).AndReturn(mock_value)
            moblab_rpc_interface.multiprocessing.Process(
                    target=mox.IgnoreArg(), args=mox.IgnoreArg()).AndReturn(
                        mock_process)

        self.mox.ReplayAll()

        expected = {
            '192.168.0.20': {
                'mac_address': '3c:52:82:5f:15:20',
                'ssh_connection_ok': False
            },
            '192.168.0.30': {
                'mac_address': '3c:52:82:5f:15:21',
                'ssh_connection_ok': False
            }
        }

        connected_duts = moblab_rpc_interface._test_all_dut_connections(leases)
        self.assertDictEqual(expected, connected_duts)

    def testDutSshConnection(self):
        good_ip = '192.168.0.20'
        bad_ip = '192.168.0.30'
        cmd = ('ssh -o ConnectTimeout=2 -o StrictHostKeyChecking=no '
                "root@%s 'timeout 2 cat /etc/lsb-release'")

        self.mox.StubOutWithMock(moblab_rpc_interface.subprocess,
                'check_output')
        moblab_rpc_interface.subprocess.check_output(
                cmd % good_ip, shell=True).AndReturn('CHROMEOS_RELEASE_APPID')

        moblab_rpc_interface.subprocess.check_output(
                cmd % bad_ip, shell=True).AndRaise(
                moblab_rpc_interface.subprocess.CalledProcessError(1, cmd))

        self.mox.ReplayAll()
        self.assertEquals(
            moblab_rpc_interface._test_dut_ssh_connection(good_ip), True)
        self.assertEquals(
            moblab_rpc_interface._test_dut_ssh_connection(bad_ip), False)


if __name__ == '__main__':
    unittest.main()
