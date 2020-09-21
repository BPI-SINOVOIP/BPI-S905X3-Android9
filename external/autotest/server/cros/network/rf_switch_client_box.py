# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# # Use of this source code is governed by a BSD-style license that can be
# # found in the LICENSE file.

import common
import itertools

from autotest_lib.server import frontend
from autotest_lib.server import site_utils


CLIENT_BOX_STR = 'client_box_'
RF_SWITCH_STR = 'rf_switch_'
RF_SWITCH_DUT = 'rf_switch_dut'
RF_SWITCH_CLIENT = 'rf_switch_client'


class ClientBoxException(Exception):
    pass


class ClientBox(object):
    """Class to manage devices in the Client Box."""


    def __init__(self, client_box_host):
        """Constructor for the ClientBox.

        @param client_box_host: Client Box AFE Host.
        """
        self.client_box_host = client_box_host
        self.client_box_label = ''
        self.rf_switch_label = ''
        for label in client_box_host.labels:
            if label.startswith(CLIENT_BOX_STR):
                self.client_box_label = label
            elif label.startswith(RF_SWITCH_STR) and (
                    label is not RF_SWITCH_CLIENT):
                self.rf_switch_label = label
        if not self.client_box_label or not self.rf_switch_label:
            msg = 'CleintBoxLabels: %s \t RfSwitchLabels: %s' % (
                     self.client_box_label, self.rf_switch_label)
            raise ClientBoxException(
                    'Labels not found:: %s' % msg)
        self.devices = None


    def get_devices(self):
        """Return all devices in the Client Box.

        @returns a list of autotest_lib.server.frontend.Host objects.
        """
        if self.devices is None:
            self.devices = self.get_devices_using_labels([RF_SWITCH_DUT])
        return self.devices

    def get_devices_using_labels(self, labels):
        """Returns all devices with the passed labels in the Client Box.

        @params labels: List of host labels.

        @returns a list of string containing the hostnames.
        """
        afe = frontend.AFE(
                debug=True, server=site_utils.get_global_afe_hostname())
        hosts = afe.get_hosts(label=self.client_box_label)
        labels.append(self.rf_switch_label)
        devices = []
        for host in hosts:
            labels_list = list(itertools.ifilter(
                lambda x: x in host.labels, labels))
            if len(labels) == len(labels_list):
                devices.append(host.hostname)
        return devices

