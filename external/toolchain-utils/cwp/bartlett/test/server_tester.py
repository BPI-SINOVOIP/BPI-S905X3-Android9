# Copyright 2012 Google Inc. All Rights Reserved.
# Author: mrdmnd@ (Matt Redmond)
"""A unit test for sending data to Bartlett. Requires poster module."""

import cookielib
import os
import signal
import subprocess
import tempfile
import time
import unittest
import urllib2

from poster.encode import multipart_encode
from poster.streaminghttp import register_openers

SERVER_DIR = '../.'
SERVER_URL = 'http://localhost:8080/'
GET = '_ah/login?email=googler@google.com&action=Login&continue=%s'
AUTH_URL = SERVER_URL + GET


class ServerTest(unittest.TestCase):
  """A unit test for the bartlett server. Tests upload, serve, and delete."""

  def setUp(self):
    """Instantiate the files and server needed to test upload functionality."""
    self._server_proc = LaunchLocalServer()
    self._jar = cookielib.LWPCookieJar()
    self.opener = urllib2.build_opener(urllib2.HTTPCookieProcessor(self._jar))

    # We need these files to not delete when closed, because we have to reopen
    # them in read mode after we write and close them.
    self.profile_data = tempfile.NamedTemporaryFile(delete=False)

    size = 16 * 1024
    self.profile_data.write(os.urandom(size))

  def tearDown(self):
    self.profile_data.close()
    os.remove(self.profile_data.name)
    os.kill(self._server_proc.pid, signal.SIGINT)

  def testIntegration(self):  # pylint: disable-msg=C6409
    key = self._testUpload()
    self._testListAll()
    self._testServeKey(key)
    self._testDelKey(key)

  def _testUpload(self):  # pylint: disable-msg=C6409
    register_openers()
    data = {'profile_data': self.profile_data,
            'board': 'x86-zgb',
            'chromeos_version': '2409.0.2012_06_08_1114'}
    datagen, headers = multipart_encode(data)
    request = urllib2.Request(SERVER_URL + 'upload', datagen, headers)
    response = urllib2.urlopen(request).read()
    self.assertTrue(response)
    return response

  def _testListAll(self):  # pylint: disable-msg=C6409
    request = urllib2.Request(AUTH_URL % (SERVER_URL + 'serve'))
    response = self.opener.open(request).read()
    self.assertTrue(response)

  def _testServeKey(self, key):  # pylint: disable-msg=C6409
    request = urllib2.Request(AUTH_URL % (SERVER_URL + 'serve/' + key))
    response = self.opener.open(request).read()
    self.assertTrue(response)

  def _testDelKey(self, key):  # pylint: disable-msg=C6409
    # There is no response to a delete request.
    # We will check the listAll page to ensure there is no data.
    request = urllib2.Request(AUTH_URL % (SERVER_URL + 'del/' + key))
    response = self.opener.open(request).read()
    request = urllib2.Request(AUTH_URL % (SERVER_URL + 'serve'))
    response = self.opener.open(request).read()
    self.assertFalse(response)


def LaunchLocalServer():
  """Launch and store an authentication cookie with a local server."""
  proc = subprocess.Popen(
      ['dev_appserver.py', '--clear_datastore', SERVER_DIR],
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE)
  # Wait for server to come up
  while True:
    time.sleep(1)
    try:
      request = urllib2.Request(SERVER_URL + 'serve')
      response = urllib2.urlopen(request).read()
      if response:
        break
    except urllib2.URLError:
      continue
  return proc


if __name__ == '__main__':
  unittest.main()
