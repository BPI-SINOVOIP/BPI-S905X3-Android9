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
import logging
import os
import re
import shutil
import tempfile

from vts.runners.host import const
from vts.utils.python.mirror import mirror_object


class ShellMirror(mirror_object.MirrorObject):
    """The class that acts as the mirror to an Android device's shell terminal.

    Attributes:
        _client: the TCP client instance.
        _adb: An AdbProxy object used for interacting with the device via adb.
        enabled: bool, whether remote shell feature is enabled for the device.
    """

    TMP_FILE_PATTERN = "/data/local/tmp/nohup.*"

    def __init__(self, client, adb):
        super(ShellMirror, self).__init__(client)
        self._adb = adb
        self.enabled = True

    def Execute(self, command, no_except=False):
        '''Execute remote shell commands on device.

        Args:
            command: string or a list of string, shell commands to execute on
                     device.
            no_except: bool, if set to True, no exception will be thrown and
                       error code will be -1 with error message on stderr.

        Returns:
            A dictionary containing shell command execution results
        '''
        if not self.enabled:
            # TODO(yuexima): use adb shell instead when RPC is disabled
            return {
                const.STDOUT: [""] * len(command),
                const.STDERR:
                ["VTS remote shell has been disabled."] * len(command),
                const.EXIT_CODE: [-2] * len(command)
            }
        result = self._client.ExecuteShellCommand(command, no_except)

        tmp_dir = tempfile.mkdtemp()
        pattern = re.compile(self.TMP_FILE_PATTERN)

        for result_val, result_type in zip(
            [result[const.STDOUT], result[const.STDERR]],
            ["stdout", "stderr"]):
            for index, val in enumerate(result_val):
                # If val is a tmp file name, pull the file and set the contents
                # to result.
                if pattern.match(val):
                    tmp_file = os.path.join(tmp_dir, result_type + str(index))
                    logging.info("pulling file: %s to %s", val, tmp_file)
                    self._adb.pull(val, tmp_file)
                    result_val[index] = open(tmp_file, "r").read()
                    self._adb.shell("rm -f %s" % val)
                else:
                    result_val[index] = val

        shutil.rmtree(tmp_dir)
        logging.debug("resp for VTS_AGENT_COMMAND_EXECUTE_SHELL_COMMAND: %s",
                      result)
        return result

    def SetConnTimeout(self, timeout):
        """Set remote shell connection timeout.

        Args:
            timeout: int, TCP connection timeout in seconds.
        """
        self._client.timeout = timeout
