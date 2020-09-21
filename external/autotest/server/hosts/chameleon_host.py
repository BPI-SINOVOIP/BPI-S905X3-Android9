# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

"""This file provides core logic for connecting a Chameleon Daemon."""

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.cros.chameleon import chameleon
from autotest_lib.server.cros import dnsname_mangler
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server.hosts import ssh_host


# Names of the host attributes in the database that represent the values for
# the chameleon_host and chameleon_port for a servo connected to the DUT.
CHAMELEON_HOST_ATTR = 'chameleon_host'
CHAMELEON_PORT_ATTR = 'chameleon_port'

_CONFIG = global_config.global_config
ENABLE_SSH_TUNNEL_FOR_CHAMELEON = _CONFIG.get_config_value(
        'CROS', 'enable_ssh_tunnel_for_chameleon', type=bool, default=False)

class ChameleonHostError(Exception):
    """Error in ChameleonHost."""
    pass


class ChameleonHost(ssh_host.SSHHost):
    """Host class for a host that controls a Chameleon."""

    # Chameleond process name.
    CHAMELEOND_PROCESS = 'chameleond'


    # TODO(waihong): Add verify and repair logic which are required while
    # deploying to Cros Lab.


    def _initialize(self, chameleon_host='localhost', chameleon_port=9992,
                    *args, **dargs):
        """Initialize a ChameleonHost instance.

        A ChameleonHost instance represents a host that controls a Chameleon.

        @param chameleon_host: Name of the host where the chameleond process
                               is running.
                               If this is passed in by IP address, it will be
                               treated as not in lab.
        @param chameleon_port: Port the chameleond process is listening on.

        """
        super(ChameleonHost, self)._initialize(hostname=chameleon_host,
                                               *args, **dargs)

        self._is_in_lab = None
        self._check_if_is_in_lab()

        self._chameleon_port = chameleon_port
        self._local_port = None
        self._tunneling_process = None

        try:
            if self._is_in_lab and not ENABLE_SSH_TUNNEL_FOR_CHAMELEON:
                self._chameleon_connection = chameleon.ChameleonConnection(
                        self.hostname, chameleon_port)
            else:
                # A proxy generator is passed as an argument so that a proxy
                # could be re-created on demand in ChameleonConnection
                # whenever needed, e.g., after a reboot.
                proxy_generator = (
                        lambda: self.rpc_server_tracker.xmlrpc_connect(
                                None, chameleon_port,
                                ready_test_name=chameleon.CHAMELEON_READY_TEST,
                                timeout_seconds=60))
                self._chameleon_connection = chameleon.ChameleonConnection(
                        None, proxy_generator=proxy_generator)

        except Exception as e:
            raise ChameleonHostError('Can not connect to Chameleon: %s(%s)',
                                     e.__class__, e)


    def _check_if_is_in_lab(self):
        """Checks if Chameleon host is in lab and set self._is_in_lab.

        If self.hostname is an IP address, we treat it as is not in lab zone.

        """
        self._is_in_lab = (False if dnsname_mangler.is_ip_address(self.hostname)
                           else utils.host_is_in_lab_zone(self.hostname))


    def is_in_lab(self):
        """Check whether the chameleon host is a lab device.

        @returns: True if the chameleon host is in Cros Lab, otherwise False.

        """
        return self._is_in_lab


    def get_wait_up_processes(self):
        """Get the list of local processes to wait for in wait_up.

        Override get_wait_up_processes in
        autotest_lib.client.common_lib.hosts.base_classes.Host.
        Wait for chameleond process to go up. Called by base class when
        rebooting the device.

        """
        processes = [self.CHAMELEOND_PROCESS]
        return processes


    def create_chameleon_board(self):
        """Create a ChameleonBoard object with error recovery.

        This function will reboot the chameleon board once and retry if we can't
        create chameleon board.

        @return A ChameleonBoard object.
        """
        # TODO(waihong): Add verify and repair logic which are required while
        # deploying to Cros Lab.
        chameleon_board = None
        try:
            chameleon_board = chameleon.ChameleonBoard(
                    self._chameleon_connection, self)
            return chameleon_board
        except:
            self.reboot()
            chameleon_board = chameleon.ChameleonBoard(
                self._chameleon_connection, self)
            return chameleon_board


def create_chameleon_host(dut, chameleon_args):
    """Create a ChameleonHost object.

    There three possible cases:
    1) If the DUT is in Cros Lab and has a chameleon board, then create
       a ChameleonHost object pointing to the board. chameleon_args
       is ignored.
    2) If not case 1) and chameleon_args is neither None nor empty, then
       create a ChameleonHost object using chameleon_args.
    3) If neither case 1) or 2) applies, return None.

    @param dut: host name of the host that chameleon connects. It can be used
                to lookup the chameleon in test lab using naming convention.
                If dut is an IP address, it can not be used to lookup the
                chameleon in test lab.
    @param chameleon_args: A dictionary that contains args for creating
                           a ChameleonHost object,
                           e.g. {'chameleon_host': '172.11.11.112',
                                 'chameleon_port': 9992}.

    @returns: A ChameleonHost object or None.

    """
    if not utils.is_in_container():
        is_moblab = utils.is_moblab()
    else:
        is_moblab = _CONFIG.get_config_value(
                'SSP', 'is_moblab', type=bool, default=False)

    if not is_moblab:
        dut_is_hostname = not dnsname_mangler.is_ip_address(dut)
        if dut_is_hostname:
            chameleon_hostname = chameleon.make_chameleon_hostname(dut)
            if utils.host_is_in_lab_zone(chameleon_hostname):
                # Be more tolerant on chameleon in the lab because
                # we don't want dead chameleon blocks non-chameleon tests.
                if utils.ping(chameleon_hostname, deadline=3):
                   logging.warning(
                           'Chameleon %s is not accessible. Please file a bug'
                           ' to test lab', chameleon_hostname)
                   return None
                return ChameleonHost(chameleon_host=chameleon_hostname)
        if chameleon_args:
            return ChameleonHost(**chameleon_args)
        else:
            return None
    else:
        afe = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10)
        hosts = afe.get_hosts(hostname=dut)
        if hosts and CHAMELEON_HOST_ATTR in hosts[0].attributes:
            return ChameleonHost(
                chameleon_host=hosts[0].attributes[CHAMELEON_HOST_ATTR],
                chameleon_port=hosts[0].attributes.get(
                    CHAMELEON_PORT_ATTR, 9992)
            )
        else:
            return None
