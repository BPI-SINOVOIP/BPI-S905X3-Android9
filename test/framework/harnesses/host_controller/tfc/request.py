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


class Request(api_message.ApiMessage):
    """The requests defined by TFC API.

    Attributes:
        _BODY: The requests.new parameters that are put into http message body.
        _PARAMETERS: The requests.new parameters put into http parameters.
        _ALL_KEYS: Union of above.
    """
    _BODY = {
            "command_line",
            "user"}
    _PARAMETERS = {
            "branch",
            "build_flavor",
            "build_id",
            "build_os",
            "cluster",
            "no_build_args",
            "run_target",
            "shard_count"
            "run_count"}
    _ALL_KEYS = (_BODY | _PARAMETERS)

    def __init__(self, cluster, command_line, run_target, user, **kwargs):
        """Initializes the attributes.

        Args:
            cluster: The ID of the cluster to send this request to.
            command_line: The command to execute on a host.
            run_target: The target device to run the command.
            user: The name of the user sending this request.
            **kwargs: The optional attributes.
        """
        super(Request, self).__init__(self._ALL_KEYS,
                                      cluster=cluster,
                                      command_line=command_line,
                                      run_target=run_target,
                                      user=user,
                                      **kwargs)

    def GetBody(self):
        """Returns the http message body.

        Returns:
            A JSON object.
        """
        return self.ToJson(self._BODY)

    def GetParameters(self):
        """Returns the http parameters.

        Returns:
            A dict of strings.
        """
        return self.ToJson(self._PARAMETERS)
