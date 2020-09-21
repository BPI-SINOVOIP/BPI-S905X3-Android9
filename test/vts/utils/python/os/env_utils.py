#
# Copyright 2017 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os


def SaveAndClearEnvVars(var_name_list):
    """Saves and clears the values of existing environment variables.

    Args:
        var_name_list: a list of strings where each string is the environment
                       variable name.

    Returns:
        a dict where the key is an environment variable and the value is the
        saved environment variable value.
    """
    env_vars = {}
    for var_name in var_name_list:
        if var_name in os.environ:
            env_vars[var_name] = os.environ[var_name]
            os.environ[var_name] = ""
    return env_vars


def RestoreEnvVars(env_vars):
    """Restores the values of existing environment variables.

    Args:
        env_vars: a dict where the key is an environment variable and
                  the value is the environment variable value to set.
    """
    for var_name in env_vars:
        os.environ[var_name] = env_vars[var_name]
