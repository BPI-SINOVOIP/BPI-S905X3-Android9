#!/usr/bin/env python3.4
#
#   Copyright 2017 - The Android Open Source Project
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

import logging
import socket
import sys

from acts import base_test
from acts.controllers.ap_lib import hostapd_ap_preset
from acts.controllers.ap_lib import hostapd_bss_settings
from acts.controllers.ap_lib import hostapd_constants
from acts.controllers.ap_lib import hostapd_config
from acts.controllers.ap_lib import hostapd_security


class SetupWifiNetworkTest(base_test.BaseTestClass):
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
            connection.close()
            sock.shutdown(socket.SHUT_RDWR)
            sock.close()

    def setup_ap(self):
        bss_settings = []
        self.access_point = self.access_points[0]
        network_type = self.user_params["network_type"]
        if (network_type == "2G"):
            self.channel = hostapd_constants.AP_DEFAULT_CHANNEL_2G
        else:
            self.channel = hostapd_constants.AP_DEFAULT_CHANNEL_5G

        self.ssid = self.user_params["ssid"]
        self.security = self.user_params["security"]
        if self.security == "none":
            self.config = hostapd_ap_preset.create_ap_preset(
                channel=self.channel,
                ssid=self.ssid,
                bss_settings=bss_settings,
                profile_name='whirlwind')
        else:
            self.passphrase = self.user_params["passphrase"]
            self.hostapd_security = hostapd_security.Security(
                security_mode=self.security, password=self.passphrase)
            self.config = hostapd_ap_preset.create_ap_preset(
                channel=self.channel,
                ssid=self.ssid,
                security=self.hostapd_security,
                bss_settings=bss_settings,
                profile_name='whirlwind')
        self.access_point.start_ap(self.config)

    def test_set_up_single_ap(self):
        req_params = [
            "AccessPoint", "network_type", "ssid", "passphrase", "security",
            "socket_port", "socket_timeout_secs"
        ]
        opt_params = []
        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_params)
        # Setup the AP environment
        self.setup_ap()
        # AP enviroment created. Wait for client to teardown the environment
        self.wait_for_test_completion()

    def test_set_up_open_ap(self):
        req_params = [
            "AccessPoint", "network_type", "ssid", "security", "socket_port",
            "socket_timeout_secs"
        ]
        opt_params = []
        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_params)
        # Setup the AP environment
        self.setup_ap()
        # AP enviroment created. Wait for client to teardown the environment
        self.wait_for_test_completion()
