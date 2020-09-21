# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome

class platform_CryptohomeSyncStress(test.test):
    """
    This test is intended to be used from platform_CryptohomeSyncStressServer.
    """
    version = 1

    def run_once(self, username=None, password=None):
        # log in.
        with chrome.Chrome(username=username, password=password):

            # make sure cryptohome is mounted
            bus = dbus.SystemBus()
            proxy = bus.get_object('org.chromium.Cryptohome',
                                   '/org/chromium/Cryptohome')
            cryptohome = dbus.Interface(proxy, 'org.chromium.CryptohomeInterface')

            ismounted = cryptohome.IsMounted()
            if not ismounted:
                raise error.TestFail('Cryptohome failed to mount.')

            self.job.set_state('client_fail', False)
