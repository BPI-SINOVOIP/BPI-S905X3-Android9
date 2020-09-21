# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import sys

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.site_utils.lxc import constants
from autotest_lib.site_utils.lxc import lxc
from autotest_lib.site_utils.lxc import utils as lxc_utils
from autotest_lib.site_utils.lxc.container import Container


class BaseImage(object):
    """A class that manages a base container.

    Instantiating this class will cause it to search for a base container under
    the given path and name.  If one is found, the class adopts it.  If not, the
    setup() method needs to be called, to download and install a new base
    container.

    The actual base container can be obtained by calling the get() method.

    Calling cleanup() will delete the base container along with all of its
    associated snapshot clones.
    """

    def __init__(self,
                 container_path=constants.DEFAULT_CONTAINER_PATH,
                 base_name=constants.BASE):
        """Creates a new BaseImage.

        If a valid base container already exists on this machine, the BaseImage
        adopts it.  Otherwise, setup needs to be called to download a base and
        install a base container.

        @param container_path: The LXC path for the base container.
        @param base_name: The base container name.
        """
        self.container_path = container_path
        self.base_name = base_name
        try:
            base_container = Container.create_from_existing_dir(
                    container_path, base_name);
            base_container.refresh_status()
            self.base_container = base_container
        except error.ContainerError as e:
            self.base_container = None
            self.base_container_error = e


    def setup(self, name=None, force_delete=False):
        """Download and setup the base container.

        @param name: Name of the base container, defaults to the name passed to
                     the constructor.  If a different name is provided, that
                     name overrides the name originally passed to the
                     constructor.
        @param force_delete: True to force to delete existing base container.
                             This action will destroy all running test
                             containers. Default is set to False.
        """
        if name is not None:
            self.base_name = name

        if not self.container_path:
            raise error.ContainerError(
                    'You must set a valid directory to store containers in '
                    'global config "AUTOSERV/ container_path".')

        if not os.path.exists(self.container_path):
            os.makedirs(self.container_path)

        if self.base_container and not force_delete:
            logging.error(
                    'Base container already exists. Set force_delete to True '
                    'to force to re-stage base container. Note that this '
                    'action will destroy all running test containers')
            # Set proper file permission. base container in moblab may have
            # owner of not being root. Force to update the folder's owner.
            self._set_root_owner()
            return

        # Destroy existing base container if exists.
        if self.base_container:
            self.cleanup()

        try:
            self._download_and_install_base_container()
            self._set_root_owner()
        except:
            # Clean up if something went wrong.
            base_path = os.path.join(self.container_path, self.base_name)
            if lxc_utils.path_exists(base_path):
                exc_info = sys.exc_info()
                container = Container.create_from_existing_dir(
                        self.container_path, self.base_name)
                # Attempt destroy.  Log but otherwise ignore errors.
                try:
                    container.destroy()
                except error.CmdError as e:
                    logging.error(e)
                # Raise the cached exception with original backtrace.
                raise exc_info[0], exc_info[1], exc_info[2]
            else:
                raise
        else:
            self.base_container = Container.create_from_existing_dir(
                    self.container_path, self.base_name)


    def cleanup(self):
        """Destroys the base container.

        This operation will also destroy all snapshot clones of the base
        container.
        """
        # Find and delete clones first.
        for clone in self._find_clones():
            clone.destroy()
        base = Container.create_from_existing_dir(self.container_path,
                                                  self.base_name)
        base.destroy()


    def get(self):
        """Returns the base container.

        @raise ContainerError: If the base image is invalid or missing.
        """
        if self.base_container is None:
            raise self.base_container_error
        else:
            return self.base_container


    def _download_and_install_base_container(self):
        """Downloads the base image, untars and configures it."""
        base_path = os.path.join(self.container_path, self.base_name)
        tar_path = os.path.join(self.container_path,
                                '%s.tar.xz' % self.base_name)

        # Force cleanup of any previously downloaded/installed base containers.
        # This ensures a clean setup of the new base container.
        #
        # TODO(kenobi): Add a check to ensure that the base container doesn't
        # get deleted while snapshot clones exist (otherwise running tests might
        # get disrupted).
        path_to_cleanup = [tar_path, base_path]
        for path in path_to_cleanup:
            if os.path.exists(path):
                utils.run('sudo rm -rf "%s"' % path)
        container_url = constants.CONTAINER_BASE_URL_FMT % self.base_name
        lxc.download_extract(container_url, tar_path, self.container_path)
        # Remove the downloaded container tar file.
        utils.run('sudo rm "%s"' % tar_path)

        # Update container config with container_path from global config.
        config_path = os.path.join(base_path, 'config')
        rootfs_path = os.path.join(base_path, 'rootfs')
        utils.run(('sudo sed '
                   '-i "s|\(lxc\.rootfs[[:space:]]*=\).*$|\\1 {rootfs}|" '
                   '"{config}"').format(rootfs=rootfs_path,
                                        config=config_path))

    def _set_root_owner(self):
        """Changes the container group and owner to root.

        This is necessary because we currently run privileged containers.
        """
        # TODO(dshi): Change root to current user when test container can be
        # unprivileged container.
        base_path = os.path.join(self.container_path, self.base_name)
        utils.run('sudo chown -R root "%s"' % base_path)
        utils.run('sudo chgrp -R root "%s"' % base_path)


    def _find_clones(self):
        """Finds snapshot clones of the current base container."""
        snapshot_file = os.path.join(self.container_path,
                                     self.base_name,
                                     'lxc_snapshots')
        if not lxc_utils.path_exists(snapshot_file):
            return
        cmd = 'sudo cat %s' % snapshot_file
        clone_info = [line.strip()
                      for line in utils.run(cmd).stdout.splitlines()]
        # lxc_snapshots contains pairs of lines (lxc_path, container_name).
        for i in range(0, len(clone_info), 2):
            lxc_path = clone_info[i]
            name = clone_info[i+1]
            yield Container.create_from_existing_dir(lxc_path, name)
