# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
import ConfigParser
import logging
import os

from autotest_lib.server.cros import ap_config

AP_BOX_STR = 'ap_box_'
RF_SWITCH_STR = 'rf_switch_'
RF_SWITCH_APS = 'rf_switch_aps'
FILE_NAME = '%s_%s_ap_list.conf'


class APBoxException(Exception):
    pass


class APBox(object):
    """Class to manage APs in an AP Box."""


    def __init__(self, ap_box_host):
        """Constructor for the AP Box.

        @param ap_box_host: AP Box AFE Host object.

        @raises APBoxException.
        """
        self.ap_box_host = ap_box_host
        self.ap_box_label = ''
        self.rf_switch_label = ''
        for label in ap_box_host.labels:
            if label.startswith(AP_BOX_STR):
                self.ap_box_label = label
            elif label.startswith(RF_SWITCH_STR) and (
                    label != RF_SWITCH_APS):
                self.rf_switch_label = label
        if not self.ap_box_label or not self.rf_switch_label:
            raise APBoxException(
                    'AP Box %s does not have ap_box and/or rf_switch labels' %
                    ap_box_host.hostname)
        self.aps = None


    def _get_ap_list(self):
        """Returns a list of all APs in the AP Box.

        @returns a list of autotest_lib.server.cros.AP objects.
        """
        aps = []
        # FILE_NAME is formed using rf_switch and ap_box labels.
        # for example, rf_switch_1 and ap_box_1, the configuration
        # filename is rf_switch_1_ap_box_1_ap_list.conf
        file_name = FILE_NAME % (
                self.rf_switch_label.lower(), self.ap_box_label.lower())
        ap_config_parser = ConfigParser.RawConfigParser()
        path = os.path.join(
                os.path.dirname(os.path.abspath(__file__)), '..',
                file_name)
        logging.debug('Reading the static configurations from %s', path)
        ap_config_parser.read(path)
        for bss in ap_config_parser.sections():
            aps.append(ap_config.AP(bss, ap_config_parser))
        return aps


    def get_ap_list(self):
        """Returns a list of all APs in the AP Box.

        @returns a list of autotest_lib.server.cros.AP objects.
        """
        if self.aps is None:
            self.aps = self._get_ap_list()
        return self.aps

