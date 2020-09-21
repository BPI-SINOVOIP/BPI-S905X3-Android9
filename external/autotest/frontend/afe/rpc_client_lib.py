"""
This module provides utility functions useful when writing clients for the RPC
server.
"""

__author__ = 'showard@google.com (Steve Howard)'

import getpass, os
from json_rpc import proxy
from autotest_lib.client.common_lib import utils


class AuthError(Exception):
    pass

def add_protocol(hostname):
    """Constructs a normalized URL to make RPC calls

    This function exists because our configuration files
    (global_config/shadow_config) include only the hostname of the RPC server to
    hit, not the protocol (http/https).
    To support endpoints that require a specific protocol, we allow the hostname
    to include the protocol prefix, and respect the protocol. If no protocol is
    provided, http is used, viz,

        add_protocol('cautotest') --> 'http://cautotest'
        add_protocol('http://cautotest') --> 'http://cautotest'
        add_protocol('https://cautotest') --> 'https://cautotest'

    @param hostname: hostname or url prefix of the RPC server.
    @returns: A string URL for the RPC server with the protocl prefix.
    """
    if (not hostname.startswith('http://') and
        not hostname.startswith('https://')):
        return 'http://' + hostname
    return hostname


def get_proxy(*args, **kwargs):
    """Use this to access the AFE or TKO RPC interfaces."""
    return proxy.ServiceProxy(*args, **kwargs)


def _base_authorization_headers(username, server):
    """
    Don't call this directly, call authorization_headers().
    This implementation may be overridden by site code.

    @returns A dictionary of authorization headers to pass in to get_proxy().
    """
    if not username:
        if 'AUTOTEST_USER' in os.environ:
            username = os.environ['AUTOTEST_USER']
        else:
            username = getpass.getuser()
    return {'AUTHORIZATION' : username}


authorization_headers = utils.import_site_function(
        __file__, 'autotest_lib.frontend.afe.site_rpc_client_lib',
        'authorization_headers', _base_authorization_headers)
