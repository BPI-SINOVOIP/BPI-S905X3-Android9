"""Tests for sso_discovery."""

import mock
import unittest
import subprocess

from skylab_venv import sso_discovery


class SsoDiscoveryTest(unittest.TestCase):
  """Test sso_discovery."""

  URL = 'https://TEST_URL.com'

  def setUp(self):
    super(SsoDiscoveryTest, self).setUp()
    patcher = mock.patch.object(subprocess, 'check_output')
    self.mock_run_cmd = patcher.start()
    self.addCleanup(patcher.stop)

  def testSsoRequestWhenFailToRunSsoClientCmd(self):
    """Test sso_request when fail to run the sso_client command."""
    self.mock_run_cmd.side_effect = subprocess.CalledProcessError(
        returncode=1, cmd='test_cmd')

    with self.assertRaises(sso_discovery.BadHttpRequestException):
       sso_discovery.sso_request(self.URL)

  def testSsoRequestWithAcceptEncodingHeader(self):
    """Test sso_request when accept-encoding header is given."""
    self.mock_run_cmd.return_value = '''HTTP/1.1 200 OK
content-type: application/json
\r\n\r\n
{"test": "test"}'''

    sso_discovery.sso_request(self.URL, headers={'accept-encoding': 'test'})

    self.mock_run_cmd.assert_called_with(
        ['sso_client', '--url', self.URL, '-dump_header'])

  def testSsoRequestWhenFailToParseStatusFromHttpResponse(self):
    """Test sso_request when fail to parse status from the http response."""
    self.mock_run_cmd.return_value = 'invalid response'
    with self.assertRaises(sso_discovery.BadHttpResponseException) as e:
      sso_discovery.sso_request(self.URL)

  def testSsoRequestWhenNoBodyContentInHttpResponse(self):
    """Test sso_request when body is missing from the http response."""
    http_response = '''HTTP/1.1 404 Not Found
fake_header: fake_value'''

    self.mock_run_cmd.return_value = http_response
    resp, body = sso_discovery.sso_request(self.URL)

    self.mock_run_cmd.assert_called_with(
        ['sso_client', '--url', self.URL, '-dump_header'])
    self.assertEqual(resp.status, 404)
    self.assertEqual(resp['body'], '')
    self.assertEqual(resp['headers'], http_response)
    self.assertEqual(body, '')

  def testSsoRequestWithSuccessfulHttpResponse(self):
    """Test sso_request with 200 http response."""
    http_response = '''HTTP/1.1 200 OK
content-type: application/json
\r\n\r\n
{"test": "test"}'''

    self.mock_run_cmd.return_value = http_response
    resp, body = sso_discovery.sso_request(self.URL)

    self.mock_run_cmd.assert_called_with(
        ['sso_client', '--url', self.URL, '-dump_header'])
    self.assertEqual(resp.status, 200)
    self.assertEqual(resp['body'], '{"test": "test"}')
    self.assertEqual(resp['headers'],
                     'HTTP/1.1 200 OK\ncontent-type: application/json')
    self.assertEqual(body, '{"test": "test"}')


if __name__ == '__main__':
  unittest.main()
