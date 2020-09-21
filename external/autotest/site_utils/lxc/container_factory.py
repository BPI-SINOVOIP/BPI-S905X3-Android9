# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.site_utils.lxc import constants
from autotest_lib.site_utils.lxc import container

try:
    from chromite.lib import metrics
except ImportError:
    metrics = utils.metrics_mock


class ContainerFactory(object):
    """A factory class for creating LXC container objects."""

    def __init__(self, base_container, container_class=container.Container,
                 snapshot=True, force_cleanup=False,
                 lxc_path=constants.DEFAULT_CONTAINER_PATH):
        """Initializes a ContainerFactory.

        @param base_container: The base container from which other containers
                               are cloned.
        @param container_class: (optional) The Container class to instantiate.
                                By default, lxc.Container is instantiated.
        @param snapshot: (optional) If True, creates LXC snapshot clones instead
                         of full clones.  By default, snapshot clones are used.
        @param force_cleanup: (optional) If True, if a container is created with
                              a name and LXC directory matching an existing
                              container, the existing container is destroyed,
                              and the new container created in its place. By
                              default, existing containers are not destroyed and
                              a ContainerError is raised.
        @param lxc_path: (optional) The default LXC path that will be used for
                         new containers.  If one is not provided, the
                         DEFAULT_CONTAINER_PATH from lxc.constants will be used.
                         Note that even if a path is provided here, it can still
                         be overridden when create_container is called.
        """
        self._container_class = container_class
        self._base_container = base_container
        self._snapshot = snapshot
        self._force_cleanup = force_cleanup
        self._lxc_path = lxc_path


    def create_container(self, cid=None, lxc_path=None):
        """Creates a new container.

        @param cid: (optional) A ContainerId for the new container.  If an ID is
                    provided, it determines both the name and the ID of the
                    container.  If no ID is provided, a random name is generated
                    for the container, and it is not assigned an ID.
        @param lxc_path: (optional) The LXC path for the new container.  If one
                         is not provided, the factory's default lxc_path
                         (specified when the factory was constructed) is used.
        """
        name = str(cid) if cid else None
        if lxc_path is None:
            lxc_path = self._lxc_path

        logging.debug('Creating new container (name: %s, lxc_path: %s)',
                      name, lxc_path)

        # If an ID is provided, use it as the container name.
        new_container = self._create_from_base(name, lxc_path)
        # If an ID is provided, assign it to the container.  When the container
        # is created just-in-time by the container bucket, this ensures that the
        # resulting container is correctly registered with the autoserv system.
        # If the container is being created by a container pool, the ID will be
        # assigned later, when the continer is bound to an actual test process.
        if cid:
            new_container.id = cid
        return new_container


    # create_from_base_duration is the original name of the metric.  Keep this
    # so we have history.
    @metrics.SecondsTimerDecorator(
            '%s/create_from_base_duration' % constants.STATS_KEY)
    def _create_from_base(self, name, lxc_path):
        """Creates a container from the base container.

        @param name: Name of the container.
        @param lxc_path: The LXC path of the new container.

        @return: A Container object for the created container.

        @raise ContainerError: If the container already exist.
        @raise error.CmdError: If lxc-clone call failed for any reason.
        """
        use_snapshot = constants.SUPPORT_SNAPSHOT_CLONE and self._snapshot

        try:
            return self._container_class.clone(src=self._base_container,
                                               new_name=name,
                                               new_path=lxc_path,
                                               snapshot=use_snapshot,
                                               cleanup=self._force_cleanup)
        except error.CmdError:
            logging.debug('Creating snapshot clone failed. Attempting without '
                           'snapshot...')
            if not use_snapshot:
                raise
            else:
                # Snapshot clone failed, retry clone without snapshot.
                return self._container_class.clone(src=self._base_container,
                                                   new_name=name,
                                                   new_path=lxc_path,
                                                   snapshot=False,
                                                   cleanup=self._force_cleanup)
