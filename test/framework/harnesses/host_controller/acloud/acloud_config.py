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

import logging
import os


# In fact, all fields are required. The fields listed below are used
# to check whether the config class has been properly initialized before
# generating config file
REQUIRED_KEYS = [
    'ssh_public_key_path',
    'ssh_private_key_path',
    'project',
    'client_id',
    'client_secret'
    ]


class ACloudConfig(object):
    '''For ACloud configuration file operations.

    Attributes:
        configs: dict of string:string, configuration keys and values.
        has_error: bool, whether error occurred.
    '''
    configs = {}
    has_error = False

    def Validate(self):
        '''Validate config class.

        Check whether required fields has been set.
        Check whether loading configuration file is success.

        Returns:
            bool, True if validated.
        '''
        for key in REQUIRED_KEYS:
            if key not in self.configs:
                logging.error('Required key %s is not '
                              'set for acloud config' % key)
                return False

        return not self.has_error

    def Load(self, file_path):
        '''Load configs from a file.

        Args:
            file_path: string, path to config file.
        '''
        if not os.path.isfile(file_path):
            logging.error('Failed to read acloud config file %s' % file_path)
            self.has_error = True
            return

        separator = ': "'

        with open(file_path, 'r') as f:
            for line in f:
                line = line.strip()
                # Skip empty line and comments
                if not line or line.startswith('#'):
                    continue

                idx = line.find(separator)

                if idx < 1 or not line.endswith('"'):
                    logging.error('Error parsing line %s from '
                                  'acloud config file %s' % (line, file_path))
                    self.has_error = True
                    return

                key = line[:idx]
                val = line[len(separator) + idx : -1]

                self.configs[key] = val

    def Save(self, file_path):
        '''Save config to a file.

        Args:
            file_path: string, path to config file.
        '''
        separator = ':'

        with open(file_path, 'w') as f:
            for key in self.configs:
                f.write(key + separator + '"%s"' % self.configs[key])