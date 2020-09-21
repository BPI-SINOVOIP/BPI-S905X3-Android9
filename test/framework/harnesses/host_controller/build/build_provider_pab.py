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
"""Module to fetch artifacts from Partner Android Build server."""

import argparse
import getpass
import httplib2
import json
import logging
import os
import requests
import urlparse
from posixpath import join as path_urljoin

from oauth2client.client import flow_from_clientsecrets
from oauth2client.file import Storage
from oauth2client.tools import argparser
from oauth2client.tools import run_flow

from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.common.exceptions import TimeoutException
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.support.ui import WebDriverWait

from host_controller.build import build_provider

# constants for GET and POST endpoints
GET = 'GET'
POST = 'POST'


class BuildProviderPAB(build_provider.BuildProvider):
    """Client that manages Partner Android Build downloading.

    Attributes:
        BAD_XSRF_CODE: int, error code for bad XSRF token error
        BASE_URL: string, path to PAB entry point
        BUILDARTIFACT_NAME_KEY: string, index in artifact containing name
        BUILD_BUILDID_KEY: string, index in build containing build_id
        BUILD_COMPLETED_STATUS: int, value of 'complete' build
        BUILD_STATUS_KEY: string, index in build object containing status.
        CHROME_DRIVER_LOCATION: string, path to chromedriver
        CHROME_LOCATION: string, path to Chrome browser
        CLIENT_STORAGE: string, path to store credentials.
        DEFAULT_CHUNK_SIZE: int, number of bytes to download at a time.
        DOWNLOAD_URL_KEY: string, index in downloadBuildArtifact containing url
        EMAIL: string, email constant for userinfo JSON
        EXPIRED_XSRF_CODE: int, error code for expired XSRF token error
        GETBUILD_ARTIFACTS_KEY, string, index in build obj containing artifacts
        GMS_DOWNLOAD_URL: string, base url for downloading artifacts.
        LISTBUILD_BUILD_KEY: string, index in listBuild containing builds
        PAB_URL: string, redirect url from Google sign-in to PAB
        PASSWORD: string, password constant for userinfo JSON
        SCOPE: string, URL for which to request access via oauth2.
        SVC_URL: string, path to buildsvc RPC
        XSRF_STORE: string, path to store xsrf token
        _credentials : oauth2client credentials object
        _userinfo_file: location of file containing email and password
        _xsrf : string, XSRF token from PAB website. expires after 7 days.
    """
    _credentials = None
    _userinfo_file = None
    _xsrf = None
    BAD_XSRF_CODE = -32000
    BASE_URL = 'https://partner.android.com'
    BUILDARTIFACT_NAME_KEY = '1'
    BUILD_BUILDID_KEY = '1'
    BUILD_COMPLETED_STATUS = 7
    BUILD_STATUS_KEY = '7'
    CHROME_DRIVER_LOCATION = '/usr/local/bin/chromedriver'
    CHROME_LOCATION = '/usr/bin/google-chrome'
    CLIENT_SECRETS = os.path.join(
        os.path.dirname(__file__), 'client_secrets.json')
    CLIENT_STORAGE = os.path.join(os.path.dirname(__file__), 'credentials')
    DEFAULT_CHUNK_SIZE = 1024
    DOWNLOAD_URL_KEY = '1'
    EMAIL = 'email'
    EXPIRED_XSRF_CODE = -32001
    GETBUILD_ARTIFACTS_KEY = '2'
    GMS_DOWNLOAD_URL = 'https://partnerdash.google.com/build/gmsdownload'
    LISTBUILD_BUILD_KEY = '1'
    PAB_URL = ('https://www.google.com/accounts/Login?&continue='
               'https://partner.android.com/build/')
    PASSWORD = 'password'
    # need both of these scopes to access PAB downloader
    scopes = ('https://www.googleapis.com/auth/partnerdash',
              'https://www.googleapis.com/auth/alkali-base')
    SCOPE = ' '.join(scopes)
    SVC_URL = urlparse.urljoin(BASE_URL, 'build/u/0/_gwt/_rpc/buildsvc')
    XSRF_STORE = os.path.join(os.path.dirname(__file__), 'xsrf')

    def __init__(self):
        """Creates a temp dir."""
        super(BuildProviderPAB, self).__init__()

    def Authenticate(self, userinfo_file=None, noauth_local_webserver=False):
        """Authenticate using OAuth2.

        Args:
            userinfo_file: (optional) the path of a JSON file which has
                           "email" and "password" string fields.
            noauth_local_webserver: boolean, True if do not (or can not) use
                                    a local web server.
        """
        # this should be a JSON file with "email" and "password" string fields
        self._userinfo_file = userinfo_file
        logging.info('Parsing flags, use --noauth_local_webserver'
                     ' if running on remote machine')

        parser = argparse.ArgumentParser(parents=[argparser])
        flags, unknown = parser.parse_known_args()
        flags.noauth_local_webserver = noauth_local_webserver
        logging.info('Preparing OAuth token')
        flow = flow_from_clientsecrets(self.CLIENT_SECRETS, scope=self.SCOPE)
        storage = Storage(self.CLIENT_STORAGE)
        if self._credentials is None:
            self._credentials = storage.get()
        if self._credentials is None or self._credentials.invalid:
            logging.info('Credentials not found, authenticating.')
            self._credentials = run_flow(flow, storage, flags)

        if self._credentials.access_token_expired:
            logging.info('Access token expired, refreshing.')
            self._credentials.refresh(http=httplib2.Http())

        if self.XSRF_STORE is not None and os.path.isfile(self.XSRF_STORE):
            with open(self.XSRF_STORE, 'r') as handle:
                self._xsrf = handle.read()

    def GetXSRFToken(self, email=None, password=None):
        """Get XSRF token. Prompt if email/password not provided.

        Args:
            email: string, optional. Gmail account of user logging in
            password: string, optional. Password of user logging in

        Returns:
            boolean, whether the token was accessed and stored
        """
        if self._userinfo_file is not None:
            with open(self._userinfo_file, 'r') as handle:
                userinfo = json.load(handle)

            if self.EMAIL not in userinfo or self.PASSWORD not in userinfo:
                raise ValueError(
                    'Malformed userinfo file: needs email and password')

            email = userinfo[self.EMAIL]
            password = userinfo[self.PASSWORD]

        chrome_options = Options()
        chrome_options.add_argument("--headless")

        driver = webdriver.Chrome(
            chrome_options=chrome_options)

        driver.set_window_size(1080, 800)
        wait = WebDriverWait(driver, 10)

        driver.get(self.PAB_URL)

        query = driver.find_element_by_id("identifierId")
        if email is None:
            email = raw_input("Email: ")
        query.send_keys(email)
        driver.find_element_by_id("identifierNext").click()

        pw = wait.until(EC.element_to_be_clickable((By.NAME, "password")))
        pw.clear()

        if password is None:
            pw.send_keys(getpass.getpass("Password: "))
        else:
            pw.send_keys(password)

        driver.find_element_by_id("passwordNext").click()

        try:
            wait.until(EC.title_contains("Partner Android Build"))
        except TimeoutException as e:
            logging.exception(e)
            raise ValueError('Wrong password or non-standard login flow')

        self._xsrf = driver.execute_script("return clientConfig.XSRF_TOKEN;")
        with open(self.XSRF_STORE, 'w') as handle:
            handle.write(self._xsrf)

        return True

    def CallBuildsvc(self, method, params, account_id):
        """Call the buildsvc RPC with given parameters.

        Args:
            method: string, name of method to be called in buildsvc
            params: dict, parameters to RPC call
            account_id: int, ID associated with the PAB account.

        Returns:
            dict, result from RPC call
        """
        if self._xsrf is None:
            self.GetXSRFToken()
        params = json.dumps(params)

        data = {"method": method, "params": params, "xsrf": self._xsrf}
        data = json.dumps(data)
        headers = {}
        self._credentials.apply(headers)
        headers['Content-Type'] = 'application/json'
        headers['x-alkali-account'] = account_id

        response = requests.post(self.SVC_URL, data=data, headers=headers)

        responseJSON = {}

        try:
            responseJSON = response.json()
        except ValueError:
            raise ValueError("Backend error -- check your account ID")

        if 'result' in responseJSON:
            return responseJSON['result']

        if 'error' in responseJSON and 'code' in responseJSON['error']:
            if responseJSON['error']['code'] == self.BAD_XSRF_CODE:
                raise ValueError(
                    "Bad XSRF token -- must be for the same account as your credentials")
            if responseJSON['error']['code'] == self.EXPIRED_XSRF_CODE:
                raise ValueError("Expired XSRF token -- please refresh")

        raise ValueError("Unknown response from server -- %s" %
                         json.dumps(responseJSON))

    def GetBuildList(self,
                     account_id,
                     branch,
                     target,
                     page_token="",
                     max_results=10,
                     internal=True,
                     method=GET):
        """Get the list of builds for a given account, branch and target
        Args:
            account_id: int, ID associated with the PAB account.
            branch: string, branch to pull resource from.
            target: string, "latest" or a specific version.
            page_token: string, token used for pagination
            max_results: maximum build results the build list contains, e.g. 25
            internal: bool, whether to query internal build
            method: 'GET' or 'POST', which endpoint to query

        Returns:
            list of dicts representing the builds, descending in time
        """
        if method == POST:
            params = {
                "1": branch,
                "2": target,
                "3": page_token,
                "4": max_results,
                "7": int(internal)
            }

            result = self.CallBuildsvc("listBuild", params, account_id)
            # in listBuild response, index '1' contains builds
            if self.LISTBUILD_BUILD_KEY in result:
                return result[self.LISTBUILD_BUILD_KEY]
            raise ValueError("Build list not found -- %s" % params)
        elif method == GET:
            headers = {}
            self._credentials.apply(headers)

            action = 'list-internal' if internal else 'list'
            # PAB URL format expects something (anything) to be given as buildid
            # and resource, even for action list
            dummy = 'DUMMY'
            url = path_urljoin(self.BASE_URL, 'build', 'builds', action,
                               branch, target, dummy,
                               dummy) + '?a=' + str(account_id)

            response = requests.get(url, headers=headers)
            try:
                responseJSON = response.json()
                return responseJSON['build']
            except ValueError as e:
                logging.exception(e)
                raise ValueError("Backend error -- check your account ID")

    def GetLatestBuildId(self, account_id, branch, target, method=GET):
        """Get the most recent build_id for a given account, branch and target
        Args:
            account_id: int, ID associated with the PAB account.
            branch: string, branch to pull resource from.
            target: string, "latest" or a specific version.
            method: 'GET' or 'POST', which endpoint to query

        Returns:
            string, most recent build id
        """
        # TODO: support pagination, maybe?
        build_list = self.GetBuildList(account_id=account_id,
                                       branch=branch,
                                       target=target,
                                       method=method)
        if len(build_list) == 0:
            raise ValueError(
                'No builds found for account_id=%s, branch=%s, target=%s' %
                (account_id, branch, target))
        for build in build_list:
            if method == POST:
                # get build status: 7 = completed build
                if build.get(self.BUILD_STATUS_KEY,
                             None) == self.BUILD_COMPLETED_STATUS:
                    # return build id (index '1')
                    return build[self.BUILD_BUILDID_KEY]
            elif method == GET:
                if build['build_attempt_status'] == "COMPLETE" and build[
                        "successful"]:
                    return build['build_id']
        raise ValueError(
            'No complete builds found: %s failed or incomplete builds found' %
            len(build_list))

    def GetBuildArtifacts(
            self, account_id, build_id, branch, target, method=POST):
        """Get the list of build artifacts.

        For an account, build, target, branch.

        Args:
            account_id: int, ID associated with the PAB account.
            build_id: string, ID of the build
            branch: string, branch to pull resource from.
            target: string, "latest" or a specific version.
            method: 'GET' or 'POST', which endpoint to query

        Returns:
            list of build artifact objects
        """
        if method == GET:
            raise NotImplementedError(
                "GetBuildArtifacts not supported with GET")
        params = {"1": build_id, "2": target, "3": branch}

        result = self.CallBuildsvc("getBuild", params, account_id)
        # in getBuild response, index '2' contains the artifacts
        if self.GETBUILD_ARTIFACTS_KEY in result:
            return result[self.GETBUILD_ARTIFACTS_KEY]
        if len(result) == 0:
            raise ValueError("Build artifacts not found -- %s" % params)

    def GetArtifactURL(self,
                       account_id,
                       build_id,
                       target,
                       artifact_name,
                       branch,
                       internal,
                       method=GET):
        """Get the URL for an artifact on the PAB server, using buildsvc.

        Args:
            account_id: int, ID associated with the PAB account.
            build_id: string/int, id of the build.
            target: string, "latest" or a specific version.
            artifact_name: string, simple file name (no parent dir or path).
            branch: string, branch to pull resource from.
            internal: int, whether the request is for an internal build artifact
            method: 'GET' or 'POST', which endpoint to query

        Returns:
            string, The URL for the resource specified by the parameters
        """
        if method == POST:
            params = {
                "1": str(build_id),
                "2": target,
                "3": artifact_name,
                "4": branch,
                "5": "",  # release_candidate_name
                "6": internal
            }

            result = self.CallBuildsvc(method='downloadBuildArtifact',
                                       params=params,
                                       account_id=account_id)

            # in downloadBuildArtifact response, index '1' contains the url
            if self.DOWNLOAD_URL_KEY in result:
                return result[self.DOWNLOAD_URL_KEY]
            if len(result) == 0:
                raise ValueError("Resource not found -- %s" % params)
        elif method == GET:
            headers = {}
            self._credentials.apply(headers)

            action = 'get-internal' if internal else 'get'
            get_url = path_urljoin(self.BASE_URL, 'build', 'builds', action,
                                   branch, target, build_id,
                                   artifact_name) + '?a=' + str(account_id)

            response = requests.get(get_url, headers=headers)
            try:
                responseJSON = response.json()
                return responseJSON['url']
            except ValueError:
                raise ValueError("Backend error -- check your account ID")

    def DownloadArtifact(self, download_url, filename):
        """Get artifact from Partner Android Build server.

        Args:
            download_url: location of resource that we want to download
            filename: where the artifact gets downloaded locally.

        Returns:
            boolean, whether the file was successfully downloaded
        """
        headers = {}
        self._credentials.apply(headers)

        response = requests.get(download_url, headers=headers, stream=True)
        response.raise_for_status()

        logging.info('%s now downloading...', download_url)
        with open(filename, 'wb') as handle:
            for block in response.iter_content(self.DEFAULT_CHUNK_SIZE):
                handle.write(block)
        return True

    def GetArtifact(self,
                    account_id,
                    branch,
                    target,
                    artifact_name,
                    build_id='latest',
                    method=GET):
        """Get an artifact for an account, branch, target and name and build id.

        If build_id not given, get latest.

        Args:
            account_id: int, ID associated with the PAB account.
            branch: string, branch to pull resource from.
            target: string, "latest" or a specific version.
            artifact_name: name of artifact, e.g. aosp_arm64_ab-img-4353141.zip
                ({id} will automatically get replaced with build ID)
            build_id: string, build ID of an artifact to fetch (or 'latest').
            method: 'GET' or 'POST', which endpoint to query.

        Returns:
            a dict containing the device image info.
            a dict containing the test suite package info.
            a dict containing the artifact info.
            a dict containing the global config info.
        """
        artifact_info = {}
        if build_id == 'latest':
            build_id = self.GetLatestBuildId(account_id=account_id,
                                             branch=branch,
                                             target=target,
                                             method=method)
            print("latest build ID = %s" % build_id)
        artifact_info["build_id"] = build_id

        if "build_id" in artifact_name:
            artifact_name = artifact_name.format(build_id=build_id)

        if method == POST:
            artifacts = self.GetBuildArtifacts(account_id=account_id,
                                               build_id=build_id,
                                               branch=branch,
                                               target=target,
                                               method=method)

            if len(artifacts) == 0:
                raise ValueError(
                    "No artifacts found for build_id=%s, branch=%s, target=%s"
                    % (build_id, branch, target))

            # in build artifact response, index '1' contains the name
            artifact_names = [
                artifact[self.BUILDARTIFACT_NAME_KEY] for artifact in artifacts
            ]
            if artifact_name not in artifact_names:
                raise ValueError("%s not found in artifact list" %
                                 artifact_name)

        url = self.GetArtifactURL(account_id=account_id,
                                  build_id=build_id,
                                  target=target,
                                  artifact_name=artifact_name,
                                  branch=branch,
                                  internal=False,
                                  method=method)

        if self.tmp_dirpath:
            artifact_path = os.path.join(self.tmp_dirpath, artifact_name)
        else:
            artifact_path = artifact_name
        self.DownloadArtifact(url, artifact_path)

        self.SetFetchedFile(artifact_path)

        return (self.GetDeviceImage(), self.GetTestSuitePackage(),
                artifact_info, self.GetConfigPackage())
