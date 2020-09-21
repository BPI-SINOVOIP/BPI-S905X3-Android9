#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import mox
import unittest

import common

from autotest_lib.client.common_lib import utils
from autotest_lib.site_utils import cloud_console_client
from autotest_lib.site_utils import cloud_console_pb2 as cpcon
from autotest_lib.site_utils import pubsub_utils

class PubSubBasedClientTests(mox.MoxTestBase):
    """Tests for the 'PubSubBasedClient'."""

    def setUp(self):
        super(PubSubBasedClientTests, self).setUp()
        self._pubsub_client_mock = self.mox.CreateMock(
                pubsub_utils.PubSubClient)
        self._stubs = mox.stubout.StubOutForTesting()
        self._stubs.Set(cloud_console_client, '_create_pubsub_client',
                lambda x: self._pubsub_client_mock)

    def tearDown(self):
        self._stubs.UnsetAll()
        super(PubSubBasedClientTests, self).tearDown()

    def test_create_test_result_notification(self):
        """Tests the test result notification message."""
        self._console_client = cloud_console_client.PubSubBasedClient()
        self.mox.StubOutWithMock(utils,
                                 'get_moblab_serial_number')
        utils.get_moblab_serial_number().AndReturn(
            'PV120BB8JAC01E')
        self.mox.StubOutWithMock(utils, 'get_moblab_id')
        utils.get_moblab_id().AndReturn(
            'c8386d92-9ad1-11e6-80f5-111111111111')
        self.mox.ReplayAll()
        console_client = cloud_console_client.PubSubBasedClient()
        msg = console_client._create_test_job_offloaded_message(
                'gs://test_bucket/123-moblab')
        self.assertEquals(base64.b64encode(
            cloud_console_client.LEGACY_TEST_OFFLOAD_MESSAGE), msg['data'])
        self.assertEquals(
            cloud_console_client.CURRENT_MESSAGE_VERSION,
            msg['attributes'][cloud_console_client.LEGACY_ATTR_VERSION])
        self.assertEquals(
            '1c:dc:d1:11:01:e1',
            msg['attributes'][cloud_console_client.LEGACY_ATTR_MOBLAB_MAC]
            )
        self.assertEquals(
            'c8386d92-9ad1-11e6-80f5-111111111111',
            msg['attributes'][cloud_console_client.LEGACY_ATTR_MOBLAB_ID])
        self.assertEquals(
            'gs://test_bucket/123-moblab',
            msg['attributes'][cloud_console_client.LEGACY_ATTR_GCS_URI])
        self.mox.VerifyAll()

    def test_send_test_job_offloaded_message(self):
        """Tests send job offloaded notification."""
        console_client = cloud_console_client.PubSubBasedClient(
                pubsub_topic='test topic')

        message = {'data': 'dummy data', 'attributes' : {'key' : 'value'}}
        self.mox.StubOutWithMock(cloud_console_client.PubSubBasedClient,
                '_create_test_job_offloaded_message')
        console_client._create_test_job_offloaded_message(
                'gs://test_bucket/123-moblab').AndReturn(message)

        msg_ids = ['1']
        self._pubsub_client_mock.publish_notifications(
                'test topic', [message]).AndReturn(msg_ids)

        self.mox.ReplayAll()
        result = console_client.send_test_job_offloaded_message(
                'gs://test_bucket/123-moblab')
        self.assertTrue(result)
        self.mox.VerifyAll()

    def test_send_heartbeat(self):
        """Tests send heartbeat."""
        console_client = cloud_console_client.PubSubBasedClient(
                pubsub_topic='test topic')
        self.mox.StubOutWithMock(utils, 'get_moblab_id')
        utils.get_moblab_id().AndReturn(
            'c8386d92-9ad1-11e6-80f5-111111111111')

        message = {
                'attributes' : {
                    'ATTR_MOBLAB_ID': 'c8386d92-9ad1-11e6-80f5-111111111111',
                    'ATTR_MESSAGE_VERSION': '1',
                    'ATTR_MESSAGE_TYPE': 'MSG_MOBLAB_HEARTBEAT',
                    'ATTR_MOBLAB_MAC_ADDRESS': '8c:dc:d4:56:06:e7'}}
        msg_ids = ['1']
        self._pubsub_client_mock.publish_notifications(
                'test topic', [message]).AndReturn(msg_ids)
        self.mox.ReplayAll()
        console_client.send_heartbeat()
        self.mox.VerifyAll()

    def test_send_event(self):
        """Tests send heartbeat."""
        console_client = cloud_console_client.PubSubBasedClient(
                pubsub_topic='test topic')
        self.mox.StubOutWithMock(utils, 'get_moblab_id')
        utils.get_moblab_id().AndReturn(
            'c8386d92-9ad1-11e6-80f5-111111111111')

        message = {
                'data': '\x08\x01\x12\x0ethis is a test',
                'attributes' : {
                    'ATTR_MOBLAB_ID': 'c8386d92-9ad1-11e6-80f5-111111111111',
                    'ATTR_MESSAGE_VERSION': '1',
                    'ATTR_MESSAGE_TYPE': 'MSG_MOBLAB_REMOTE_EVENT',
                    'ATTR_MOBLAB_MAC_ADDRESS': '8c:dc:d4:56:06:e7'}}
        msg_ids = ['1']
        self._pubsub_client_mock.publish_notifications(
                'test topic', [message]).AndReturn(msg_ids)
        self.mox.ReplayAll()
        console_client.send_event(
                cpcon.RemoteEventMessage.EVENT_MOBLAB_BOOT_COMPLETE,
                'this is a test')
        self.mox.VerifyAll()

if __name__ == '__main__':
    unittest.main()
