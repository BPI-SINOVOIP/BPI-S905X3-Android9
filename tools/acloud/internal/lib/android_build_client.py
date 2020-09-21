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

"""A client that talks to Android Build APIs."""

import io
import logging
import os

import apiclient

from acloud.internal.lib import base_cloud_client
from acloud.public import errors

logger = logging.getLogger(__name__)


class AndroidBuildClient(base_cloud_client.BaseCloudApiClient):
    """Client that manages Android Build."""

    # API settings, used by BaseCloudApiClient.
    API_NAME = "androidbuildinternal"
    API_VERSION = "v2beta1"
    SCOPE = "https://www.googleapis.com/auth/androidbuild.internal"

    # other variables.
    DEFAULT_RESOURCE_ID = "0"
    # TODO(fdeng): We should use "latest".
    DEFAULT_ATTEMPT_ID = "0"
    DEFAULT_CHUNK_SIZE = 20 * 1024 * 1024
    NO_ACCESS_ERROR_PATTERN = "does not have storage.objects.create access"

    # Message constant
    COPY_TO_MSG = ("build artifact (target: %s, build_id: %s, "
                   "artifact: %s, attempt_id: %s) to "
                   "google storage (bucket: %s, path: %s)")

    def DownloadArtifact(self,
                         build_target,
                         build_id,
                         resource_id,
                         local_dest,
                         attempt_id=None):
        """Get Android build attempt information.

        Args:
            build_target: Target name, e.g. "gce_x86-userdebug"
            build_id: Build id, a string, e.g. "2263051", "P2804227"
            resource_id: Id of the resource, e.g "avd-system.tar.gz".
            local_dest: A local path where the artifact should be stored.
                        e.g. "/tmp/avd-system.tar.gz"
            attempt_id: String, attempt id, will default to DEFAULT_ATTEMPT_ID.
        """
        attempt_id = attempt_id or self.DEFAULT_ATTEMPT_ID
        api = self.service.buildartifact().get_media(
            buildId=build_id,
            target=build_target,
            attemptId=attempt_id,
            resourceId=resource_id)
        logger.info("Downloading artifact: target: %s, build_id: %s, "
                    "resource_id: %s, dest: %s", build_target, build_id,
                    resource_id, local_dest)
        try:
            with io.FileIO(local_dest, mode="wb") as fh:
                downloader = apiclient.http.MediaIoBaseDownload(
                    fh, api, chunksize=self.DEFAULT_CHUNK_SIZE)
                done = False
                while not done:
                    _, done = downloader.next_chunk()
            logger.info("Downloaded artifact: %s", local_dest)
        except OSError as e:
            logger.error("Downloading artifact failed: %s", str(e))
            raise errors.DriverError(str(e))

    def CopyTo(self,
               build_target,
               build_id,
               artifact_name,
               destination_bucket,
               destination_path,
               attempt_id=None):
        """Copy an Android Build artifact to a storage bucket.

        Args:
            build_target: Target name, e.g. "gce_x86-userdebug"
            build_id: Build id, a string, e.g. "2263051", "P2804227"
            artifact_name: Name of the artifact, e.g "avd-system.tar.gz".
            destination_bucket: String, a google storage bucket name.
            destination_path: String, "path/inside/bucket"
            attempt_id: String, attempt id, will default to DEFAULT_ATTEMPT_ID.
        """
        attempt_id = attempt_id or self.DEFAULT_ATTEMPT_ID
        copy_msg = "Copying %s" % self.COPY_TO_MSG
        logger.info(copy_msg, build_target, build_id, artifact_name,
                    attempt_id, destination_bucket, destination_path)
        api = self.service.buildartifact().copyTo(
            buildId=build_id,
            target=build_target,
            attemptId=attempt_id,
            artifactName=artifact_name,
            destinationBucket=destination_bucket,
            destinationPath=destination_path)
        try:
            self.Execute(api)
            finish_msg = "Finished copying %s" % self.COPY_TO_MSG
            logger.info(finish_msg, build_target, build_id, artifact_name,
                        attempt_id, destination_bucket, destination_path)
        except errors.HttpError as e:
            if e.code == 503:
                if self.NO_ACCESS_ERROR_PATTERN in str(e):
                    error_msg = "Please grant android build team's service account "
                    error_msg += "write access to bucket %s. Original error: %s"
                    error_msg %= (destination_bucket, str(e))
                    raise errors.HttpError(e.code, message=error_msg)
            raise
