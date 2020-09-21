# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging
import shutil
import time
import urlparse

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils

class autoupdate_DisconnectReconnectNetwork(test.test):
    """
    Tests removing network for a couple minutes.

    This test will be used in conjunction with
    autoupdate_ForcedOOBEUpdate.interrupt.full.

    """
    version = 1

    def cleanup(self):
        shutil.copy('/var/log/update_engine.log', self.resultsdir)

        # Turn adapters back on
        utils.run('ifconfig eth0 up', ignore_status=True)
        utils.run('ifconfig eth1 up', ignore_status=True)
        utils.start_service('recover_duts', ignore_status=True)

        # We can't return right after reconnecting the network or the server
        # test may not receive the message. So we wait a bit longer for the
        # DUT to be reconnected.
        utils.poll_for_condition(lambda: utils.ping(self._update_server,
                                                    deadline=5, timeout=5) == 0,
                                 timeout=60,
                                 sleep_interval=1)
        logging.info('Online ready to return to server test')


    def run_once(self, update_url, time_without_network=120):
        self._update_server = urlparse.urlparse(update_url).hostname
        # DUTs in the lab have a service called recover_duts that is used to
        # check that the DUT is online and if it is not it will bring it back
        # online. We will need to stop this service for the length of this test.
        utils.stop_service('recover_duts', ignore_status=True)

        # Disable the network adapters.
        utils.run('ifconfig eth0 down')
        utils.run('ifconfig eth1 down')

        # Check that we are offline.
        result = utils.ping(self._update_server, deadline=5, timeout=5)
        if result != 2:
            raise error.TestFail('Ping succeeded even though we were offline.')

        # Get the update percentage as the network is down
        percent_before = utils.run('update_engine_client --status').stdout
        percent_before = percent_before.splitlines()[1].partition('=')[2]

        seconds = 1
        while seconds < time_without_network:
            logging.info(utils.run('update_engine_client --status').stdout)
            time.sleep(1)
            seconds = seconds + 1

        percent_after = utils.run('update_engine_client --status').stdout
        percent_after = percent_after.splitlines()[1].partition('=')[2]

        if percent_before != percent_after:
            if percent_before < percent_after:
                raise error.TestFail('The update continued while the network '
                                     'was supposedly disabled. Before: '
                                     '%s, After: %s' % (percent_before,
                                                        percent_after))
            else:
                raise error.TestFail('The update appears to have restarted. '
                                     'Before: %s, After: %s' % (percent_before,
                                                                percent_after))