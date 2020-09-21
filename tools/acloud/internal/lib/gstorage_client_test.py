"""Tests for acloud.internal.lib.gstorage_client."""

import io
import time

import apiclient
import mock

import unittest
from acloud.internal.lib import driver_test_lib
from acloud.internal.lib import gstorage_client
from acloud.public import errors


class StorageClientTest(driver_test_lib.BaseDriverTest):
    """Test StorageClient."""

    LOCAL_SRC = "/fake/local/path"
    BUCKET = "fake_bucket"
    OBJECT = "fake_obj"
    MIME_TYPE = "fake_mimetype"

    def setUp(self):
        """Set up test."""
        super(StorageClientTest, self).setUp()
        self.Patch(gstorage_client.StorageClient, "InitResourceHandle")
        self.client = gstorage_client.StorageClient(mock.MagicMock())
        self.client._service = mock.MagicMock()

    def testGet(self):
        """Test Get."""
        mock_api = mock.MagicMock()
        resource_mock = mock.MagicMock()
        self.client._service.objects = mock.MagicMock(
            return_value=resource_mock)
        resource_mock.get = mock.MagicMock(return_value=mock_api)
        self.client.Get(self.BUCKET, self.OBJECT)
        resource_mock.get.assert_called_with(
            bucket=self.BUCKET, object=self.OBJECT)
        self.assertTrue(mock_api.execute.called)

    def testList(self):
        """Test List."""
        mock_items = ["fake/return"]
        self.Patch(
            gstorage_client.StorageClient,
            "ListWithMultiPages",
            return_value=mock_items)
        resource_mock = mock.MagicMock()
        self.client._service.objects = mock.MagicMock(
            return_value=resource_mock)
        items = self.client.List(self.BUCKET, self.OBJECT)
        self.client.ListWithMultiPages.assert_called_once_with(
            api_resource=resource_mock.list,
            bucket=self.BUCKET,
            prefix=self.OBJECT)
        self.assertEqual(mock_items, items)

    def testUpload(self):
        """Test Upload."""
        # Create mocks
        mock_file = mock.MagicMock()
        mock_file_io = mock.MagicMock()
        mock_file_io.__enter__.return_value = mock_file
        mock_media = mock.MagicMock()
        mock_api = mock.MagicMock()
        mock_response = mock.MagicMock()

        self.Patch(io, "FileIO", return_value=mock_file_io)
        self.Patch(
            apiclient.http, "MediaIoBaseUpload", return_value=mock_media)
        resource_mock = mock.MagicMock()
        self.client._service.objects = mock.MagicMock(
            return_value=resource_mock)
        resource_mock.insert = mock.MagicMock(return_value=mock_api)
        mock_api.execute = mock.MagicMock(return_value=mock_response)

        # Make the call to the api
        response = self.client.Upload(self.LOCAL_SRC, self.BUCKET, self.OBJECT,
                                      self.MIME_TYPE)

        # Verify
        self.assertEqual(response, mock_response)
        io.FileIO.assert_called_with(self.LOCAL_SRC, mode="rb")
        apiclient.http.MediaIoBaseUpload.assert_called_with(mock_file,
                                                            self.MIME_TYPE)
        resource_mock.insert.assert_called_with(
            bucket=self.BUCKET, name=self.OBJECT, media_body=mock_media)

    def testUploadOSError(self):
        """Test Upload when OSError is raised."""
        self.Patch(io, "FileIO", side_effect=OSError("fake OSError"))
        self.assertRaises(errors.DriverError, self.client.Upload,
                          self.LOCAL_SRC, self.BUCKET, self.OBJECT,
                          self.MIME_TYPE)

    def testDelete(self):
        """Test Delete."""
        mock_api = mock.MagicMock()
        resource_mock = mock.MagicMock()
        self.client._service.objects = mock.MagicMock(
            return_value=resource_mock)
        resource_mock.delete = mock.MagicMock(return_value=mock_api)
        self.client.Delete(self.BUCKET, self.OBJECT)
        resource_mock.delete.assert_called_with(
            bucket=self.BUCKET, object=self.OBJECT)
        self.assertTrue(mock_api.execute.called)

    def testDeleteMultipleFiles(self):
        """Test Delete multiple files."""
        fake_objs = ["fake_obj1", "fake_obj2"]
        mock_api = mock.MagicMock()
        resource_mock = mock.MagicMock()
        self.client._service.objects = mock.MagicMock(
            return_value=resource_mock)
        resource_mock.delete = mock.MagicMock(return_value=mock_api)
        deleted, failed, error_msgs = self.client.DeleteFiles(self.BUCKET,
                                                              fake_objs)
        self.assertEqual(deleted, fake_objs)
        self.assertEqual(failed, [])
        self.assertEqual(error_msgs, [])
        calls = [mock.call(
            bucket=self.BUCKET, object="fake_obj1"), mock.call(
                bucket=self.BUCKET, object="fake_obj2")]
        resource_mock.delete.assert_has_calls(calls)
        self.assertEqual(mock_api.execute.call_count, 2)

    def testGetUrl(self):
        """Test GetUrl."""
        fake_item = {"name": "fake-item-1", "selfLink": "link1"}
        self.Patch(
            gstorage_client.StorageClient, "Get", return_value=fake_item)
        self.assertEqual(
            self.client.GetUrl("fake_bucket", "fake-item-1"), "link1")

    def testGetUrlNotFound(self):
        """Test GetUrl when object is not found."""
        self.Patch(
            gstorage_client.StorageClient,
            "Get",
            side_effect=errors.ResourceNotFoundError(404, "expected error"))
        self.Patch(time, "sleep")
        self.assertRaises(errors.ResourceNotFoundError, self.client.GetUrl,
                          "fake_bucket", "fake-item-1")


if __name__ == "__main__":
    unittest.main()
