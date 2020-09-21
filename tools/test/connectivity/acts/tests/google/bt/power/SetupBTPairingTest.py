#!/usr/bin/env python3.4
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

"""
This test script leverages the relay_lib to pair different BT devices. This
script will be invoked from Tradefed test. The test will first setup pairing
between BT device and DUT and wait for signal (through socket) from tradefed
to power down the BT device
"""

import logging
import socket
import sys
import time

from acts import base_test

class SetupBTPairingTest(base_test.BaseTestClass):

    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)

    def select_device_by_mac_address(self, mac_address):
        for device in self.relay_devices:
            if device.mac_address == mac_address:
                return device
        return self.relay_devices[0]

    def setup_test(self):
        self.bt_device = self.select_device_by_mac_address(self.user_params["mac_address"])

    def wait_for_test_completion(self):
        port = int(self.user_params["socket_port"])
        timeout = float(self.user_params["socket_timeout_secs"])

        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        server_address = ('localhost', port)
        logging.info("Starting server socket on localhost port %s", port)
        sock.bind(('localhost', port))
        sock.settimeout(timeout)
        sock.listen(1)
        logging.info("Waiting for client socket connection")
        try:
            connection, client_address = sock.accept()
        except socket.timeout:
            logging.error("Did not receive signal. Shutting down AP")
        except socket.error:
            logging.error("Socket connection errored out. Shutting down AP")
        finally:
            if connection is not None:
                connection.close()
            if sock is not None:
                sock.shutdown(socket.SHUT_RDWR)
                sock.close()


    def enable_pairing_mode(self):
        self.bt_device.setup()
        self.bt_device.power_on()
        # Wait for a moment between pushing buttons
        time.sleep(2)
        self.bt_device.enter_pairing_mode()

    def test_bt_pairing(self):
        req_params = [
            "RelayDevice", "socket_port", "socket_timeout_secs"
        ]
        opt_params = []
        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_params)
        # Setup BT pairing mode
        self.enable_pairing_mode()
        # BT pairing mode is turned on
        self.wait_for_test_completion()

    def teardown_test(self):
        self.bt_device.power_off()
        self.bt_device.clean_up()
