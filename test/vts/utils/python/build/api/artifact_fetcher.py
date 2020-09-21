#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Class to fetch artifacts from internal build server.
"""

import googleapiclient
import httplib2
import io
import json
import logging
import re
import time
from googleapiclient.discovery import build
from oauth2client import client as oauth2_client
from oauth2client.service_account import ServiceAccountCredentials

logger = logging.getLogger('artifact_fetcher')


class DriverError(Exception):
    """Base Android GCE driver exception."""


class AndroidBuildClient(object):
    """Client that manages Android Build.

    Attributes:
        service: object, initialized and authorized service object for the
                 androidbuildinternal API.
        API_NAME: string, name of internal API accessed by the client.
        API_VERSION: string, version of the internal API accessed by the client.
        SCOPE: string, URL for which to request access via oauth2.
        DEFAULT_RESOURCE_ID: string, default artifact name to request.
        DEFAULT_ATTEMPT_ID: string, default attempt to request for the artifact.
        DEFAULT_CHUNK_SIZE: int, number of bytes to download at a time.
        RETRY_COUNT: int, max number of retries.
        RETRY_DELAY_IN_SECS: int, time delays between retries in seconds.
    """

    API_NAME = "androidbuildinternal"
    API_VERSION = "v2beta1"
    SCOPE = "https://www.googleapis.com/auth/androidbuild.internal"

    # other variables.
    BUILDS_KEY = "builds"
    BUILD_ID_KEY = "buildId"
    DEFAULT_ATTEMPT_ID = "latest"
    DEFAULT_BUILD_ATTEMPT_STATUS = "complete"
    DEFAULT_BUILD_TYPE = "submitted"
    DEFAULT_CHUNK_SIZE = 20 * 1024 * 1024
    DEFAULT_RESOURCE_ID = "0"

    # Defaults for retry.
    RETRY_COUNT = 5
    RETRY_DELAY_IN_SECS = 3

    def __init__(self, oauth2_service_json):
        """Initialize.

        Args:
          oauth2_service_json: Path to service account json file.
        """
        authToken = ServiceAccountCredentials.from_json_keyfile_name(
            oauth2_service_json, [self.SCOPE])
        http_auth = authToken.authorize(httplib2.Http())
        for _ in xrange(self.RETRY_COUNT):
            try:
                self.service = build(
                    serviceName=self.API_NAME,
                    version=self.API_VERSION,
                    http=http_auth)
                break
            except oauth2_client.AccessTokenRefreshError as e:
                # The following HTTP code typically indicates transient errors:
                #    500  (Internal Server Error)
                #    502  (Bad Gateway)
                #    503  (Service Unavailable)
                logging.exception(e)
                logging.info("Retrying to connect to %s", self.API_NAME)
                time.sleep(self.RETRY_DELAY_IN_SECS)

    def DownloadArtifactToFile(self,
                               branch,
                               build_target,
                               build_id,
                               resource_id,
                               dest_filepath,
                               attempt_id=None):
        """Get artifact from android build server.

        Args:
            branch: Branch from which the code was built, e.g. "master"
            build_target: Target name, e.g. "gce_x86-userdebug"
            build_id: Build id, a string, e.g. "2263051", "P2804227"
            resource_id: Name of resource to be downloaded, a string.
            attempt_id: string, attempt id, will default to DEFAULT_ATTEMPT_ID.
            dest_filepath: string, set a file path to store to a file.

        Returns:
            Contents of the requested resource as a string if dest_filepath is None;
            None otherwise.
        """
        return self.GetArtifact(branch, build_target, build_id, resource_id,
                                attempt_id=attempt_id, dest_filepath=dest_filepath)

    def GetArtifact(self,
                    branch,
                    build_target,
                    build_id,
                    resource_id,
                    attempt_id=None,
                    dest_filepath=None):
        """Get artifact from android build server.

        Args:
            branch: Branch from which the code was built, e.g. "master"
            build_target: Target name, e.g. "gce_x86-userdebug"
            build_id: Build id, a string, e.g. "2263051", "P2804227"
            resource_id: Name of resource to be downloaded, a string.
            attempt_id: string, attempt id, will default to DEFAULT_ATTEMPT_ID.
            dest_filepath: string, set a file path to store to a file.

        Returns:
            Contents of the requested resource as a string if dest_filepath is None;
            None otherwise.
        """
        attempt_id = attempt_id or self.DEFAULT_ATTEMPT_ID
        api = self.service.buildartifact().get_media(
            buildId=build_id,
            target=build_target,
            attemptId=attempt_id,
            resourceId=resource_id)
        logger.info("Downloading artifact: target: %s, build_id: %s, "
                    "resource_id: %s", build_target, build_id, resource_id)
        fh = None
        try:
            if dest_filepath:
                fh = io.FileIO(dest_filepath, mode='wb')
            else:
                fh = io.BytesIO()

            downloader = googleapiclient.http.MediaIoBaseDownload(
                fh, api, chunksize=self.DEFAULT_CHUNK_SIZE)
            done = False
            while not done:
                _, done = downloader.next_chunk()
            logger.info("Downloaded artifact %s" % resource_id)

            if not dest_filepath:
                return fh.getvalue()
        except OSError as e:
            logger.error("Downloading artifact failed: %s", str(e))
            raise DriverError(str(e))
        finally:
            if fh:
                fh.close()

    def GetManifest(self, branch, build_target, build_id, attempt_id=None):
        """Get Android build manifest XML file.

        Args:
            branch: Branch from which the code was built, e.g. "master"
            build_target: Target name, e.g. "gce_x86-userdebug"
            build_id: Build id, a string, e.g. "2263051", "P2804227"
            attempt_id: String, attempt id, will default to DEFAULT_ATTEMPT_ID.


        Returns:
            Contents of the requested XML file as a string.
        """
        resource_id = "manifest_%s.xml" % build_id
        return self.GetArtifact(branch, build_target, build_id, resource_id,
                                attempt_id)

    def GetRepoDictionary(self,
                          branch,
                          build_target,
                          build_id,
                          attempt_id=None):
        """Get dictionary of repositories and git revision IDs

        Args:
            branch: Branch from which the code was built, e.g. "master"
            build_target: Target name, e.g. "gce_x86-userdebug"
            build_id: Build id, a string, e.g. "2263051", "P2804227"
            attempt_id: String, attempt id, will default to DEFAULT_ATTEMPT_ID.


        Returns:
            Dictionary of project names (string) to commit ID (string)
        """
        resource_id = "BUILD_INFO"
        build_info = self.GetArtifact(branch, build_target, build_id,
                                      resource_id, attempt_id)
        try:
            return json.loads(build_info)["repo-dict"]
        except (ValueError, KeyError):
            logger.warn("Could not find repo dictionary.")
            return {}

    def GetCoverage(self,
                    branch,
                    build_target,
                    build_id,
                    product,
                    attempt_id=None):
        """Get Android build coverage zip file.

        Args:
            branch: Branch from which the code was built, e.g. "master"
            build_target: Target name, e.g. "gce_x86-userdebug"
            build_id: Build id, a string, e.g. "2263051", "P2804227"
            product: Name of product for build target, e.g. "bullhead", "angler"
            attempt_id: String, attempt id, will default to DEFAULT_ATTEMPT_ID.


        Returns:
            Contents of the requested zip file as a string.
        """
        resource_id = ("%s-coverage-%s.zip" % (product, build_id))
        return self.GetArtifact(branch, build_target, build_id, resource_id,
                                attempt_id)

    def ListBuildIds(self,
                     branch,
                     build_target,
                     limit=1,
                     build_type=DEFAULT_BUILD_TYPE,
                     build_attempt_status=DEFAULT_BUILD_ATTEMPT_STATUS):
        """Get a list of most recent build IDs.

        Args:
            branch: Branch from which the code was built, e.g. "master"
            build_target: Target name, e.g. "gce_x86-userdebug"
            limit: (optional) an int, max number of build IDs to fetch,
                default of 1
            build_type: (optional) a string, the build type to filter, default
                of "submitted"
            build_attempt_status: (optional) a string, the build attempt status
                to filter, default of "completed"

        Returns:
            A list of build ID strings in reverse time order.
        """
        builds = self.service.build().list(
            branch=branch,
            target=build_target,
            maxResults=limit,
            buildType=build_type,
            buildAttemptStatus=build_attempt_status).execute()
        return [str(build.get(self.BUILD_ID_KEY))
                for build in builds.get(self.BUILDS_KEY)]
