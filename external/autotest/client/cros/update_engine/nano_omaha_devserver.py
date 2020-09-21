# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import BaseHTTPServer
import thread
import urlparse

def _split_url(url):
    """Splits a URL into the URL base and path."""
    split_url = urlparse.urlsplit(url)
    url_base = urlparse.urlunsplit(
            (split_url.scheme, split_url.netloc, '', '', ''))
    url_path = split_url.path
    return url_base, url_path.lstrip('/')


class NanoOmahaDevserver(object):
    """A simple Omaha instance that can be setup on a DUT in client tests."""

    def __init__(self, eol=False):
        """
        Create a nano omaha devserver.

        @param eol: True if we should return a response with _eol flag

        """
        self._eol = eol


    class Handler(BaseHTTPServer.BaseHTTPRequestHandler):
        """Inner class for handling HTTP requests."""

        _OMAHA_RESPONSE_TEMPLATE_HEAD = """
          <response protocol=\"3.0\">
            <daystart elapsed_seconds=\"44801\"/>
            <app appid=\"{87efface-864d-49a5-9bb3-4b050a7c227a}\" status=\"ok\">
              <ping status=\"ok\"/>
              <updatecheck status=\"ok\">
                <urls>
                  <url codebase=\"%s\"/>
                </urls>
                <manifest version=\"9999.0.0\">
                  <packages>
                    <package name=\"%s\" size=\"%d\" required=\"true\"/>
                  </packages>
                  <actions>
                    <action event=\"postinstall\"
              ChromeOSVersion=\"9999.0.0\"
              sha256=\"%s\"
              needsadmin=\"false\"
              IsDeltaPayload=\"%s\"
        """

        _OMAHA_RESPONSE_TEMPLATE_TAIL = """ />
                  </actions>
                </manifest>
              </updatecheck>
            </app>
          </response>
        """

        _OMAHA_RESPONSE_EOL = """
          <response protocol=\"3.0\">
            <daystart elapsed_seconds=\"44801\"/>
            <app appid=\"{87efface-864d-49a5-9bb3-4b050a7c227a}\" status=\"ok\">
              <ping status=\"ok\"/>
              <updatecheck _eol=\"eol\" status=\"noupdate\"/>
            </app>
          </response>
        """

        def do_POST(self):
            """Handler for POST requests."""
            if self.path == '/update':
                if self.server._devserver._eol:
                    response = self._OMAHA_RESPONSE_EOL
                else:
                    (base, name) = _split_url(self.server._devserver._image_url)
                    response = self._OMAHA_RESPONSE_TEMPLATE_HEAD % (
                            base + '/',
                            name,
                            self.server._devserver._image_size,
                            self.server._devserver._sha256,
                            str(self.server._devserver._is_delta).lower())
                    if self.server._devserver._is_delta:
                        response += '              IsDelta="true"\n'
                    if self.server._devserver._critical:
                        response += '              deadline="now"\n'
                    if self.server._devserver._metadata_size:
                        response += '              MetadataSize="%d"\n' % (
                                self.server._devserver._metadata_size)
                    if self.server._devserver._metadata_signature:
                        response += '              ' \
                                    'MetadataSignatureRsa="%s"\n' % (
                                self.server._devserver._metadata_signature)
                    if self.server._devserver._public_key:
                        response += '              PublicKeyRsa="%s"\n' % (
                                self.server._devserver._public_key)
                    response += self._OMAHA_RESPONSE_TEMPLATE_TAIL
                self.send_response(200)
                self.send_header('Content-Type', 'application/xml')
                self.end_headers()
                self.wfile.write(response)
            else:
                self.send_response(500)

    def start(self):
        """Starts the server."""
        self._httpd = BaseHTTPServer.HTTPServer(('127.0.0.1', 0), self.Handler)
        self._httpd._devserver = self
        # Serve HTTP requests in a dedicated thread.
        thread.start_new_thread(self._httpd.serve_forever, ())
        self._port = self._httpd.socket.getsockname()[1]

    def stop(self):
        """Stops the server."""
        self._httpd.shutdown()

    def get_port(self):
        """Returns the TCP port number the server is listening on."""
        return self._port

    def set_image_params(self, image_url, image_size, sha256,
                         metadata_size=None, metadata_signature=None,
                         public_key=None, is_delta=False, critical=False):
        """Sets the values to return in the Omaha response. Only the
        |image_url|, |image_size| and |sha256| parameters are
        mandatory."""
        self._image_url = image_url
        self._image_size = image_size
        self._sha256 = sha256
        self._metadata_size = metadata_size
        self._metadata_signature = metadata_signature
        self._public_key = public_key
        self._is_delta = is_delta
        self._critical = critical
