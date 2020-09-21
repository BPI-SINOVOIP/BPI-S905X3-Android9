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


class PathPermission(check_setup_cleanup.CheckSetupCleanup):
    """Class for check and set path permissions.

    Attributes:
        to_check: bool, whether or not to check the defined environment
                  requirement. Default: True
        to_setup: bool, whether or not to setup the defined environment
                  requirement. Default: False
        to_cleanup: bool, whether or not to cleanup the defined environment
                    requirement if it is set up by this class. Default: False
        _paths: list string, target paths
        _failed_paths: list of string, paths that don't have desired permissions
        _permissions: list of int, desired permissions for each path
    """

    def __init__(self,
                 paths=None,
                 permissions=None,
                 to_check=True,
                 to_setup=False,
                 to_cleanup=False):
        self._paths = paths
        self._permissions = permissions
        self.to_check = to_check
        self.to_setup = to_setup
        self.to_cleanup = to_cleanup

    def ValidateInputs(self):
        """Validate inputs.

        Check whether input lists is not null or empty list
        or list containing empty string, and two lists containing same number
        of items. If inputs are two single item, they will
        be converted to single item lists.
        """
        normalized = self.NormalizeInputLists(self._paths, self._permissions)
        if not normalized:
            return False
        self._paths, self._permissions = normalized
        self._failed_paths = zip(self._paths, self._permissions,
                                 self._permissions)

        return True

    def Check(self):
        commands = ["stat -c {}a {}".format('%', path) for path in self._paths]
        results = self.ExecuteShellCommand(commands)[const.STDOUT]

        self._failed_paths = [
            (path, permission, result)
            for path, permission, result in zip(self._paths, self._permissions,
                                                results)
            if str(permission) != result
        ]

        if not self._failed_paths:
            return True

        self.note = ("Some paths do not have desired "
                     "permission: %s") % self._failed_paths
        return False

    def Setup(self):
        commands = ["chmod {} {}".format(permission, path)
                    for (path, permission, result) in self._failed_paths]
        # TODO: perhaps store or print failed setup paths
        return not any(self.ExecuteShellCommand(commands)[const.EXIT_CODE])

    def Cleanup(self):
        commands = ["chmod {} {}".format(original, path)
                    for (path, permission, original) in self._failed_paths]
        return not any(self.ExecuteShellCommand(commands)[const.EXIT_CODE])
