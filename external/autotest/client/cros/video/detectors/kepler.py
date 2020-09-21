# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import logging
import subprocess

from autotest_lib.client.common_lib import error


def detect():
    """
    Checks whether or not a device equips kepler by using 'lspci'.

    @returns string: 'kepler' if a device equips kepler, empty string otherwise.
    """
    kepler_id = '1ae0:001a'
    try:
        lspci_result = subprocess.check_output(['lspci', '-n', '-d', kepler_id])
        logging.debug("lspci output:\n%s", lspci_result)
        return 'kepler' if lspci_result.strip() else ''
    except subprocess.CalledProcessError:
        logging.exception('lspci failed.')
        raise error.TestFail('Fail to execute "lspci"')
