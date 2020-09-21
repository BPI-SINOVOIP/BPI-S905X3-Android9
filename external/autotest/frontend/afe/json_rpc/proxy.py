
"""
  Copyright (c) 2007 Jan-Klaas Kollhof

  This file is part of jsonrpc.

  jsonrpc is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this software; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
"""

import os
import socket
import subprocess
import urllib
import urllib2
from autotest_lib.client.common_lib import error as exceptions
from autotest_lib.client.common_lib import global_config

from json import decoder

from json import encoder as json_encoder
json_encoder_class = json_encoder.JSONEncoder


# Try to upgrade to the Django JSON encoder. It uses the standard json encoder
# but can handle DateTime
try:
    # See http://crbug.com/418022 too see why the try except is needed here.
    from django import conf as django_conf
    # The serializers can't be imported if django isn't configured.
    # Using try except here doesn't work, as test_that initializes it's own
    # django environment (setup_django_lite_environment) which raises import
    # errors if the django dbutils have been previously imported, as importing
    # them leaves some state behind.
    # This the variable name must not be undefined or empty string.
    if os.environ.get(django_conf.ENVIRONMENT_VARIABLE, None):
        from django.core.serializers import json as django_encoder
        json_encoder_class = django_encoder.DjangoJSONEncoder
except ImportError:
    pass


class JSONRPCException(Exception):
    pass


class ValidationError(JSONRPCException):
    """Raised when the RPC is malformed."""
    def __init__(self, error, formatted_message):
        """Constructor.

        @param error: a dict of error info like so:
                      {error['name']: 'ErrorKind',
                       error['message']: 'Pithy error description.',
                       error['traceback']: 'Multi-line stack trace'}
        @formatted_message: string representation of this exception.
        """
        self.problem_keys = eval(error['message'])
        self.traceback = error['traceback']
        super(ValidationError, self).__init__(formatted_message)


def BuildException(error):
    """Exception factory.

    Given a dict of error info, determine which subclass of
    JSONRPCException to build and return.  If can't determine the right one,
    just return a JSONRPCException with a pretty-printed error string.

    @param error: a dict of error info like so:
                  {error['name']: 'ErrorKind',
                   error['message']: 'Pithy error description.',
                   error['traceback']: 'Multi-line stack trace'}
    """
    error_message = '%(name)s: %(message)s\n%(traceback)s' % error
    for cls in JSONRPCException.__subclasses__():
        if error['name'] == cls.__name__:
            return cls(error, error_message)
    for cls in (exceptions.CrosDynamicSuiteException.__subclasses__() +
                exceptions.RPCException.__subclasses__()):
        if error['name'] == cls.__name__:
            return cls(error_message)
    return JSONRPCException(error_message)


class ServiceProxy(object):
    def __init__(self, serviceURL, serviceName=None, headers=None):
        """
        @param serviceURL: The URL for the service we're proxying.
        @param serviceName: Name of the REST endpoint to hit.
        @param headers: Extra HTTP headers to include.
        """
        self.__serviceURL = serviceURL
        self.__serviceName = serviceName
        self.__headers = headers or {}

        # TODO(pprabhu) We are reading this config value deep in the stack
        # because we don't want to update all tools with a new command line
        # argument. Once this has been proven to work, flip the switch -- use
        # sso by default, and turn it off internally in the lab via
        # shadow_config.
        self.__use_sso_client = global_config.global_config.get_config_value(
            'CLIENT', 'use_sso_client', type=bool, default=False)


    def __getattr__(self, name):
        if self.__serviceName is not None:
            name = "%s.%s" % (self.__serviceName, name)
        return ServiceProxy(self.__serviceURL, name, self.__headers)

    def __call__(self, *args, **kwargs):
        # Caller can pass in a minimum value of timeout to be used for urlopen
        # call. Otherwise, the default socket timeout will be used.
        min_rpc_timeout = kwargs.pop('min_rpc_timeout', None)
        postdata = json_encoder_class().encode({'method': self.__serviceName,
                                                'params': args + (kwargs,),
                                                'id': 'jsonrpc'})
        url_with_args = self.__serviceURL + '?' + urllib.urlencode({
            'method': self.__serviceName})
        if self.__use_sso_client:
            respdata = _sso_request(url_with_args, self.__headers, postdata,
                                    min_rpc_timeout)
        else:
            respdata = _raw_http_request(url_with_args, self.__headers,
                                         postdata, min_rpc_timeout)

        try:
            resp = decoder.JSONDecoder().decode(respdata)
        except ValueError:
            raise JSONRPCException('Error decoding JSON reponse:\n' + respdata)
        if resp['error'] is not None:
            raise BuildException(resp['error'])
        else:
            return resp['result']


def _raw_http_request(url_with_args, headers, postdata, timeout):
    """Make a raw HTPP request.

    @param url_with_args: url with the GET params formatted.
    @headers: Any extra headers to include in the request.
    @postdata: data for a POST request instead of a GET.
    @timeout: timeout to use (in seconds).

    @returns: the response from the http request.
    """
    request = urllib2.Request(url_with_args, data=postdata, headers=headers)
    default_timeout = socket.getdefaulttimeout()
    if not default_timeout:
        # If default timeout is None, socket will never time out.
        return urllib2.urlopen(request).read()
    else:
        return urllib2.urlopen(
                request,
                timeout=max(timeout, default_timeout),
        ).read()


def _sso_request(url_with_args, headers, postdata, timeout):
    """Make an HTTP request via sso_client.

    @param url_with_args: url with the GET params formatted.
    @headers: Any extra headers to include in the request.
    @postdata: data for a POST request instead of a GET.
    @timeout: timeout to use (in seconds).

    @returns: the response from the http request.
    """
    headers_str = '; '.join(['%s: %s' % (k, v) for k, v in headers.iteritems()])
    cmd = [
        'sso_client',
        '-url', url_with_args,
    ]
    if headers_str:
        cmd += [
                '-header_sep', '";"',
                '-headers', headers_str,
        ]
    if postdata:
        cmd += [
                '-method', 'POST',
                '-data', postdata,
        ]
    if timeout:
        cmd += ['-request_timeout', str(timeout)]
    else:
        # sso_client has a default timeout of 5 seconds. To mimick the raw
        # behaviour of never timing out, we force a large timeout.
        cmd += ['-request_timeout', '3600']

    try:
        return subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        if _sso_creds_error(e.output):
            raise JSONRPCException('RPC blocked by uberproxy. Have your run '
                                   '`prodaccess`')

        raise JSONRPCException(
                'Error (code: %s) retrieving url (%s): %s' %
                (e.returncode, url_with_args, e.output)
        )


def _sso_creds_error(output):
    return 'No user creds available' in output
