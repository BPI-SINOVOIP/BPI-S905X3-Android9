
# Copyright 2016 Google Inc.
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

import base64
import logging
import os
import subprocess
import tempfile

from oauth2client import service_account

_OPENID_SCOPE = 'openid'

class DashboardRestClient(object):
    """Instance of the Dashboard REST client.

    Attributes:
        post_cmd: String, The command-line string to post data to the dashboard,
                  e.g. 'wget <url> --post-file '
        service_json_path: String, The path to the service account keyfile
                           created from Google App Engine settings.
        auth_token: ServiceAccountCredentials object or None if not
                    initialized.
    """

    def __init__(self, post_cmd, service_json_path):
        self.post_cmd = post_cmd
        self.service_json_path = service_json_path
        self.auth_token = None

    def Initialize(self):
        """Initializes the client with an auth token and access token.

        Returns:
            True if the client is initialized successfully, False otherwise.
        """
        try:
            self.auth_token = service_account.ServiceAccountCredentials.from_json_keyfile_name(
                self.service_json_path, [_OPENID_SCOPE])
            self.auth_token.get_access_token()
        except IOError as e:
            logging.error("Error reading service json keyfile: %s", e)
            return False
        except (ValueError, KeyError) as e:
            logging.error("Invalid service json keyfile: %s", e)
            return False
        return True


    def _GetToken(self):
        """Gets an OAuth2 token using from a service account json keyfile.

        Uses the service account keyfile located at 'service_json_path', provided
        to the constructor, to request an OAuth2 token.

        Returns:
            String, an OAuth2 token using the service account credentials.
            None if authentication fails.
        """
        return str(self.auth_token.get_access_token().access_token)

    def AddAuthToken(self, post_message):
        """Add OAuth2 token to the dashboard message.

        Args:
            post_message: DashboardPostMessage, The data to post.

        Returns:
            True if successful, False otherwise
        """
        token = self._GetToken()
        if not token:
            return False

        post_message.access_token = token

    def PostData(self, post_message):
        """Post data to the dashboard database.

        Puts data into the dashboard database using its proto REST endpoint.

        Args:
            post_message: DashboardPostMessage, The data to post.

        Returns:
            True if successful, False otherwise
        """
        post_bytes = base64.b64encode(post_message.SerializeToString())

        with tempfile.NamedTemporaryFile(delete=False) as file:
            file.write(post_bytes)
        p = subprocess.Popen(
            self.post_cmd.format(path=file.name),
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        output, err = p.communicate()
        os.remove(file.name)

        if p.returncode or err:
            logging.error("Row insertion failed: %s", err)
            return False
        return True
