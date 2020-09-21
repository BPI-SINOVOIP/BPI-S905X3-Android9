#!/usr/bin/env python
#
# Copyright 2016 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Module for handling Authentication.

Possible cases of authentication are noted below.

--------------------------------------------------------
     account                   | authentcation
--------------------------------------------------------

google account (e.g. gmail)*   | normal oauth2


service account*               | oauth2 + private key

--------------------------------------------------------

* For now, non-google employees (i.e. non @google.com account) or
  non-google-owned service account can not access Android Build API.
  Only local build artifact can be used.

* Google-owned service account, if used, needs to be whitelisted by
  Android Build team so that acloud can access build api.
"""

import logging
import os
import sys

import httplib2

from oauth2client import client as oauth2_client
from oauth2client.contrib import multistore_file
from oauth2client import tools as oauth2_tools

from acloud.public import errors

logger = logging.getLogger(__name__)
HOME_FOLDER = os.path.expanduser("~")


def _CreateOauthServiceAccountCreds(email, private_key_path, scopes):
    """Create credentials with a normal service account.

    Args:
        email: email address as the account.
        private_key_path: Path to the service account P12 key.
        scopes: string, multiple scopes should be saperated by space.
                        Api scopes to request for the oauth token.

    Returns:
        An oauth2client.OAuth2Credentials instance.

    Raises:
        errors.AuthentcationError: if failed to authenticate.
    """
    try:
        with open(private_key_path) as f:
            private_key = f.read()
            credentials = oauth2_client.SignedJwtAssertionCredentials(
                email, private_key, scopes)
    except EnvironmentError as e:
        raise errors.AuthentcationError(
            "Could not authenticate using private key file %s, error message: %s",
            private_key_path, str(e))
    return credentials


class RunFlowFlags(object):
    """Flags for oauth2client.tools.run_flow."""

    def __init__(self, browser_auth):
        self.auth_host_port = [8080, 8090]
        self.auth_host_name = "localhost"
        self.logging_level = "ERROR"
        self.noauth_local_webserver = not browser_auth


def _RunAuthFlow(storage, client_id, client_secret, user_agent, scopes):
    """Get user oauth2 credentials.

    Args:
        client_id: String, client id from the cloud project.
        client_secret: String, client secret for the client_id.
        user_agent: The user agent for the credential, e.g. "acloud"
        scopes: String, scopes separated by space.

    Returns:
        An oauth2client.OAuth2Credentials instance.
    """
    flags = RunFlowFlags(browser_auth=False)
    flow = oauth2_client.OAuth2WebServerFlow(
        client_id=client_id,
        client_secret=client_secret,
        scope=scopes,
        user_agent=user_agent)
    credentials = oauth2_tools.run_flow(
        flow=flow, storage=storage, flags=flags)
    return credentials


def _CreateOauthUserCreds(creds_cache_file, client_id, client_secret,
                          user_agent, scopes):
    """Get user oauth2 credentials.

    Args:
        creds_cache_file: String, file name for the credential cache.
                                            e.g. .acloud_oauth2.dat
                                            Will be created at home folder.
        client_id: String, client id from the cloud project.
        client_secret: String, client secret for the client_id.
        user_agent: The user agent for the credential, e.g. "acloud"
        scopes: String, scopes separated by space.

    Returns:
        An oauth2client.OAuth2Credentials instance.
    """
    if not client_id or not client_secret:
        raise errors.AuthentcationError(
            "Could not authenticate using Oauth2 flow, please set client_id "
            "and client_secret in your config file. Contact the cloud project's "
            "admin if you don't have the client_id and client_secret.")
    storage = multistore_file.get_credential_storage(
        filename=os.path.abspath(creds_cache_file),
        client_id=client_id,
        user_agent=user_agent,
        scope=scopes)
    credentials = storage.get()
    if credentials is not None:
        try:
            credentials.refresh(httplib2.Http())
        except oauth2_client.AccessTokenRefreshError:
            pass
        if not credentials.invalid:
            return credentials
    return _RunAuthFlow(storage, client_id, client_secret, user_agent, scopes)


def CreateCredentials(acloud_config, scopes):
    """Create credentials.

    Args:
        acloud_config: An AcloudConfig object.
        scopes: A string representing for scopes, separted by space,
            like "SCOPE_1 SCOPE_2 SCOPE_3"

    Returns:
        An oauth2client.OAuth2Credentials instance.
    """
    if acloud_config.service_account_private_key_path:
        return _CreateOauthServiceAccountCreds(
            acloud_config.service_account_name,
            acloud_config.service_account_private_key_path,
            scopes=scopes)

    creds_cache_file = os.path.join(HOME_FOLDER,
                                    acloud_config.creds_cache_file)
    return _CreateOauthUserCreds(
        creds_cache_file=creds_cache_file,
        client_id=acloud_config.client_id,
        client_secret=acloud_config.client_secret,
        user_agent=acloud_config.user_agent,
        scopes=scopes)
