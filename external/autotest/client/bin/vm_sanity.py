#!/usr/bin/python
#
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''A simple sanity test for Chrome.

This script logs in, ensures that the cryptohome is mounted,
and checks that the browser is functional.
'''

from __future__ import print_function

import datetime
import logging
import os
import sys

# This sets up import paths for autotest.
import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros import arc, arc_common, chrome
from autotest_lib.client.common_lib.error import TestFail
from autotest_lib.client.cros import cryptohome


def main(args):
    '''The main function.'''
    if args:
        print('No args for vm_sanity.py')
        return os.EX_USAGE


    start = datetime.datetime.now()
    logging.info('Starting chrome and logging in.')
    is_arc_available = utils.is_arc_available()
    arc_mode = arc_common.ARC_MODE_ENABLED if is_arc_available else None
    with chrome.Chrome(arc_mode=arc_mode) as cr:
        # Check that the cryptohome is mounted.
        # is_vault_mounted throws an exception if it fails.
        logging.info('Checking mounted cryptohome.')
        cryptohome.is_vault_mounted(user=cr.username, allow_fail=False)
        # Navigate to about:blank.
        tab = cr.browser.tabs[0]
        tab.Navigate('about:blank')

        # Evaluate some javascript.
        logging.info('Evaluating JavaScript.')
        if tab.EvaluateJavaScript('2+2') != 4:
            raise TestFail('EvaluateJavaScript failed')

        # ARC test.
        if is_arc_available:
            arc.wait_for_android_process('org.chromium.arc.intent_helper')
            arc.wait_for_adb_ready()
            logging.info('Android booted successfully.')
            if not arc.is_package_installed('android'):
                raise TestFail('"android" system package was not listed by '
                               'Package Manager.')

    if is_arc_available:
        utils.poll_for_condition(lambda: not arc.is_adb_connected(),
                                 timeout=15,
                                 desc='Android container still running after '
                                      'Chrome shutdown.')
    elapsed = datetime.datetime.now() - start
    logging.info('Test succeeded in %s seconds.', elapsed.seconds)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
