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

"""A client that talks to Google Cloud Storage APIs."""

import io
import logging
import os

import apiclient

from acloud.internal.lib import base_cloud_client
from acloud.internal.lib import utils
from acloud.public import errors

logger = logging.getLogger(__name__)


class StorageClient(base_cloud_client.BaseCloudApiClient):
    """Client that talks to Google Cloud Storages."""

    # API settings, used by BaseCloudApiClient.
    API_NAME = "storage"
    API_VERSION = "v1"
    SCOPE = "https://www.googleapis.com/auth/devstorage.read_write"
    GET_OBJ_MAX_RETRY = 3
    GET_OBJ_RETRY_SLEEP = 5

    # Other class variables.
    OBJECT_URL_FMT = "https://storage.googleapis.com/%s/%s"

    def Get(self, bucket_name, object_name):
        """Get object in a bucket.

        Args:
            bucket_name: String, google cloud storage bucket name.
            object_name: String, full path to the object within the bucket.

        Returns:
            A dictronary representing an object resource.
        """
        request = self.service.objects().get(bucket=bucket_name,
                                             object=object_name)
        return self.Execute(request)

    def List(self, bucket_name, prefix=None):
        """Lists objects in a bucket.

        Args:
            bucket_name: String, google cloud storage bucket name.
            prefix: String, Filter results to objects whose names begin with
                    this prefix.

        Returns:
            A list of google storage objects whose names match the prefix.
            Each element is dictionary that wraps all the information about an object.
        """
        logger.debug("Listing storage bucket: %s, prefix: %s", bucket_name,
                     prefix)
        items = self.ListWithMultiPages(
            api_resource=self.service.objects().list,
            bucket=bucket_name,
            prefix=prefix)
        return items

    def Upload(self, local_src, bucket_name, object_name, mime_type):
        """Uploads a file.

        Args:
            local_src: string, a local path to a file to be uploaded.
            bucket_name: string, google cloud storage bucket name.
            object_name: string, the name of the remote file in storage.
            mime_type: string, mime-type of the file.

        Returns:
            URL to the inserted artifact in storage.
        """
        logger.info("Uploading file: src: %s, bucket: %s, object: %s",
                    local_src, bucket_name, object_name)
        try:
            with io.FileIO(local_src, mode="rb") as fh:
                media = apiclient.http.MediaIoBaseUpload(fh, mime_type)
                request = self.service.objects().insert(bucket=bucket_name,
                                                        name=object_name,
                                                        media_body=media)
                response = self.Execute(request)
            logger.info("Uploaded artifact: %s", response["selfLink"])
            return response
        except OSError as e:
            logger.error("Uploading artifact fails: %s", str(e))
            raise errors.DriverError(str(e))

    def Delete(self, bucket_name, object_name):
        """Deletes a file.

        Args:
            bucket_name: string, google cloud storage bucket name.
            object_name: string, the name of the remote file in storage.
        """
        logger.info("Deleting file: bucket: %s, object: %s", bucket_name,
                    object_name)
        request = self.service.objects().delete(bucket=bucket_name,
                                                object=object_name)
        self.Execute(request)
        logger.info("Deleted file: bucket: %s, object: %s", bucket_name,
                    object_name)

    def DeleteFiles(self, bucket_name, object_names):
        """Deletes multiple files.

        Args:
            bucket_name: string, google cloud storage bucket name.
            object_names: A list of strings, each of which is a name of a remote file.

        Returns:
            A tuple, (deleted, failed, error_msgs)
            deleted: A list of names of objects that have been deleted.
            faild: A list of names of objects that we fail to delete.
            error_msgs: A list of failure messages.
        """
        deleted = []
        failed = []
        error_msgs = []
        for object_name in object_names:
            try:
                self.Delete(bucket_name, object_name)
                deleted.append(object_name)
            except errors.DriverError as e:
                failed.append(object_name)
                error_msgs.append(str(e))
        return deleted, failed, error_msgs

    def GetUrl(self, bucket_name, object_name):
        """Get information about a file object.

        Args:
            bucket_name: string, google cloud storage bucket name.
            object_name: string, name of the file to look for.

        Returns:
            Value of "selfLink" field from the response, which represents
            a url to the file.

        Raises:
            errors.ResourceNotFoundError: when file is not found.
        """
        item = utils.RetryExceptionType(
                errors.ResourceNotFoundError,
                max_retries=self.GET_OBJ_MAX_RETRY, functor=self.Get,
                sleep_multiplier=self.GET_OBJ_RETRY_SLEEP,
                bucket_name=bucket_name, object_name=object_name)
        return item["selfLink"]
