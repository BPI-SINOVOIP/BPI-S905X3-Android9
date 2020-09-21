#
#   Copyright 2018 - The Android Open Source Project
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

import queue
import re
import threading
import time

from acts.test_utils.net import connectivity_const as cconst
from acts import asserts

MSG = "Test message "
PKTS = 5

""" Methods for android.system.Os based sockets """
def open_android_socket(ad, domain, sock_type, ip, port):
    """ Open TCP or UDP using android.system.Os class

    Args:
      1. ad - android device object
      2. domain - IPv4 or IPv6 type
      3. sock_type - UDP or TCP socket
      4. ip - IP addr on the device
      5. port - open socket on port

    Returns:
      File descriptor key
    """
    fd_key = ad.droid.openSocket(domain, sock_type, ip, port)
    ad.log.info("File descriptor: %s" % fd_key)
    asserts.assert_true(fd_key, "Failed to open socket")
    return fd_key

def close_android_socket(ad, fd_key):
    """ Close socket

    Args:
      1. ad - android device object
      2. fd_key - file descriptor key
    """
    status = ad.droid.closeSocket(fd_key)
    asserts.assert_true(status, "Failed to close socket")

def listen_accept_android_socket(client,
                                 server,
                                 client_fd,
                                 server_fd,
                                 server_ip,
                                 server_port):
    """ Listen, accept TCP sockets

    Args:
      1. client : ad object for client device
      2. server : ad object for server device
      3. client_fd : client's socket handle
      4. server_fd : server's socket handle
      5. server_ip : send data to this IP
      6. server_port : send data to this port
    """
    server.droid.listenSocket(server_fd)
    client.droid.connectSocket(client_fd, server_ip, server_port)
    sock = server.droid.acceptSocket(server_fd)
    asserts.assert_true(sock, "Failed to accept socket")
    return sock

def send_recv_data_android_sockets(client,
                                   server,
                                   client_fd,
                                   server_fd,
                                   server_ip,
                                   server_port):
    """ Send TCP or UDP data over android os sockets from client to server.
        Verify that server received the data.

    Args:
      1. client : ad object for client device
      2. server : ad object for server device
      3. client_fd : client's socket handle
      4. server_fd : server's socket handle
      5. server_ip : send data to this IP
      6. server_port : send data to this port
    """
    send_list = []
    recv_list = []

    for _ in range(1, PKTS+1):
        msg = MSG + " %s" % _
        send_list.append(msg)
        client.log.info("Sending message: %s" % msg)
        client.droid.sendDataOverSocket(server_ip, server_port, msg, client_fd)
        recv_msg = server.droid.recvDataOverSocket(server_fd)
        server.log.info("Received message: %s" % recv_msg)
        recv_list.append(recv_msg)

    recv_list = [x.rstrip('\x00') if x else x for x in recv_list]
    asserts.assert_true(send_list and recv_list and send_list == recv_list,
                        "Send and recv information is incorrect")

""" Methods for java.net.DatagramSocket based sockets """
def open_datagram_socket(ad, ip, port):
    """ Open datagram socket

    Args:
      1. ad : android device object
      2. ip : IP addr on the device
      3. port : socket port

    Returns:
      Hash key of the datagram socket
    """
    socket_key = ad.droid.openDatagramSocket(ip, port)
    ad.log.info("Datagram socket: %s" % socket_key)
    asserts.assert_true(socket_key, "Failed to open datagram socket")
    return socket_key

def close_datagram_socket(ad, socket_key):
    """ Close datagram socket

    Args:
      1. socket_key : hash key of datagram socket
    """
    status = ad.droid.closeDatagramSocket(socket_key)
    asserts.assert_true(status, "Failed to close datagram socket")

def send_recv_data_datagram_sockets(client,
                                    server,
                                    client_sock,
                                    server_sock,
                                    server_ip,
                                    server_port):
    """ Send data over datagram socket from dut_a to dut_b.
        Verify that dut_b received the data.

    Args:
      1. client : ad object for client device
      2. server : ad object for server device
      3. client_sock : client's socket handle
      4. server_sock : server's socket handle
      5. server_ip : send data to this IP
      6. server_port : send data to this port
    """
    send_list = []
    recv_list = []

    for _ in range(1, PKTS+1):
        msg = MSG + " %s" % _
        send_list.append(msg)
        client.log.info("Sending message: %s" % msg)
        client.droid.sendDataOverDatagramSocket(client_sock,
                                                msg,
                                                server_ip,
                                                server_port)
        recv_msg = server.droid.recvDataOverDatagramSocket(server_sock)
        server.log.info("Received message: %s" % recv_msg)
        recv_list.append(recv_msg)

    recv_list = [x.rstrip('\x00') if x else x for x in recv_list]
    asserts.assert_true(send_list and recv_list and send_list == recv_list,
                        "Send and recv information is incorrect")

""" Utils methods for java.net.Socket based sockets """
def _accept_socket(server, server_ip, server_port, server_sock, q):
    sock = server.droid.acceptTcpSocket(server_sock)
    server.log.info("Server socket: %s" % sock)
    q.put(sock)

def _client_socket(client, server_ip, server_port, client_ip, client_port, q):
    time.sleep(0.5)
    sock = client.droid.openTcpSocket(server_ip,
                                      server_port,
                                      client_ip,
                                      client_port)
    client.log.info("Client socket: %s" % sock)
    q.put(sock)

def open_connect_socket(client,
                        server,
                        client_ip,
                        server_ip,
                        client_port,
                        server_port,
                        server_sock):
    """ Open tcp socket and connect to server

    Args:
      1. client : ad object for client device
      2. server : ad object for server device
      3. client_ip : client's socket handle
      4. server_ip : send data to this IP
      5. client_port : port on client socket
      6. server_port : port on server socket
      7. server_sock : server socket

    Returns:
      client and server socket from successful connect
    """
    sq = queue.Queue()
    cq = queue.Queue()
    s = threading.Thread(target = _accept_socket,
                         args = (server, server_ip, server_port, server_sock,
                                 sq))
    c = threading.Thread(target = _client_socket,
                         args = (client, server_ip, server_port, client_ip,
                                 client_port, cq))
    s.start()
    c.start()
    c.join()
    s.join()

    client_sock = cq.get()
    server_sock = sq.get()
    asserts.assert_true(client_sock and server_sock, "Failed to open sockets")

    return client_sock, server_sock

def open_server_socket(server, server_ip, server_port):
    """ Open tcp server socket

    Args:
      1. server : ad object for server device
      2. server_ip : send data to this IP
      3. server_port : send data to this port
    """
    sock = server.droid.openTcpServerSocket(server_ip, server_port)
    server.log.info("Server Socket: %s" % sock)
    asserts.assert_true(sock, "Failed to open server socket")
    return sock

def close_socket(ad, socket):
    """ Close socket

    Args:
      1. ad - android device object
      2. socket - socket key
    """
    status = ad.droid.closeTcpSocket(socket)
    asserts.assert_true(status, "Failed to socket")

def close_server_socket(ad, socket):
    """ Close server socket

    Args:
      1. ad - android device object
      2. socket - server socket key
    """
    status = ad.droid.closeTcpServerSocket(socket)
    asserts.assert_true(status, "Failed to socket")

def send_recv_data_sockets(client, server, client_sock, server_sock):
    """ Send data over TCP socket from client to server.
        Verify that server received the data

    Args:
      1. client : ad object for client device
      2. server : ad object for server device
      3. client_sock : client's socket handle
      4. server_sock : server's socket handle
    """
    send_list = []
    recv_list = []

    for _ in range(1, PKTS+1):
        msg = MSG + " %s" % _
        send_list.append(msg)
        client.log.info("Sending message: %s" % msg)
        client.droid.sendDataOverTcpSocket(client_sock, msg)
        recv_msg = server.droid.recvDataOverTcpSocket(server_sock)
        server.log.info("Received message: %s" % recv_msg)
        recv_list.append(recv_msg)

    recv_list = [x.rstrip('\x00') if x else x for x in recv_list]
    asserts.assert_true(send_list and recv_list and send_list == recv_list,
                        "Send and recv information is incorrect")
