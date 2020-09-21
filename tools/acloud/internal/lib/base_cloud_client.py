#!/usr/bin/env python
#
# Copyright 2016 - The Android Open Source Project
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

"""Base Cloud API Client.

BasicCloudApiCliend does basic setup for a cloud API.
"""
import httplib
import logging
import os
import socket
import ssl

from apiclient import errors as gerrors
from apiclient.discovery import build
import apiclient.http
import httplib2
from oauth2client import client

from acloud.internal.lib import utils
from acloud.public import errors

logger = logging.getLogger(__name__)


class BaseCloudApiClient(object):
    """A class that does basic setup for a cloud API."""

    # To be overriden by subclasses.
    API_NAME = ""
    API_VERSION = "v1"
    SCOPE = ""

    # Defaults for retry.
    RETRY_COUNT = 5
    RETRY_BACKOFF_FACTOR = 1.5
    RETRY_SLEEP_MULTIPLIER = 2
    RETRY_HTTP_CODES = [
        # 403 is to retry the "Rate Limit Exceeded" error.
        # We could retry on a finer-grained error message later if necessary.
        403,
        500,  # Internal Server Error
        502,  # Bad Gateway
        503,  # Service Unavailable
    ]
    RETRIABLE_ERRORS = (httplib.HTTPException, httplib2.HttpLib2Error,
                        socket.error, ssl.SSLError)
    RETRIABLE_AUTH_ERRORS = (client.AccessTokenRefreshError, )

    def __init__(self, oauth2_credentials):
        """Initialize.

        Args:
            oauth2_credentials: An oauth2client.OAuth2Credentials instance.
        """
        self._service = self.InitResourceHandle(oauth2_credentials)

    @classmethod
    def InitResourceHandle(cls, oauth2_credentials):
        """Authenticate and initialize a Resource object.

        Authenticate http and create a Resource object with methods
        for interacting with the service.

        Args:
            oauth2_credentials: An oauth2client.OAuth2Credentials instance.

        Returns:
            An apiclient.discovery.Resource object
        """
        http_auth = oauth2_credentials.authorize(httplib2.Http())
        return utils.RetryExceptionType(
                exception_types=cls.RETRIABLE_AUTH_ERRORS,
                max_retries=cls.RETRY_COUNT,
                functor=build,
                sleep_multiplier=cls.RETRY_SLEEP_MULTIPLIER,
                retry_backoff_factor=cls.RETRY_BACKOFF_FACTOR,
                serviceName=cls.API_NAME,
                version=cls.API_VERSION,
                http=http_auth)

    def _ShouldRetry(self, exception, retry_http_codes,
                     other_retriable_errors):
        """Check if exception is retriable.

        Args:
            exception: An instance of Exception.
            retry_http_codes: a list of integers, retriable HTTP codes of
                              HttpError
            other_retriable_errors: a tuple of error types to retry other than
                                    HttpError.

        Returns:
            Boolean, True if retriable, False otherwise.
        """
        if isinstance(exception, other_retriable_errors):
            return True

        if isinstance(exception, errors.HttpError):
            if exception.code in retry_http_codes:
                return True
            else:
                logger.debug("_ShouldRetry: Exception code %s not in %s: %s",
                             exception.code, retry_http_codes, str(exception))

        logger.debug(
            "_ShouldRetry: Exception %s is not one of %s: %s", type(exception),
            list(other_retriable_errors) + [errors.HttpError], str(exception))
        return False

    def _TranslateError(self, exception):
        """Translate the exception to a desired type.

        Args:
            exception: An instance of Exception.

        Returns:
            gerrors.HttpError will be translated to errors.HttpError.
            If the error code is errors.HTTP_NOT_FOUND_CODE, it will
            be translated to errors.ResourceNotFoundError.
            Unrecognized error type will not be translated and will
            be returned as is.
        """
        if isinstance(exception, gerrors.HttpError):
            exception = errors.HttpError.CreateFromHttpError(exception)
            if exception.code == errors.HTTP_NOT_FOUND_CODE:
                exception = errors.ResourceNotFoundError(exception.code,
                                                         str(exception))
        return exception

    def ExecuteOnce(self, api):
        """Execute an api and parse the errors.

        Args:
            api: An apiclient.http.HttpRequest, representing the api to execute.

        Returns:
            Execution result of the api.

        Raises:
            errors.ResourceNotFoundError: For 404 error.
            errors.HttpError: For other types of http error.
        """
        try:
            return api.execute()
        except gerrors.HttpError as e:
            raise self._TranslateError(e)

    def Execute(self,
                api,
                retry_http_codes=None,
                max_retry=None,
                sleep=None,
                backoff_factor=None,
                other_retriable_errors=None):
        """Execute an api with retry.

        Call ExecuteOnce and retry on http error with given codes.

        Args:
            api: An apiclient.http.HttpRequest, representing the api to execute:
            retry_http_codes: A list of http codes to retry.
            max_retry: See utils.Retry.
            sleep: See utils.Retry.
            backoff_factor: See utils.Retry.
            other_retriable_errors: A tuple of error types that should be retried
                                    other than errors.HttpError.

        Returns:
          Execution result of the api.

        Raises:
          See ExecuteOnce.
        """
        retry_http_codes = (self.RETRY_HTTP_CODES if retry_http_codes is None
                            else retry_http_codes)
        max_retry = (self.RETRY_COUNT if max_retry is None else max_retry)
        sleep = (self.RETRY_SLEEP_MULTIPLIER if sleep is None else sleep)
        backoff_factor = (self.RETRY_BACKOFF_FACTOR if backoff_factor is None
                          else backoff_factor)
        other_retriable_errors = (self.RETRIABLE_ERRORS
                                  if other_retriable_errors is None else
                                  other_retriable_errors)

        def _Handler(exc):
            """Check if |exc| is a retriable exception.

            Args:
                exc: An exception.

            Returns:
                True if exc is an errors.HttpError and code exists in |retry_http_codes|
                False otherwise.
            """
            if self._ShouldRetry(exc, retry_http_codes,
                                 other_retriable_errors):
                logger.debug("Will retry error: %s", str(exc))
                return True
            return False

        return utils.Retry(
             _Handler, max_retries=max_retry, functor=self.ExecuteOnce,
             sleep_multiplier=sleep, retry_backoff_factor=backoff_factor,
             api=api)

    def BatchExecuteOnce(self, requests):
        """Execute requests in a batch.

        Args:
            requests: A dictionary where key is request id and value
                      is an http request.

        Returns:
            results, a dictionary in the following format
            {request_id: (response, exception)}
            request_ids are those from requests; response
            is the http response for the request or None on error;
            exception is an instance of DriverError or None if no error.
        """
        results = {}

        def _CallBack(request_id, response, exception):
            results[request_id] = (response, self._TranslateError(exception))

        batch = apiclient.http.BatchHttpRequest()
        for request_id, request in requests.iteritems():
            batch.add(request=request,
                      callback=_CallBack,
                      request_id=request_id)
        batch.execute()
        return results

    def BatchExecute(self,
                     requests,
                     retry_http_codes=None,
                     max_retry=None,
                     sleep=None,
                     backoff_factor=None,
                     other_retriable_errors=None):
        """Batch execute multiple requests with retry.

        Call BatchExecuteOnce and retry on http error with given codes.

        Args:
            requests: A dictionary where key is request id picked by caller,
                      and value is a apiclient.http.HttpRequest.
            retry_http_codes: A list of http codes to retry.
            max_retry: See utils.Retry.
            sleep: See utils.Retry.
            backoff_factor: See utils.Retry.
            other_retriable_errors: A tuple of error types that should be retried
                                    other than errors.HttpError.

        Returns:
            results, a dictionary in the following format
            {request_id: (response, exception)}
            request_ids are those from requests; response
            is the http response for the request or None on error;
            exception is an instance of DriverError or None if no error.
        """
        executor = utils.BatchHttpRequestExecutor(
            self.BatchExecuteOnce,
            requests=requests,
            retry_http_codes=retry_http_codes or self.RETRY_HTTP_CODES,
            max_retry=max_retry or self.RETRY_COUNT,
            sleep=sleep or self.RETRY_SLEEP_MULTIPLIER,
            backoff_factor=backoff_factor or self.RETRY_BACKOFF_FACTOR,
            other_retriable_errors=other_retriable_errors or
            self.RETRIABLE_ERRORS)
        executor.Execute()
        return executor.GetResults()

    def ListWithMultiPages(self, api_resource, *args, **kwargs):
        """Call an api that list a type of resource.

        Multiple google services support listing a type of
        resource (e.g list gce instances, list storage objects).
        The querying pattern is similar --
        Step 1: execute the api and get a response object like,
        {
            "items": [..list of resource..],
            # The continuation token that can be used
            # to get the next page.
            "nextPageToken": "A String",
        }
        Step 2: execute the api again with the nextPageToken to
        retrieve more pages and get a response object.

        Step 3: Repeat Step 2 until no more page.

        This method encapsulates the generic logic of
        calling such listing api.

        Args:
            api_resource: An apiclient.discovery.Resource object
                used to create an http request for the listing api.
            *args: Arguments used to create the http request.
            **kwargs: Keyword based arguments to create the http
                      request.

        Returns:
            A list of items.
        """
        items = []
        next_page_token = None
        while True:
            api = api_resource(pageToken=next_page_token, *args, **kwargs)
            response = self.Execute(api)
            items.extend(response.get("items", []))
            next_page_token = response.get("nextPageToken")
            if not next_page_token:
                break
        return items

    @property
    def service(self):
        """Return self._service as a property."""
        return self._service
