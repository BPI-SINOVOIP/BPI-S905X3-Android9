# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2015, ARM Limited and contributors.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging
import os
import shutil
import subprocess
from collections import namedtuple

class Build(object):
    """
    Collection of Android related build actions
    """
    def __init__(self, te):
        if (te.ANDROID_BUILD_TOP and te.TARGET_PRODUCT and te.TARGET_BUILD_VARIANT):
            self._te = te
        else:
            te._log.warning('Build initialization failed: invalid paramterers')
            raise

    def exec_cmd(self, cmd):
        ret = subprocess.call(cmd, shell=True)
        if ret != 0:
            raise RuntimeError('Command \'{}\' returned error code {}'.format(cmd, ret))

    def build_module(self, module_path):
        """
        Build a module and its dependencies.

        :param module_path: module path
        :type module_path: str

        """
        cur_dir = os.getcwd()
        os.chdir(self._te.ANDROID_BUILD_TOP)
        lunch_target = self._te.TARGET_PRODUCT + '-' + self._te.TARGET_BUILD_VARIANT
        self.exec_cmd('source build/envsetup.sh && lunch ' +
            lunch_target + ' && mmma -j16 ' + module_path)
        os.chdir(cur_dir)


# vim :set tabstop=4 shiftwidth=4 expandtab
