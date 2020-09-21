# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from contextlib import contextmanager

import logging
import multiprocessing
import os
import shutil
import tempfile
import SocketServer
import SimpleHTTPServer

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.site_utils.lxc import constants


@contextmanager
def serve_locally(file_path):
    """Starts an http server on the localhost, to serve the given file.

    Copies the given file to a temporary location, then starts a local http
    server to serve that file.  Intended for use with unit tests that require
    some URL to download a file (see testInstallSsp as an example).

    @param file_path: The path of the file to serve.

    @return The URL at which the file will be served.
    """
    p = None
    try:
        # Copy the target file into a tmpdir for serving.
        tmpdir = tempfile.mkdtemp()
        shutil.copy(file_path, tmpdir)

        httpd = SocketServer.TCPServer(
                ('', 0), SimpleHTTPServer.SimpleHTTPRequestHandler)
        port = httpd.socket.getsockname()[1]

        # Start the http daemon in the tmpdir to serve the file.
        def serve():
            """Serves the tmpdir."""
            os.chdir(tmpdir)
            httpd.serve_forever()
        p = multiprocessing.Process(target=serve)
        p.daemon = True
        p.start()

        utils.poll_for_condition(
                condition=lambda: http_up(port),
                timeout=constants.NETWORK_INIT_TIMEOUT,
                sleep_interval=constants.NETWORK_INIT_CHECK_INTERVAL)
        url = 'http://127.0.0.1:{port}/{filename}'.format(
                port=port, filename=os.path.basename(file_path))
        logging.debug('Serving %s as %s', file_path, url)
        yield url
    finally:
        if p is not None:
            p.terminate()
        shutil.rmtree(tmpdir)


def http_up(port):
    """Checks for an http server on localhost:port.

    @param port: The port to check.
    """
    try:
        utils.run('curl --head http://127.0.0.1:%d' % port)
        return True
    except error.CmdError:
        return False
