# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.cros.chameleon import chameleon
from autotest_lib.client.cros.chameleon import chameleon_measurer_base
from autotest_lib.client.cros.multimedia import local_facade_factory


class LocalChameleonMeasurer(chameleon_measurer_base._BaseChameleonMeasurer):
    """A simple tool to measure using Chameleon for a client test.

    This class can only be used in a client test. For a server test, use the
    RemoteChameleonMeasurer in server/cros/chameleon/chameleon_measurer.py.

    """

    def __init__(self, cros_host, args, chrome, outputdir=None):
        """Initializes the object."""
        self.host = cros_host
        factory = local_facade_factory.LocalFacadeFactory(chrome)
        self.display_facade = factory.create_display_facade()

        self.chameleon = chameleon.create_chameleon_board(cros_host.hostname,
                                                          args)
        self.chameleon.setup_and_reset(outputdir)
