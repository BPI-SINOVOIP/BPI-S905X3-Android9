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

"""Tests for acloud.internal.lib.android_build_client."""

import io
import time

import apiclient
import mock

import unittest
from acloud.internal.lib import android_build_client
from acloud.internal.lib import driver_test_lib
from acloud.public import errors


class AndroidBuildClientTest(driver_test_lib.BaseDriverTest):
    """Test AndroidBuildClient."""

    BUILD_TARGET = "fake_target"
    BUILD_ID = 12345
    RESOURCE_ID = "avd-system.tar.gz"
    LOCAL_DEST = "/fake/local/path"
    DESTINATION_BUCKET = "fake_bucket"

    def setUp(self):
        """Set up test."""
        super(AndroidBuildClientTest, self).setUp()
        self.Patch(android_build_client.AndroidBuildClient,
                   "InitResourceHandle")
        self.client = android_build_client.AndroidBuildClient(mock.MagicMock())
        self.client._service = mock.MagicMock()

    def testDownloadArtifact(self):
        """Test DownloadArtifact."""
        # Create mocks.
        mock_file = mock.MagicMock()
        mock_file_io = mock.MagicMock()
        mock_file_io.__enter__.return_value = mock_file
        mock_downloader = mock.MagicMock()
        mock_downloader.next_chunk = mock.MagicMock(
            side_effect=[(mock.MagicMock(), False), (mock.MagicMock(), True)])
        mock_api = mock.MagicMock()
        self.Patch(io, "FileIO", return_value=mock_file_io)
        self.Patch(
            apiclient.http,
            "MediaIoBaseDownload",
            return_value=mock_downloader)
        mock_resource = mock.MagicMock()
        self.client._service.buildartifact = mock.MagicMock(
            return_value=mock_resource)
        mock_resource.get_media = mock.MagicMock(return_value=mock_api)
        # Make the call to the api
        self.client.DownloadArtifact(self.BUILD_TARGET, self.BUILD_ID,
                                     self.RESOURCE_ID, self.LOCAL_DEST)
        # Verify
        mock_resource.get_media.assert_called_with(
            buildId=self.BUILD_ID,
            target=self.BUILD_TARGET,
            attemptId="0",
            resourceId=self.RESOURCE_ID)
        io.FileIO.assert_called_with(self.LOCAL_DEST, mode="wb")
        mock_call = mock.call(
            mock_file,
            mock_api,
            chunksize=android_build_client.AndroidBuildClient.
            DEFAULT_CHUNK_SIZE)
        apiclient.http.MediaIoBaseDownload.assert_has_calls([mock_call])
        self.assertEqual(mock_downloader.next_chunk.call_count, 2)

    def testDownloadArtifactOSError(self):
        """Test DownloadArtifact when OSError is raised."""
        self.Patch(io, "FileIO", side_effect=OSError("fake OSError"))
        self.assertRaises(errors.DriverError, self.client.DownloadArtifact,
                          self.BUILD_TARGET, self.BUILD_ID, self.RESOURCE_ID,
                          self.LOCAL_DEST)

    def testCopyTo(self):
        """Test CopyTo."""
        mock_resource = mock.MagicMock()
        self.client._service.buildartifact = mock.MagicMock(
            return_value=mock_resource)
        self.client.CopyTo(
            build_target=self.BUILD_TARGET,
            build_id=self.BUILD_ID,
            artifact_name=self.RESOURCE_ID,
            destination_bucket=self.DESTINATION_BUCKET,
            destination_path=self.RESOURCE_ID)
        mock_resource.copyTo.assert_called_once_with(
            buildId=self.BUILD_ID,
            target=self.BUILD_TARGET,
            attemptId=self.client.DEFAULT_ATTEMPT_ID,
            artifactName=self.RESOURCE_ID,
            destinationBucket=self.DESTINATION_BUCKET,
            destinationPath=self.RESOURCE_ID)

    def testCopyToWithRetry(self):
        """Test CopyTo with retry."""
        self.Patch(time, "sleep")
        mock_resource = mock.MagicMock()
        mock_api_request = mock.MagicMock()
        mock_resource.copyTo.return_value = mock_api_request
        self.client._service.buildartifact.return_value = mock_resource
        mock_api_request.execute.side_effect = errors.HttpError(503,
                                                                "fake error")
        self.assertRaises(
            errors.HttpError,
            self.client.CopyTo,
            build_id=self.BUILD_ID,
            build_target=self.BUILD_TARGET,
            artifact_name=self.RESOURCE_ID,
            destination_bucket=self.DESTINATION_BUCKET,
            destination_path=self.RESOURCE_ID)
        self.assertEqual(mock_api_request.execute.call_count, 6)


if __name__ == "__main__":
    unittest.main()
