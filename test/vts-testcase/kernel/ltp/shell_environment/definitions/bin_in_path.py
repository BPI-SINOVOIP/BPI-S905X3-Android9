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

from vts.runners.host import const
from vts.testcases.kernel.ltp.shell_environment.definitions.base_definitions import check_setup_cleanup


class BinInPath(check_setup_cleanup.CheckSetupCleanup):
    """Class for check existence of, make, and afterwards delete directories.

    Attributes:
        to_check: bool, whether or not to check the defined environment
                  requirement. Default: True
        to_setup: bool, whether or not to setup the defined environment
                  requirement. Default: False
        to_cleanup: bool, whether or not to cleanup the defined environment
                    requirement if it is set up by this class. Default: False
        _paths: list string, target directory paths
        _failed_paths: list of string, paths that don't have desired permissions
    """

    def __init__(self,
                 paths=None,
                 to_check=True,
                 to_setup=False,
                 to_cleanup=False):
        self._paths = paths
        self._failed_paths = paths
        self.to_check = to_check
        self.to_setup = to_setup
        self.to_cleanup = to_cleanup

    def ValidateInputs(self):
        """Validate input paths.

        Check input path is not null or empty list or list containing
        empty string. If input is a single path, it will
        be converted to a single item list containing that path.
        """
        if not self._paths:
            return False

        self._paths = self.ToListLike(self._paths)

        return all(self._paths)

    def Check(self):
        commands = ["which %s" % path for path in self._paths]
        results = self.ExecuteShellCommand(commands)[const.EXIT_CODE]

        self._failed_paths = [
            path for path, fail in zip(self._paths, map(bool, results)) if fail
        ]

        if not self._failed_paths:
            return True

        self.note = "Some binary do not exist in path: %s" % self._failed_paths
        return False
