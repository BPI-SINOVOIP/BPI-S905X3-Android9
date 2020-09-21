#!/usr/bin/env python
#
# Copyright 2016 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Tests for acloud.internal.lib.base_cloud_client."""

import time
import apiclient
import mock

import unittest
from acloud.internal.lib import base_cloud_client
from acloud.internal.lib import driver_test_lib
from acloud.public import errors


class FakeError(Exception):
    """Fake Error for testing retries."""


class BaseCloudApiClientTest(driver_test_lib.BaseDriverTest):
    """Test BaseCloudApiClient."""

    def setUp(self):
        """Set up test."""
        super(BaseCloudApiClientTest, self).setUp()

    def testInitResourceHandle(self):
        """Test InitResourceHandle."""
        # Setup mocks
        mock_credentials = mock.MagicMock()
        self.Patch(base_cloud_client, "build")
        # Call the method
        base_cloud_client.BaseCloudApiClient(mock.MagicMock())
        base_cloud_client.build.assert_called_once_with(
            serviceName=base_cloud_client.BaseCloudApiClient.API_NAME,
            version=base_cloud_client.BaseCloudApiClient.API_VERSION,
            http=mock.ANY)

    def _SetupInitMocks(self):
        """Setup mocks required to initialize a base cloud client.

    Returns:
      A base_cloud_client.BaseCloudApiClient mock.
    """
        self.Patch(
            base_cloud_client.BaseCloudApiClient,
            "InitResourceHandle",
            return_value=mock.MagicMock())
        return base_cloud_client.BaseCloudApiClient(mock.MagicMock())

    def _SetupBatchHttpRequestMock(self, rid_to_responses, rid_to_exceptions):
        """Setup BatchHttpRequest mock."""

        rid_to_exceptions = rid_to_exceptions or {}
        rid_to_responses = rid_to_responses or {}

        def _CreatMockBatchHttpRequest():
            """Create a mock BatchHttpRequest object."""
            requests = {}

            def _Add(request, callback, request_id):
                requests[request_id] = (request, callback)

            def _Execute():
                for rid in requests:
                    requests[rid][0].execute()
                    _, callback = requests[rid]
                    callback(
                        request_id=rid,
                        response=rid_to_responses.get(rid),
                        exception=rid_to_exceptions.get(rid))

            mock_batch = mock.MagicMock()
            mock_batch.add = _Add
            mock_batch.execute = _Execute
            return mock_batch

        self.Patch(
            apiclient.http,
            "BatchHttpRequest",
            side_effect=_CreatMockBatchHttpRequest)

    def testBatchExecute(self):
        """Test BatchExecute."""
        self.Patch(time, "sleep")
        client = self._SetupInitMocks()
        requests = {"r1": mock.MagicMock(),
                    "r2": mock.MagicMock(),
                    "r3": mock.MagicMock()}
        response = {"name": "fake_response"}
        error_1 = errors.HttpError(503, "fake retriable error.")
        error_2 = FakeError("fake retriable error.")
        responses = {"r1": response, "r2": None, "r3": None}
        exceptions = {"r1": None, "r2": error_1, "r3": error_2}
        self._SetupBatchHttpRequestMock(responses, exceptions)
        results = client.BatchExecute(
            requests, other_retriable_errors=(FakeError, ))
        expected_results = {
            "r1": (response, None),
            "r2": (None, error_1),
            "r3": (None, error_2)
        }
        self.assertEqual(results, expected_results)
        self.assertEqual(requests["r1"].execute.call_count, 1)
        self.assertEqual(requests["r2"].execute.call_count,
                         client.RETRY_COUNT + 1)
        self.assertEqual(requests["r3"].execute.call_count,
                         client.RETRY_COUNT + 1)

    def testListWithMultiPages(self):
        """Test ListWithMultiPages."""
        fake_token = "fake_next_page_token"
        item_1 = "item_1"
        item_2 = "item_2"
        response_1 = {"items": [item_1], "nextPageToken": fake_token}
        response_2 = {"items": [item_2]}

        api_mock = mock.MagicMock()
        api_mock.execute.side_effect = [response_1, response_2]
        resource_mock = mock.MagicMock(return_value=api_mock)
        client = self._SetupInitMocks()
        items = client.ListWithMultiPages(
            api_resource=resource_mock, fake_arg="fake_arg")
        self.assertEqual(items, [item_1, item_2])

    def testExecuteWithRetry(self):
        """Test Execute is called and retries are triggered."""
        self.Patch(time, "sleep")
        client = self._SetupInitMocks()
        api_mock = mock.MagicMock()
        error = errors.HttpError(503, "fake retriable error.")
        api_mock.execute.side_effect = error
        self.assertRaises(errors.HttpError, client.Execute, api_mock)

        api_mock = mock.MagicMock()
        api_mock.execute.side_effect = FakeError("fake retriable error.")
        self.assertRaises(
            FakeError,
            client.Execute,
            api_mock,
            other_retriable_errors=(FakeError, ))
        self.assertEqual(api_mock.execute.call_count, client.RETRY_COUNT + 1)


if __name__ == "__main__":
    unittest.main()
