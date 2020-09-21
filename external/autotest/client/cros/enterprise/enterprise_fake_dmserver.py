# Copyright (c) 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
import urllib2
from multiprocessing import Process

from autotest_lib.client.bin import utils

policy_testserver = None


class FakeDMServer(object):
    """Utility class for policy tests."""

    def __init__(self, proto_path):
        """
        Import the DM testserver from chrome source.

        @param proto_path: location of proto files.

        """
        self.server_url = None
        telemetry_src = '/usr/local/telemetry/src'
        for path in ['chrome/browser/policy/test',
                     'net/tools/testserver',
                     'third_party/protobuf/python/google',
                     'third_party/tlslite']:
            sys.path.append(os.path.join(telemetry_src, path))
        sys.path.append(proto_path)
        global policy_testserver
        import policy_testserver

    def start(self, tmpdir, debugdir):
        """
        Start the local DM testserver.

        @param tmpdir: location of the Autotest tmp dir.
        @param debugdir: location of the Autotest debug directory.

        """
        policy_server_runner = policy_testserver.PolicyServerRunner()
        self._policy_location = os.path.join(tmpdir, 'policy.json')
        port = utils.get_unused_port()
        # The first argument is always ignored since it is expected to be the
        # path to the executable. Hence passing an empty string for first
        # argument.
        sys.argv = ['',
                    '--config-file=%s' % self._policy_location,
                    '--host=127.0.0.1',
                    '--log-file=%s/dm_server.log' % debugdir,
                    '--log-level=DEBUG',
                    '--port=%d' % port

                   ]
        self.process = Process(target=policy_server_runner.main)
        self.process.start()
        self.server_url = 'http://127.0.0.1:%d/' % port

    def stop(self):
        """Terminate the fake DM server instance."""
        if urllib2.urlopen('%stest/ping' % self.server_url).getcode() == 200:
            urllib2.urlopen('%sconfiguration/test/exit' % self.server_url)
        if self.process.is_alive():
            self.process.join()

    def setup_policy(self, policy_blob):
        """
        Write policy blob to file used by the DM server to read policy.

        @param policy_blob: JSON policy blob to be written to the policy file.

        """
        with open(self._policy_location, 'w') as f:
            f.write(policy_blob)
