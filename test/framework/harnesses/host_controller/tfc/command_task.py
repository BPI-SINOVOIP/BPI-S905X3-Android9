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

from host_controller.tfc import api_message


class CommandTask(api_message.ApiMessage):
    """The task of executing a command defined by TFC API.

    Attributes:
        _LEASE_HOST_TASK: The fields returned by commandAttempts.list.
    """
    _LEASE_HOST_TASK = {
            "request_id",
            "command_id",
            "task_id",
            "command_line",
            "request_type",
            "device_serials"}

    def __init__(self, task_id, command_line, device_serials, **kwargs):
        super(CommandTask, self).__init__(self._LEASE_HOST_TASK,
                                          task_id=task_id,
                                          command_line=command_line,
                                          device_serials=device_serials,
                                          **kwargs)
