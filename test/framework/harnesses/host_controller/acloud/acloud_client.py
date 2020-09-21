#
# Copyright (C) 2018 The Android Open Source Project
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

import getpass
import json
import logging
import os
from os.path import expanduser
import re
import shutil
import tempfile

from acloud.public import acloud_main
from host_controller.acloud import acloud_config
from vts.utils.python.common import cmd_utils

DEFAULT_BRANCH = 'git_master'
DEFAULT_BUILD_TARGET = 'gce_x86_phone-userdebug_fastbuild3c_linux'

#TODO(yuexima): add full support to multiple instances per host


class ACloudClient(object):
    '''Helper class to manage access to the acloud module.'''

    def __init__(self):
        tmpdir_base = os.path.join(os.getcwd(), "tmp")
        if not os.path.exists(tmpdir_base):
            os.mkdir(tmpdir_base)
        self._tmpdir = tempfile.mkdtemp(dir=tmpdir_base)

    def __del__(self):
        """Deletes the temp dir if still set."""
        if self._tmpdir:
            shutil.rmtree(self._tmp_dirpath)
            self._tmpdir = None

    def GetCreateCmd(self,
                     build_id,
                     branch=None,
                     build_target=None,
                     num=1):
        '''Get acould create command with given build id.

        Args:
            build_id: string, build_id.
            branch: string, build branch
            build_target: string, build target
            num: int, number of instances to build

        Returns:
            string, acloud create command.
        '''
        if not branch:
            branch = DEFAULT_BRANCH

        if not build_target:
            build_target = DEFAULT_BUILD_TARGET

        #TODO latest build id (in the caller class of this function).
        #TODO use unique log and tmp file location
        cmd = ('create '
            '--branch {branch} '
            '--build_target {build_target} '
            '--build_id {build_id} '
            '--config_file {config_file} '
            '--report_file {report_file} '
            '--log_file {log_file} '
            '--email {email} '
            '--num {num}').format(
            branch = branch,
            build_target = build_target,
            build_id = build_id,
            config_file = os.path.join(self._tmpdir, 'acloud.config'),
            report_file = os.path.join(self._tmpdir, 'acloud_report.json'),
            log_file = os.path.join(self._tmpdir, 'acloud.log'),
            #TODO use host email address
            email = getpass.getuser() + '@google.com',
            num = num
        )
        return cmd

    def GetDeleteCmd(self, instance_names):
        '''Get Acould delete command with given instance names.

        Args:
            instance_names: list of string, instance names.

        Returns:
            string, acloud create command.
        '''
        cmd = ('delete '
            '--instance_names {instance_names} '
            '--config_file {config_file} '
            '--report_file {report_file} '
            '--log_file {log_file} '
            '--email {email}').format(
            instance_names = ' '.join(instance_names),
            config_file = os.path.join(self._tmpdir, 'acloud.config'),
            report_file = os.path.join(self._tmpdir, 'acloud_report.json'),
            log_file = os.path.join(self._tmpdir, 'acloud.log'),
            email = getpass.getuser() + '@google.com'
        )
        return cmd

    def CreateInstance(self, build_id):
        '''Create Acould instance with given build id.

        Args:
            build_id: string, build_id.
        '''
        cmd = self.GetCreateCmd(build_id)
        acloud_main.main(cmd.split())

        report_file = os.path.join(self._tmpdir, 'acloud_report.json')
        with open(report_file, 'r') as f:
            report = json.load(f)

        return report['status']=='SUCCESS'

    def PrepareConfig(self, file_path):
        '''Prepare acloud Acloud config file.

        Args:
            file_path: string, path to acloud config file.
        '''
        config = acloud_config.ACloudConfig()
        config.Load(file_path)
        if not config.Validate():
            logging.error('Failed to prepare acloud config.')
            return

        config.Save(os.path.join(self._tmpdir, 'acloud.config'))

    def GetInstanceIP(self):
        '''Get an Acloud instance ip'''
        #TODO support ip addresses when num > 1 (json parser)
        report_file = os.path.join(self._tmpdir, 'acloud_report.json')

        with open(report_file, 'r') as f:
            report = json.load(f)

        return report['data']['devices'][0]['ip']

    def GetInstanceName(self):
        '''Get an Acloud instance name'''
        report_file = os.path.join(self._tmpdir, 'acloud_report.json')

        with open(report_file, 'r') as f:
            report = json.load(f)

        return report['data']['devices'][0]['instance_name']

    def ConnectInstanceToAdb(self, ip=None):
        '''Connect an Acloud instance to adb

        Args:
            ip: string, ip address
        '''
        if not ip:
            ip = self.GetInstanceIP()

        cmds = [('ssh -i ~/.ssh/acloud_rsa -o UserKnownHostsFile=/dev/null '
                 '-o StrictHostKeyChecking=no -L 40000:127.0.0.1:6444 -L '
                 '30000:127.0.0.1:5555 -N -f -l root %s') % ip,
                 'adb connect 127.0.0.1:30000']

        for cmd in cmds:
            print cmd
            results = cmd_utils.ExecuteShellCommand(cmd)
            if any(results[cmd_utils.EXIT_CODE]):
                logging.error("Fail to execute command: %s\nResult:%s" % (cmd,
                                                                        results))
                return
            print 'Acloud instance created and connected to local port 127.0.0.1:30000'

    def DeleteInstance(self, instance_names=None):
        '''Delete an Acloud instance

        Args:
            instance_names: string, instance name
        '''
        cmd = self.GetDeleteCmd(instance_names)
        acloud_main.main(cmd.split())

        report_file = os.path.join(self._tmpdir, 'acloud_report.json')
        with open(report_file, 'r') as f:
            report = json.load(f)

        return report['status']=='SUCCESS'
