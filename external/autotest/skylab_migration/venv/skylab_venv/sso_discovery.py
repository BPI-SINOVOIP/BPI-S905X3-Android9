"""Library to make sso_client wrapped HTTP requests to skylab service."""

from apiclient import discovery
import httplib2
import logging
import re
import subprocess


CODE_REGEX = '\d\d\d'
LINE_SEP = '\r\n' # The line separator used in http response.
DEFAULT_FETCH_DEADLINE_SEC = 60


class BadHttpResponseException(Exception):
  """Raise when fail to parse the HTTP response."""
  pass


class BadHttpRequestException(Exception):
  """Raise when the sso_client based http request failed."""
  pass


def retry_if_bad_request(exception):
  """Return True if we should retry, False otherwise"""
  return isinstance(exception, BadHttpRequestException)

def sso_request(url, method='', body='', headers={}, max_redirects=None):
  """Create sso_client wrapped http request.

  @param url: String of the URL to request.
  @param method: The HTTP method to get use, e.g. GET or DELETE. Use POST if
                 there is data, GET otherwise. Default is "". If not specified,
                 GET is the default method for sso_client command.
  @param body: The data used by POST/UPDATE method. String, default is "".
  @param headers: Dict of the request headers, default is {}.
  @param max_redirects: String format of integer representing the maximum
                        redirects. If not specified, sso_client uses 10 as
                        default.

  @returns: The return value is a tuple of (response, content), the first
            being and instance of the 'Response' class, the second being
            a string that contains the response entity body.
  """
  try:
    cmd = ['sso_client', '--url', url, '-dump_header']
    if method:
      cmd.extend(['-method', method])
    if body:
      cmd.extend(['-data', body])
    if headers:
      # Remove the accept-encoding header to disable encoding in order to
      # receive the raw text.
      headers.pop('accept-encoding', None)
      headers_str = ['%s:%s' % (k, v) for k, v in headers.iteritems()]
      headers_str = ';'.join(headers_str)
      if headers_str:
        cmd.extend(['-headers', headers_str])
    if max_redirects:
      cmd.extend(['-max_redirects', max_redirects])

    logging.debug('Sending SSO request: %s', cmd)
    result = subprocess.check_output(cmd)
  except subprocess.CalledProcessError as e:
    error_msg = ('Fail to make the sso_client request to %s.\nError:\n%s' %
                 (url, e.output))
    raise BadHttpRequestException(error_msg)

  result = result.strip()
  if result.find(LINE_SEP+LINE_SEP) == -1:
    (headers, body) = (result, '')
  else:
    [headers, body] = [s.strip() for s in result.split(LINE_SEP+LINE_SEP)]
  status = re.search(CODE_REGEX, headers.split(LINE_SEP)[0])
  if not status:
    raise BadHttpResponseException(
        'Fail to parse the status return code from the HTTP response:\n%s' %
        result)
  status = status.group(0)
  info = {'status': status, 'body': body, 'headers': headers}
  return (httplib2.Response(info), body)


def _new_http(include_sso=True):
  """Creates httplib2 client that adds SSO cookie."""
  http = httplib2.Http(timeout=DEFAULT_FETCH_DEADLINE_SEC)
  if include_sso:
    http.request = sso_request

  return http


def build_service(service_name,
                  version,
                  discovery_service_url=discovery.DISCOVERY_URI,
                  include_sso=True):
  """Construct a service wrapped with SSO credentials as required."""
  logging.debug('Requesting discovery service for url: %s',
               discovery_service_url)
  return discovery.build(
      service_name,
      version,
      http=_new_http(include_sso),
      discoveryServiceUrl=discovery_service_url)

