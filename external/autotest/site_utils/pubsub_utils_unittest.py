#!/usr/bin/env python2
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit test for pubsub_utils.py"""

from __future__ import print_function
import os
import unittest

import mox

from apiclient import discovery
from oauth2client.client import ApplicationDefaultCredentialsError
from oauth2client.client import GoogleCredentials
from googleapiclient.errors import UnknownApiNameOrVersion

import pubsub_utils

_TEST_CLOUD_SERVICE_ACCOUNT_FILE = '/tmp/test-credential'


class MockedPubSub(object):
    """A mocked PubSub handle."""
    def __init__(self, test, topic, msg, retry, ret_val=None,
                 raise_except=False):
        self.test = test
        self.topic = topic
        self.msg = msg
        self.retry = retry
        self.ret_val = ret_val
        self.raise_except = raise_except

    def projects(self):
        """Mocked PubSub projects."""
        return self

    def topics(self):
        """Mocked PubSub topics."""
        return self

    def publish(self, topic, body):
        """Mocked PubSub publish method.

        @param topic: PubSub topic string.
        @param body: PubSub notification body.
        """
        self.test.assertEquals(self.topic, topic)
        self.test.assertEquals(self.msg, body['messages'][0])
        return self

    def execute(self, num_retries):
        """Mocked PubSub execute method.

        @param num_retries: Number of retries.
        """
        self.test.assertEquals(self.retry, num_retries)
        if self.raise_except:
            raise Exception()
        return self.ret_val


def _create_sample_message():
    """Creates a sample pubsub message."""
    msg_payload = {'data': 'sample data'}
    msg_attributes = {}
    msg_attributes['var'] = 'value'
    msg_payload['attributes'] = msg_attributes

    return msg_payload


class PubSubTests(mox.MoxTestBase):
    """Tests for pubsub related functios."""

    def test_pubsub_with_no_service_account(self):
        """Test getting the pubsub service"""
        self.mox.StubOutWithMock(os.path, 'isfile')
        self.mox.ReplayAll()
        with self.assertRaises(pubsub_utils.PubSubException):
            pubsub_utils.PubSubClient()
        self.mox.VerifyAll()

    def test_pubsub_with_non_existing_service_account(self):
        """Test getting the pubsub service"""
        self.mox.StubOutWithMock(os.path, 'isfile')
        os.path.isfile(_TEST_CLOUD_SERVICE_ACCOUNT_FILE).AndReturn(False)
        self.mox.ReplayAll()
        with self.assertRaises(pubsub_utils.PubSubException):
            pubsub_utils.PubSubClient(_TEST_CLOUD_SERVICE_ACCOUNT_FILE)
        self.mox.VerifyAll()

    def test_pubsub_with_corrupted_service_account(self):
        """Test pubsub with corrupted service account."""
        self.mox.StubOutWithMock(os.path, 'isfile')
        self.mox.StubOutWithMock(GoogleCredentials, 'from_stream')
        os.path.isfile(_TEST_CLOUD_SERVICE_ACCOUNT_FILE).AndReturn(True)
        GoogleCredentials.from_stream(
            _TEST_CLOUD_SERVICE_ACCOUNT_FILE).AndRaise(
                ApplicationDefaultCredentialsError())
        self.mox.ReplayAll()
        with self.assertRaises(pubsub_utils.PubSubException):
            pubsub_utils.PubSubClient(_TEST_CLOUD_SERVICE_ACCOUNT_FILE)
        self.mox.VerifyAll()

    def test_pubsub_with_invalid_service_account(self):
        """Test pubsubwith invalid service account."""
        self.mox.StubOutWithMock(os.path, 'isfile')
        self.mox.StubOutWithMock(GoogleCredentials, 'from_stream')
        os.path.isfile(_TEST_CLOUD_SERVICE_ACCOUNT_FILE).AndReturn(True)
        credentials = self.mox.CreateMock(GoogleCredentials)
        GoogleCredentials.from_stream(
            _TEST_CLOUD_SERVICE_ACCOUNT_FILE).AndReturn(credentials)
        credentials.create_scoped_required().AndReturn(True)
        credentials.create_scoped(pubsub_utils.PUBSUB_SCOPES).AndReturn(
            credentials)
        self.mox.StubOutWithMock(discovery, 'build')
        discovery.build(
            pubsub_utils.PUBSUB_SERVICE_NAME,
            pubsub_utils.PUBSUB_VERSION,
            credentials=credentials).AndRaise(UnknownApiNameOrVersion())
        self.mox.ReplayAll()
        with self.assertRaises(pubsub_utils.PubSubException):
            msg = _create_sample_message()
            pubsub_client = pubsub_utils.PubSubClient(
                _TEST_CLOUD_SERVICE_ACCOUNT_FILE)
            pubsub_client.publish_notifications('test_topic', [msg])
        self.mox.VerifyAll()

    def test_publish_notifications(self):
        """Test getting the pubsub service"""
        self.mox.StubOutWithMock(os.path, 'isfile')
        self.mox.StubOutWithMock(GoogleCredentials, 'from_stream')
        os.path.isfile(_TEST_CLOUD_SERVICE_ACCOUNT_FILE).AndReturn(True)
        credentials = self.mox.CreateMock(GoogleCredentials)
        GoogleCredentials.from_stream(
            _TEST_CLOUD_SERVICE_ACCOUNT_FILE).AndReturn(credentials)
        credentials.create_scoped_required().AndReturn(True)
        credentials.create_scoped(pubsub_utils.PUBSUB_SCOPES).AndReturn(
            credentials)
        self.mox.StubOutWithMock(discovery, 'build')
        msg = _create_sample_message()
        discovery.build(
            pubsub_utils.PUBSUB_SERVICE_NAME,
            pubsub_utils.PUBSUB_VERSION,
            credentials=credentials).AndReturn(MockedPubSub(
                self,
                'test_topic',
                msg,
                pubsub_utils.DEFAULT_PUBSUB_NUM_RETRIES,
                # use tuple ('123') instead of list just for easy to
                # write the test.
                ret_val={'messageIds': ('123')}))

        self.mox.ReplayAll()
        pubsub_client = pubsub_utils.PubSubClient(
                _TEST_CLOUD_SERVICE_ACCOUNT_FILE)
        msg_ids = pubsub_client.publish_notifications('test_topic', [msg])
        self.assertEquals(('123'), msg_ids)

        self.mox.VerifyAll()


if __name__ == '__main__':
    unittest.main()
