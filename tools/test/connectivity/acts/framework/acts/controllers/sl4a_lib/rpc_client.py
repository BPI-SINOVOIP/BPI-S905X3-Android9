import json
import threading

import time
from concurrent import futures

from acts import logger

SOCKET_TIMEOUT = 60

# The Session UID when a UID has not been received yet.
UNKNOWN_UID = -1


class Sl4aException(Exception):
    """The base class for all SL4A exceptions."""


class Sl4aStartError(Sl4aException):
    """Raised when sl4a is not able to be started."""


class Sl4aApiError(Sl4aException):
    """Raised when remote API reports an error."""


class Sl4aConnectionError(Sl4aException):
    """An error raised upon failure to connect to SL4A."""


class Sl4aProtocolError(Sl4aException):
    """Raised when there an error in exchanging data with server on device."""
    NO_RESPONSE_FROM_HANDSHAKE = 'No response from handshake.'
    NO_RESPONSE_FROM_SERVER = 'No response from server.'
    MISMATCHED_API_ID = 'Mismatched API id.'


class MissingSl4AError(Sl4aException):
    """An error raised when an Sl4aClient is created without SL4A installed."""


class RpcClient(object):
    """An RPC client capable of processing multiple RPCs concurrently.

    Attributes:
        _free_connections: A list of all idle RpcConnections.
        _working_connections: A list of all working RpcConnections.
        _lock: A lock used for accessing critical memory.
        max_connections: The maximum number of RpcConnections at a time.
            Increasing or decreasing the number of max connections does NOT
            modify the thread pool size being used for self.future RPC calls.
        _log: The logger for this RpcClient.
    """
    """The default value for the maximum amount of connections for a client."""
    DEFAULT_MAX_CONNECTION = 15

    class AsyncClient(object):
        """An object that allows RPC calls to be called asynchronously.

        Attributes:
            _rpc_client: The RpcClient to use when making calls.
            _executor: The ThreadPoolExecutor used to keep track of workers
        """

        def __init__(self, rpc_client):
            self._rpc_client = rpc_client
            self._executor = futures.ThreadPoolExecutor(
                max_workers=max(rpc_client.max_connections - 2, 1))

        def rpc(self, name, *args, **kwargs):
            future = self._executor.submit(name, *args, **kwargs)
            return future

        def __getattr__(self, name):
            """Wrapper for python magic to turn method calls into RPC calls."""

            def rpc_call(*args, **kwargs):
                future = self._executor.submit(
                    self._rpc_client.__getattr__(name), *args, **kwargs)
                return future

            return rpc_call

    def __init__(self,
                 uid,
                 serial,
                 on_error_callback,
                 _create_connection_func,
                 max_connections=None):
        """Creates a new RpcClient object.

        Args:
            uid: The session uid this client is a part of.
            serial: The serial of the Android device. Used for logging.
            on_error_callback: A callback for when a connection error is raised.
            _create_connection_func: A reference to the function that creates a
                new session.
            max_connections: The maximum number of connections the RpcClient
                can have.
        """
        self._serial = serial
        self.on_error = on_error_callback
        self._create_connection_func = _create_connection_func
        self._free_connections = [self._create_connection_func(uid)]

        self.uid = self._free_connections[0].uid
        self._lock = threading.Lock()

        def _log_formatter(message):
            """Formats the message to be logged."""
            return '[RPC Service|%s|%s] %s' % (self._serial, self.uid, message)

        self._log = logger.create_logger(_log_formatter)

        self._working_connections = []
        if max_connections is None:
            self.max_connections = RpcClient.DEFAULT_MAX_CONNECTION
        else:
            self.max_connections = max_connections

        self._async_client = RpcClient.AsyncClient(self)
        self.is_alive = True

    def terminate(self):
        """Terminates all connections to the SL4A server."""
        if len(self._working_connections) > 0:
            self._log.warning(
                '%s connections are still active, and waiting on '
                'responses.Closing these connections now.' % len(
                    self._working_connections))
        connections = self._free_connections + self._working_connections
        for connection in connections:
            self._log.debug(
                'Closing connection over ports %s' % connection.ports)
            connection.close()
        self._free_connections = []
        self._working_connections = []
        self.is_alive = False

    def _get_free_connection(self):
        """Returns a free connection to be used for an RPC call.

        This function also adds the client to the working set to prevent
        multiple users from obtaining the same client.
        """
        while True:
            if len(self._free_connections) > 0:
                with self._lock:
                    # Check if another thread grabbed the remaining connection.
                    # while we were waiting for the lock.
                    if len(self._free_connections) == 0:
                        continue
                    client = self._free_connections.pop()
                    self._working_connections.append(client)
                    return client

            client_count = (
                len(self._free_connections) + len(self._working_connections))
            if client_count < self.max_connections:
                with self._lock:
                    client_count = (len(self._free_connections) +
                                    len(self._working_connections))
                    if client_count < self.max_connections:
                        client = self._create_connection_func(self.uid)
                        self._working_connections.append(client)
                        return client
            time.sleep(.01)

    def _release_working_connection(self, connection):
        """Marks a working client as free.

        Args:
            connection: The client to mark as free.
        Raises:
            A ValueError if the client is not a known working connection.
        """
        # We need to keep this code atomic because the client count is based on
        # the length of the free and working connection list lengths.
        with self._lock:
            self._working_connections.remove(connection)
            self._free_connections.append(connection)

    def rpc(self, method, *args, timeout=None, retries=1):
        """Sends an rpc to sl4a.

        Sends an rpc call to sl4a over this RpcClient's corresponding session.

        Args:
            method: str, The name of the method to execute.
            args: any, The args to send to sl4a.
            timeout: The amount of time to wait for a response.
            retries: Misnomer, is actually the number of tries.

        Returns:
            The result of the rpc.

        Raises:
            Sl4aProtocolError: Something went wrong with the sl4a protocol.
            Sl4aApiError: The rpc went through, however executed with errors.
        """
        connection = self._get_free_connection()
        ticket = connection.get_new_ticket()

        if timeout:
            connection.set_timeout(timeout)
        data = {'id': ticket, 'method': method, 'params': args}
        request = json.dumps(data)
        response = ''
        try:
            for i in range(1, retries + 1):
                connection.send_request(request)
                self._log.debug('Sent: %s' % request)

                response = connection.get_response()
                self._log.debug('Received: %s', response)
                if not response:
                    if i < retries:
                        self._log.warning(
                            'No response for RPC method %s on iteration %s',
                            method, i)
                        continue
                    else:
                        self.on_error(connection)
                        raise Sl4aProtocolError(
                            Sl4aProtocolError.NO_RESPONSE_FROM_SERVER)
                else:
                    break
        except BrokenPipeError as e:
            if self.is_alive:
                self._log.error('Exception %s happened while communicating to '
                                'SL4A.', e)
                self.on_error(connection)
            else:
                self._log.warning('The connection was killed during cleanup:')
                self._log.warning(e)
            raise Sl4aConnectionError(e)
        finally:
            if timeout:
                connection.set_timeout(SOCKET_TIMEOUT)
            self._release_working_connection(connection)
        result = json.loads(str(response, encoding='utf8'))

        if result['error']:
            err_msg = 'RPC call %s to device failed with error %s' % (
                method, result['error'])
            self._log.error(err_msg)
            raise Sl4aApiError(err_msg)
        if result['id'] != ticket:
            self._log.error('RPC method %s with mismatched api id %s', method,
                            result['id'])
            raise Sl4aProtocolError(Sl4aProtocolError.MISMATCHED_API_ID)
        return result['result']

    @property
    def future(self):
        """Returns a magic function that returns a future running an RPC call.

        This function effectively allows the idiom:

        >>> rpc_client = RpcClient(...)
        >>> # returns after call finishes
        >>> rpc_client.someRpcCall()
        >>> # Immediately returns a reference to the RPC's future, running
        >>> # the lengthy RPC call on another thread.
        >>> future = rpc_client.future.someLengthyRpcCall()
        >>> rpc_client.doOtherThings()
        >>> ...
        >>> # Wait for and get the returned value of the lengthy RPC.
        >>> # Can specify a timeout as well.
        >>> value = future.result()

        The number of concurrent calls to this method is limited to
        (max_connections - 2), to prevent future calls from exhausting all free
        connections.
        """
        return self._async_client

    def __getattr__(self, name):
        """Wrapper for python magic to turn method calls into RPC calls."""

        def rpc_call(*args, **kwargs):
            return self.rpc(name, *args, **kwargs)

        if not self.is_alive:
            raise Sl4aStartError(
                'This SL4A session has already been terminated. You must '
                'create a new session to continue.')
        return rpc_call
