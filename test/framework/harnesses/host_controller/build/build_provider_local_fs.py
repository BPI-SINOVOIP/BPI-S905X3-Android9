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

from host_controller.build import build_provider


class BuildProviderLocalFS(build_provider.BuildProvider):
    """A build provider for local file system (fs)."""

    def __init__(self):
        super(BuildProviderLocalFS, self).__init__()

    def Fetch(self, path):
        """Fetches Android device artifact file(s) from a local path.

        Args:
            path: string, the path of the artifacts.

        Returns:
            a dict containing the device image info.
            a dict containing the test suite package info.
        """
        if path.endswith(".tar.md5"):
            self.SetDeviceImage("img", path)
        else:
            self.SetFetchedFile(path)
        return self.GetDeviceImage(), self.GetTestSuitePackage()
