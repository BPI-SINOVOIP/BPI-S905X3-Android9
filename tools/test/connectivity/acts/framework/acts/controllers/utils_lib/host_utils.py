#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import socket


def get_available_host_port():
    """Finds a semi-random available port.

    A race condition is still possible after the port number is returned, if
    another process happens to bind it.

    Returns:
        A port number that is unused on both TCP and UDP.
    """
    # On the 2.6 kernel, calling _try_bind() on UDP socket returns the
    # same port over and over. So always try TCP first.
    while True:
        # Ask the OS for an unused port.
        port = _try_bind(0, socket.SOCK_STREAM, socket.IPPROTO_TCP)
        # Check if this port is unused on the other protocol.
        if port and _try_bind(port, socket.SOCK_DGRAM, socket.IPPROTO_UDP):
            return port


def is_port_available(port):
    """Checks if a given port number is available on the system.

    Args:
        port: An integer which is the port number to check.

    Returns:
        True if the port is available; False otherwise.
    """
    return (_try_bind(port, socket.SOCK_STREAM, socket.IPPROTO_TCP) and
            _try_bind(port, socket.SOCK_DGRAM, socket.IPPROTO_UDP))


def _try_bind(port, socket_type, socket_proto):
    s = socket.socket(socket.AF_INET, socket_type, socket_proto)
    try:
        try:
            s.bind(('', port))
            # The result of getsockname() is protocol dependent, but for both
            # IPv4 and IPv6 the second field is a port number.
            return s.getsockname()[1]
        except socket.error:
            return None
    finally:
        s.close()
