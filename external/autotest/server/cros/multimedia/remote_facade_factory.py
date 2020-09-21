# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import httplib
import logging
import os
import re
import socket
import xmlrpclib
import pprint
import sys

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import logging_manager
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.client.cros import constants
from autotest_lib.server import autotest
from autotest_lib.server.cros.multimedia import audio_facade_adapter
from autotest_lib.server.cros.multimedia import bluetooth_hid_facade_adapter
from autotest_lib.server.cros.multimedia import browser_facade_adapter
from autotest_lib.server.cros.multimedia import cfm_facade_adapter
from autotest_lib.server.cros.multimedia import display_facade_adapter
from autotest_lib.server.cros.multimedia import input_facade_adapter
from autotest_lib.server.cros.multimedia import kiosk_facade_adapter
from autotest_lib.server.cros.multimedia import system_facade_adapter
from autotest_lib.server.cros.multimedia import usb_facade_adapter
from autotest_lib.server.cros.multimedia import video_facade_adapter


# Log the client messages in the DEBUG level, with the prefix [client].
CLIENT_LOG_STREAM = logging_manager.LoggingFile(
        level=logging.DEBUG,
        prefix='[client] ')


class _Method:
    """Class to save the name of the RPC method instead of the real object.

    It keeps the name of the RPC method locally first such that the RPC method
    can be evalulated to a real object while it is called. Its purpose is to
    refer to the latest RPC proxy as the original previous-saved RPC proxy may
    be lost due to reboot.

    The call_method is the method which does refer to the latest RPC proxy.
    """

    def __init__(self, call_method, name):
        self.__call_method = call_method
        self.__name = name


    def __getattr__(self, name):
        # Support a nested method.
        return _Method(self.__call_method, "%s.%s" % (self.__name, name))


    def __call__(self, *args, **dargs):
        return self.__call_method(self.__name, *args, **dargs)


class RemoteFacadeProxy(object):
    """An abstraction of XML RPC proxy to the DUT multimedia server.

    The traditional XML RPC server proxy is static. It is lost when DUT
    reboots. This class reconnects the server again when it finds the
    connection is lost.

    """

    XMLRPC_CONNECT_TIMEOUT = 90
    XMLRPC_RETRY_TIMEOUT = 180
    XMLRPC_RETRY_DELAY = 10
    REBOOT_TIMEOUT = 60

    def __init__(self, host, no_chrome, extra_browser_args=None):
        """Construct a RemoteFacadeProxy.

        @param host: Host object representing a remote host.
        @param no_chrome: Don't start Chrome by default.
        @param extra_browser_args: A list containing extra browser args passed
                                   to Chrome in addition to default ones.

        """
        self._client = host
        self._xmlrpc_proxy = None
        self._log_saving_job = None
        self._no_chrome = no_chrome
        self._extra_browser_args = extra_browser_args
        self.connect()
        if not no_chrome:
            self._start_chrome(reconnect=False, retry=True,
                               extra_browser_args=self._extra_browser_args)


    def __getattr__(self, name):
        """Return a _Method object only, not its real object."""
        return _Method(self.__call_proxy, name)


    def __call_proxy(self, name, *args, **dargs):
        """Make the call on the latest RPC proxy object.

        This method gets the internal method of the RPC proxy and calls it.

        @param name: Name of the RPC method, a nested method supported.
        @param args: The rest of arguments.
        @param dargs: The rest of dict-type arguments.
        @return: The return value of the RPC method.
        """
        def process_log():
            """Process the log from client, i.e. showing the log messages."""
            if self._log_saving_job:
                # final_read=True to process all data until the end
                self._log_saving_job.process_output(
                        stdout=True, final_read=True)
                self._log_saving_job.process_output(
                        stdout=False, final_read=True)

        def parse_exception(message):
            """Parse the given message and extract the exception line.

            @return: A tuple of (keyword, reason); or None if not found.
            """
            EXCEPTION_PATTERN = r'(\w+): (.+)'
            # Search the line containing the exception keyword, like:
            #   "TestFail: Not able to start session."
            for line in reversed(message.split('\n')):
                m = re.match(EXCEPTION_PATTERN, line)
                if m:
                    return (m.group(1), m.group(2))
            return None

        def call_rpc_with_log():
            """Call the RPC with log."""
            value = getattr(self._xmlrpc_proxy, name)(*args, **dargs)
            process_log()

            # For debug, print the return value.
            logging.debug('RPC %s returns %s.', rpc, pprint.pformat(value))

            # Raise some well-known client exceptions, like TestFail.
            if type(value) is str and value.startswith('Traceback'):
                exception_tuple = parse_exception(value)
                if exception_tuple:
                    keyword, reason = exception_tuple
                    reason = reason + ' (RPC: %s)' % name
                    if keyword == 'TestFail':
                        raise error.TestFail(reason)
                    elif keyword == 'TestError':
                        raise error.TestError(reason)

                    # Raise the exception with the original exception keyword.
                    raise Exception('%s: %s' % (keyword, reason))

                # Raise the default exception with the original message.
                raise Exception('Exception from client (RPC: %s)\n%s' %
                                (name, value))

            return value

        try:
            # TODO(ihf): This logs all traffic from server to client. Make
            # the spew optional.
            rpc = (
                '%s(%s, %s)' %
                (pprint.pformat(name), pprint.pformat(args),
                 pprint.pformat(dargs)))
            try:
                return call_rpc_with_log()
            except (socket.error,
                    xmlrpclib.ProtocolError,
                    httplib.BadStatusLine):
                # Reconnect the RPC server in case connection lost, e.g. reboot.
                self.connect()
                if not self._no_chrome:
                    self._start_chrome(
                            reconnect=True, retry=False,
                            extra_browser_args=self._extra_browser_args)
                # Try again.
                logging.warning('Retrying RPC %s.', rpc)
                return call_rpc_with_log()
        except:
            # Process the log if any. It is helpful for debug.
            process_log()
            logging.error(
                'Failed RPC %s with status [%s].', rpc, sys.exc_info()[0])
            raise


    def save_log_bg(self):
        """Save the log from client in background."""
        # Run a tail command in background that keeps all the log messages from
        # client.
        command = 'tail -n0 -f %s' % constants.MULTIMEDIA_XMLRPC_SERVER_LOG_FILE
        full_command = '%s "%s"' % (self._client.ssh_command(), command)

        if self._log_saving_job:
            # Kill and join the previous job, probably due to a DUT reboot.
            # In this case, a new job will be recreated.
            logging.info('Kill and join the previous log job.')
            utils.nuke_subprocess(self._log_saving_job.sp)
            utils.join_bg_jobs([self._log_saving_job])

        # Create the background job and pipe its stdout and stderr to the
        # Autotest logging.
        self._log_saving_job = utils.BgJob(full_command,
                                           stdout_tee=CLIENT_LOG_STREAM,
                                           stderr_tee=CLIENT_LOG_STREAM)


    def connect(self):
        """Connects the XML-RPC proxy on the client.

        @return: True on success. Note that if autotest server fails to
                 connect to XMLRPC server on Cros host after timeout,
                 error.TimeoutException will be raised by retry.retry
                 decorator.

        """
        @retry.retry((socket.error,
                      xmlrpclib.ProtocolError,
                      httplib.BadStatusLine),
                      timeout_min=self.XMLRPC_RETRY_TIMEOUT / 60.0,
                      delay_sec=self.XMLRPC_RETRY_DELAY)
        def connect_with_retries():
            """Connects the XML-RPC proxy with retries."""
            self._xmlrpc_proxy = self._client.rpc_server_tracker.xmlrpc_connect(
                    constants.MULTIMEDIA_XMLRPC_SERVER_COMMAND,
                    constants.MULTIMEDIA_XMLRPC_SERVER_PORT,
                    command_name=(
                        constants.MULTIMEDIA_XMLRPC_SERVER_CLEANUP_PATTERN
                    ),
                    ready_test_name=(
                        constants.MULTIMEDIA_XMLRPC_SERVER_READY_METHOD),
                    timeout_seconds=self.XMLRPC_CONNECT_TIMEOUT,
                    logfile=constants.MULTIMEDIA_XMLRPC_SERVER_LOG_FILE,
                    request_timeout_seconds=
                            constants.MULTIMEDIA_XMLRPC_SERVER_REQUEST_TIMEOUT)

        logging.info('Setup the connection to RPC server, with retries...')
        connect_with_retries()

        logging.info('Start a job to save the log from the client.')
        self.save_log_bg()

        return True


    def _start_chrome(self, reconnect, retry=False, extra_browser_args=None):
        """Starts Chrome using browser facade on Cros host.

        @param reconnect: True for reconnection, False for the first-time.
        @param retry: True to retry using a reboot on host.
        @param extra_browser_args: A list containing extra browser args passed
                                   to Chrome in addition to default ones.

        @raise: error.TestError: if fail to start Chrome after retry.

        """
        logging.info(
                'Start Chrome with default arguments and extra browser args %s...',
                extra_browser_args)
        success = self._xmlrpc_proxy.browser.start_default_chrome(
                reconnect, extra_browser_args)
        if not success and retry:
            logging.warning('Can not start Chrome. Reboot host and try again')
            # Reboot host and try again.
            self._client.reboot()
            # Wait until XMLRPC server can be reconnected.
            utils.poll_for_condition(condition=self.connect,
                                     timeout=self.REBOOT_TIMEOUT)
            logging.info(
                    'Retry starting Chrome with default arguments and '
                    'extra browser args %s...', extra_browser_args)
            success = self._xmlrpc_proxy.browser.start_default_chrome(
                    reconnect, extra_browser_args)

        if not success:
            raise error.TestError(
                    'Failed to start Chrome on DUT. '
                    'Check multimedia_xmlrpc_server.log in result folder.')


    def __del__(self):
        """Destructor of RemoteFacadeFactory."""
        self._client.rpc_server_tracker.disconnect(
                constants.MULTIMEDIA_XMLRPC_SERVER_PORT)


class RemoteFacadeFactory(object):
    """A factory to generate remote multimedia facades.

    The facade objects are remote-wrappers to access the DUT multimedia
    functionality, like display, video, and audio.

    """

    def __init__(self, host, no_chrome=False, install_autotest=True,
                 results_dir=None, extra_browser_args=None):
        """Construct a RemoteFacadeFactory.

        @param host: Host object representing a remote host.
        @param no_chrome: Don't start Chrome by default.
        @param install_autotest: Install autotest on host.
        @param results_dir: A directory to store multimedia server init log.
        @param extra_browser_args: A list containing extra browser args passed
                                   to Chrome in addition to default ones.
        If it is not None, we will get multimedia init log to the results_dir.

        """
        self._client = host
        if install_autotest:
            # Make sure the client library is on the device so that
            # the proxy code is there when we try to call it.
            client_at = autotest.Autotest(self._client)
            client_at.install()
        try:
            self._proxy = RemoteFacadeProxy(
                    host=self._client,
                    no_chrome=no_chrome,
                    extra_browser_args=extra_browser_args)
        finally:
            if results_dir:
                host.get_file(constants.MULTIMEDIA_XMLRPC_SERVER_LOG_FILE,
                              os.path.join(results_dir,
                                           'multimedia_xmlrpc_server.log.init'))


    def ready(self):
        """Returns the proxy ready status"""
        return self._proxy.ready()


    def create_audio_facade(self):
        """Creates an audio facade object."""
        return audio_facade_adapter.AudioFacadeRemoteAdapter(
                self._client, self._proxy)


    def create_video_facade(self):
        """Creates a video facade object."""
        return video_facade_adapter.VideoFacadeRemoteAdapter(
                self._client, self._proxy)


    def create_display_facade(self):
        """Creates a display facade object."""
        return display_facade_adapter.DisplayFacadeRemoteAdapter(
                self._client, self._proxy)


    def create_system_facade(self):
        """Creates a system facade object."""
        return system_facade_adapter.SystemFacadeRemoteAdapter(
                self._client, self._proxy)


    def create_usb_facade(self):
        """"Creates a USB facade object."""
        return usb_facade_adapter.USBFacadeRemoteAdapter(self._proxy)


    def create_browser_facade(self):
        """"Creates a browser facade object."""
        return browser_facade_adapter.BrowserFacadeRemoteAdapter(self._proxy)


    def create_bluetooth_hid_facade(self):
        """"Creates a bluetooth hid facade object."""
        return bluetooth_hid_facade_adapter.BluetoothHIDFacadeRemoteAdapter(
                self._client, self._proxy)


    def create_input_facade(self):
        """"Creates an input facade object."""
        return input_facade_adapter.InputFacadeRemoteAdapter(self._proxy)


    def create_cfm_facade(self):
        """"Creates a cfm facade object."""
        return cfm_facade_adapter.CFMFacadeRemoteAdapter(
                self._client, self._proxy)


    def create_kiosk_facade(self):
         """"Creates a kiosk facade object."""
         return kiosk_facade_adapter.KioskFacadeRemoteAdapter(
                self._client, self._proxy)
