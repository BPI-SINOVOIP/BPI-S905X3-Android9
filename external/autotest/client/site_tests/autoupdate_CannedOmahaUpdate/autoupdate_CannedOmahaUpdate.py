# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import shutil

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.client.cros.cellular import test_environment
from autotest_lib.client.cros.update_engine import nano_omaha_devserver

class autoupdate_CannedOmahaUpdate(test.test):
    """
    Client-side mechanism to update a DUT with a given image.

    Restarts update_engine and attempts an update from the image
    pointed to by |image_url| of size |image_size| with checksum
    |image_sha256|. The rest of the parameters are optional.

    If the |allow_failure| parameter is True, then the test will
    succeed even if the update failed.

    """
    version = 1

    def cleanup(self):
        shutil.copy('/var/log/update_engine.log', self.resultsdir)


    def run_canned_update(self, allow_failure):
        """
        Performs the update.

        @param allow_failure: True if we dont raise an error on failure.

        """
        utils.run('restart update-engine')
        self._omaha.start()
        try:
            utils.run('update_engine_client -update -omaha_url=' +
                      'http://127.0.0.1:%d/update ' % self._omaha.get_port())
        except error.CmdError as e:
            self._omaha.stop()
            if not allow_failure:
                raise error.TestFail('Update attempt failed: %s' % e)
            else:
                logging.info('Ignoring failed update. Failure reason: %s', e)


    def run_once(self, image_url, image_size, image_sha256,
                 allow_failure=False, metadata_size=None,
                 metadata_signature=None, public_key=None, use_cellular=False,
                 is_delta=False):

        self._omaha = nano_omaha_devserver.NanoOmahaDevserver()
        self._omaha.set_image_params(image_url, image_size, image_sha256,
                                     metadata_size, metadata_signature,
                                     public_key, is_delta)

        if use_cellular:
            # Setup DUT so that we have ssh over ethernet but DUT uses
            # cellular as main connection.
            try:
                test_env = test_environment.CellularOTATestEnvironment()
                CONNECT_TIMEOUT = 120
                with test_env:
                    service = test_env.shill.wait_for_cellular_service_object()
                    if not service:
                        raise error.TestError('No cellular service found.')
                    test_env.shill.connect_service_synchronous(
                            service, CONNECT_TIMEOUT)
                    self.run_canned_update(allow_failure)
            except error.TestError as e:
                # Raise as test failure so it is propagated to server test
                # failure message.
                logging.error('Failed setting up cellular connection.')
                raise error.TestFail(e)
        else:
            self.run_canned_update(allow_failure)