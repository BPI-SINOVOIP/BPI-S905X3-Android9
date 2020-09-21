#!/usr/bin/python
#
# Copyright Gregory P. Smith, Google Inc 2008
# Released under the GPL v2

"""Tests for server.frontend."""

#pylint: disable=missing-docstring

import os, unittest
import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.test_utils import mock
from autotest_lib.frontend.afe import rpc_client_lib
from autotest_lib.server import frontend

GLOBAL_CONFIG = global_config.global_config


class BaseRpcClientTest(unittest.TestCase):
    def setUp(self):
        self.god = mock.mock_god()
        self.god.mock_up(rpc_client_lib, 'rpc_client_lib')
        self.god.stub_function(utils, 'send_email')
        self._saved_environ = dict(os.environ)
        if 'AUTOTEST_WEB' in os.environ:
            del os.environ['AUTOTEST_WEB']


    def tearDown(self):
        self.god.unstub_all()
        os.environ.clear()
        os.environ.update(self._saved_environ)


class RpcClientTest(BaseRpcClientTest):
    def test_init(self):
        os.environ['LOGNAME'] = 'unittest-user'
        GLOBAL_CONFIG.override_config_value('SERVER', 'hostname', 'test-host')
        rpc_client_lib.add_protocol.expect_call('test-host').and_return(
                'http://test-host')
        rpc_client_lib.get_proxy.expect_call(
                'http://test-host/path',
                headers={'AUTHORIZATION': 'unittest-user'})
        frontend.RpcClient('/path', None, None, None, None, None)
        self.god.check_playback()


if __name__ == '__main__':
    unittest.main()
