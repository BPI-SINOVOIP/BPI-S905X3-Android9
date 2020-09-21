#
# Copyright (C) 2017 The Android Open Source Project
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

import logging
import os
import re
import zipfile

from host_controller.build import build_provider
from vts.utils.python.common import cmd_utils

_GCLOUD_AUTH_ENV_KEY = "run_gcs_key"


class BuildProviderGCS(build_provider.BuildProvider):
    """A build provider for GCS (Google Cloud Storage)."""

    def __init__(self):
        super(BuildProviderGCS, self).__init__()
        if _GCLOUD_AUTH_ENV_KEY in os.environ:
            gcloud_path = BuildProviderGCS.GetGcloudPath()
            if gcloud_path is not None:
                auth_cmd = "%s auth activate-service-account --key-file=%s" % (
                    gcloud_path, os.environ[_GCLOUD_AUTH_ENV_KEY])
                _, stderr, ret_code = cmd_utils.ExecuteOneShellCommand(
                    auth_cmd)
                if ret_code == 0:
                    logging.info(stderr)
                else:
                    print(stderr)
                    logging.error(stderr)

    @staticmethod
    def GetGcloudPath():
        """Returns the gcloud file path if found; None otherwise."""
        sh_stdout, _, ret_code = cmd_utils.ExecuteOneShellCommand(
            "which gcloud")
        if ret_code == 0:
            return sh_stdout.strip()
        else:
            logging.error("`gcloud` doesn't exist on the host; "
                          "please install Google Cloud SDK before retrying.")
            return None

    @staticmethod
    def GetGsutilPath():
        """Returns the gsutil file path if found; None otherwise."""
        sh_stdout, sh_stderr, ret_code = cmd_utils.ExecuteOneShellCommand(
            "which gsutil")
        if ret_code == 0:
            return sh_stdout.strip()
        else:
            logging.fatal("`gsutil` doesn't exist on the host; "
                          "please install Google Cloud SDK before retrying.")
            return None

    @staticmethod
    def IsGcsFile(gsutil_path, gs_path):
        """Checks whether a given path is for a GCS file.

        Args:
            gsutil_path: string, the path of a gsutil binary.
            gs_path: string, the GCS file path (e.g., gs://<bucket>/<file>.

        Returns:
            True if gs_path is a file, False otherwise.
        """
        check_command = "%s stat %s" % (gsutil_path, gs_path)
        _, _, ret_code = cmd_utils.ExecuteOneShellCommand(check_command)
        return ret_code == 0

    def Fetch(self, path):
        """Fetches Android device artifact file(s) from GCS.

        Args:
            path: string, the path of a directory which keeps artifacts.

        Returns:
            a dict containing the device image info.
            a dict containing the test suite package info.
            a dict containing the info about custom tool files.
        """
        if not path.startswith("gs://"):
            path = "gs://" + re.sub("^/*", "", path)
        path = re.sub("/*$", "", path)
        # make sure gsutil is available. Instead of a Python library,
        # gsutil binary is used that is to avoid packaging GCS PIP package
        # as part of VTS HC (Host Controller).
        gsutil_path = BuildProviderGCS.GetGsutilPath()
        if gsutil_path:
            temp_dir_path = self.CreateNewTmpDir()
            # IsGcsFile returns False if path is directory or doesn't exist.
            # cp command returns non-zero if path doesn't exist.
            if not BuildProviderGCS.IsGcsFile(gsutil_path, path):
                dest_path = temp_dir_path
                copy_command = "%s cp -r %s/* %s" % (gsutil_path, path,
                                                     temp_dir_path)
            else:
                dest_path = os.path.join(temp_dir_path, os.path.basename(path))
                copy_command = "%s cp %s %s" % (gsutil_path, path,
                                                temp_dir_path)

            _, _, ret_code = cmd_utils.ExecuteOneShellCommand(copy_command)
            if ret_code == 0:
                self.SetFetchedFile(dest_path, temp_dir_path)
            else:
                logging.error("Error in copy file from GCS (code %s)." %
                              ret_code)
        return (self.GetDeviceImage(),
                self.GetTestSuitePackage(),
                self.GetAdditionalFile())
