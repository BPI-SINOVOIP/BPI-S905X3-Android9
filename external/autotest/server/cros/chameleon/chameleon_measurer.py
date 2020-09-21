# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.cros.chameleon import chameleon_measurer_base
from autotest_lib.server.cros.multimedia import remote_facade_factory


class RemoteChameleonMeasurer(chameleon_measurer_base._BaseChameleonMeasurer):
    """A simple tool to measure using Chameleon for a server test.

    This class can only be used in a server test. For a client test, use the
    LocalChameleonMeasurer in client/cros/chameleon/chameleon_measurer.py.

    """

    def __init__(self, cros_host, outputdir=None, no_chrome=False):
        """Initializes the object."""
        self.host = cros_host
        factory = remote_facade_factory.RemoteFacadeFactory(
                cros_host, no_chrome)
        self.display_facade = factory.create_display_facade()

        self.chameleon = cros_host.chameleon
        self.chameleon.setup_and_reset(outputdir)
