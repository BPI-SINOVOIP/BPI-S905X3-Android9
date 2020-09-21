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
import time

def GetTimestamp():
    """Returns the current UTC time (unit: microseconds)."""
    return int(time.time() * 1000000)

class Feature(object):
    """Configuration object for a feature.

    Stores configuration parameters and metadata.

    Attributes:
        enabled: boolean, True if the feature is enabled, False otherwise
    """

    enabled = False

    def ParseParameters(self, toggle_param_name=None, required_param_names=[],
                        optional_param_names=[], user_params={}):
        """Creates a feature configuration object.

        Args:
            toggle_param_name: String, The name of the parameter used to toggle the feature
            required_param_names: list, The list of parameter names that are required
            optional_param_names: list, The list of parameter names that are optional
        """
        self.enabled = False
        if toggle_param_name:
            if toggle_param_name not in user_params:
                logging.info("Missing toggle parameter in configuration: %s", toggle_param_name)
                return
            if not user_params[toggle_param_name]:
                logging.info("Feature disabled in configuration: %s=False", toggle_param_name)
                return

        for name in required_param_names:
            if name not in user_params:
                return
            else:
                setattr(self, name, user_params[name])

        self.enabled = True

        for name in optional_param_names:
            if name in user_params:
               setattr(self, name, user_params[name])
