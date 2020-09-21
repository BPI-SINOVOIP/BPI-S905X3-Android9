# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""RF Switch is a device used to route WiFi signals through RF cables
between 4 isolation boxes with APs and 4 isolation boxes with DUTs.

RFSwitchController Class provides methods to control RF Switch.

This class can be used to:
1) retrieve all APBoxes and ClientBoxes connected to the RF Switch.
2) connect Client and AP Boxes for testing.
3) retrieve information about current connections.
4) reset all connections.
"""

import common
import logging

from autotest_lib.server import frontend
from autotest_lib.server import site_utils
from autotest_lib.server.cros.network import rf_switch_ap_box
from autotest_lib.server.cros.network import rf_switch_client_box


RF_SWITCH_STR = 'rf_switch_'
RF_SWITCH_CLIENT = 'rf_switch_client'
RF_SWITCH_APS = 'rf_switch_aps'


class RFSwitchController(object):
    """Controller class to manage the RFSwitch."""


    def __init__(self, rf_switch_host):
        """Constructor for RF Switch Controller.

        @param rf_switch_host: An AFE Host object of RF Switch.
        """
        self.rf_switch_host = rf_switch_host
        self.rf_switch_labels = rf_switch_host.labels
        # RF Switches are named as rf_switch_1, rf_switch_2 using labels.
        # To find the rf_switch, find label starting with 'rf_switch_'
        labels = [
                s for s in self.rf_switch_labels if s.startswith(RF_SWITCH_STR)]
        afe = frontend.AFE(
                debug=True, server=site_utils.get_global_afe_hostname())
        self.hosts = afe.get_hosts(label=labels[0])
        self.ap_boxes = []
        self.client_boxes = []
        for host in self.hosts:
            if RF_SWITCH_APS in host.labels:
                self.ap_boxes.append(rf_switch_ap_box.APBox(host))
            elif RF_SWITCH_CLIENT in host.labels:
                self.client_boxes.append(rf_switch_client_box.ClientBox(host))
        if not self.ap_boxes:
            logging.error('No AP boxes available for the RF Switch.')
        if not self.client_boxes:
            logging.error('No Client boxes available for the RF Switch.')


    def get_ap_boxes(self):
        """Returns all AP Boxes connected to the RF Switch.

        @returns a list of
        autotest_lib.server.cros.network.rf_switch_ap_box.APBox objects.
        """
        return self.ap_boxes


    def get_client_boxes(self):
        """Returns all Client Boxes connected to the RF Switch.

        @returns a list of
        autotest_lib.server.cros.network.rf_switch_client_box.ClientBox objects.
        """
        return self.client_boxes
