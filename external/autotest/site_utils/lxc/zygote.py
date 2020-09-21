# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.site_utils.lxc import Container
from autotest_lib.site_utils.lxc import constants
from autotest_lib.site_utils.lxc import lxc
from autotest_lib.site_utils.lxc import utils as lxc_utils


class Zygote(Container):
    """A Container that implements post-bringup configuration.
    """

    def __init__(self, container_path, name, attribute_values, src=None,
                 snapshot=False, host_path=None):
        """Initialize an object of LXC container with given attribute values.

        @param container_path: Directory that stores the container.
        @param name: Name of the container.
        @param attribute_values: A dictionary of attribute values for the
                                 container.
        @param src: An optional source container.  If provided, the source
                    continer is cloned, and the new container will point to the
                    clone.
        @param snapshot: Whether or not to create a snapshot clone.  By default,
                         this is false.  If a snapshot is requested and creating
                         a snapshot clone fails, a full clone will be attempted.
        @param host_path: If set to None (the default), a host path will be
                          generated based on constants.DEFAULT_SHARED_HOST_PATH.
                          Otherwise, this can be used to override the host path
                          of the new container, for testing purposes.
        """
        # Check if this is a pre-existing LXC container.  Do this before calling
        # the super ctor, because that triggers container creation.
        exists = lxc.get_container_info(container_path, name=name)

        super(Zygote, self).__init__(container_path, name, attribute_values,
                                     src, snapshot)

        logging.debug(
                'Creating Zygote (lxcpath:%s name:%s)', container_path, name)

        # host_path is a directory within a shared bind-mount, which enables
        # bind-mounts from the host system to be shared with the LXC container.
        if host_path is not None:
            # Allow the host_path to be injected, for testing.
            self.host_path = host_path
        else:
            if exists:
                # Pre-existing Zygotes must have a host path.
                self.host_path = self._find_existing_host_dir()
                if self.host_path is None:
                    raise error.ContainerError(
                            'Container %s has no host path.' %
                            os.path.join(container_path, name))
            else:
                # New Zygotes use a predefined template to generate a host path.
                self.host_path = os.path.join(
                        os.path.realpath(constants.DEFAULT_SHARED_HOST_PATH),
                        self.name)

        # host_path_ro is a directory for holding intermediate mount points,
        # which are necessary when creating read-only bind mounts.  See the
        # mount_dir method for more details.
        #
        # Generate a host_path_ro based on host_path.
        ro_dir, ro_name = os.path.split(self.host_path.rstrip(os.path.sep))
        self.host_path_ro = os.path.join(ro_dir, '%s.ro' % ro_name)

        # Remember mounts so they can be cleaned up in destroy.
        self.mounts = []

        if exists:
            self._find_existing_bind_mounts()
        else:
            # Creating a new Zygote - initialize the host dirs.  Don't use sudo,
            # so that the resulting directories can be accessed by autoserv (for
            # SSP installation, etc).
            if not lxc_utils.path_exists(self.host_path):
                os.makedirs(self.host_path)
            if not lxc_utils.path_exists(self.host_path_ro):
                os.makedirs(self.host_path_ro)

            # Create the mount point within the container's rootfs.
            # Changes within container's rootfs require sudo.
            utils.run('sudo mkdir %s' %
                      os.path.join(self.rootfs,
                                   constants.CONTAINER_HOST_DIR.lstrip(
                                           os.path.sep)))
            self.mount_dir(self.host_path, constants.CONTAINER_HOST_DIR)


    def destroy(self, force=True):
        """Destroy the Zygote.

        This destroys the underlying container (see Container.destroy) and also
        cleans up any host mounts associated with it.

        @param force: Force container destruction even if it's running.  See
                      Container.destroy.
        """
        logging.debug('Destroying Zygote %s', self.name)
        super(Zygote, self).destroy(force)
        self._cleanup_host_mount()


    def install_ssp(self, ssp_url):
        """Downloads and installs the given server package.

        @param ssp_url: The URL of the ssp to download and install.
        """
        # The host dir is mounted directly on /usr/local/autotest within the
        # container.  The SSP structure assumes it gets untarred into the
        # /usr/local directory of the container's rootfs.  In order to unpack
        # with the correct directory structure, create a tmpdir, mount the
        # container's host dir as ./autotest, and unpack the SSP.
        if not self.is_running():
            super(Zygote, self).install_ssp(ssp_url)
            return

        usr_local_path = os.path.join(self.host_path, 'usr', 'local')
        os.makedirs(usr_local_path)

        with lxc_utils.TempDir(dir=usr_local_path) as tmpdir:
            download_tmp = os.path.join(tmpdir,
                                        'autotest_server_package.tar.bz2')
            lxc.download_extract(ssp_url, download_tmp, usr_local_path)

        container_ssp_path = os.path.join(
                constants.CONTAINER_HOST_DIR,
                constants.CONTAINER_AUTOTEST_DIR.lstrip(os.path.sep))
        self.attach_run('mkdir -p %s && mount --bind %s %s' %
                        (constants.CONTAINER_AUTOTEST_DIR,
                         container_ssp_path,
                         constants.CONTAINER_AUTOTEST_DIR))


    def copy(self, host_path, container_path):
        """Copies files into the Zygote.

        @param host_path: Path to the source file/dir to be copied.
        @param container_path: Path to the destination dir (in the container).
        """
        if not self.is_running():
            return super(Zygote, self).copy(host_path, container_path)

        logging.debug('copy %s to %s', host_path, container_path)

        # First copy the files into the host mount, then move them from within
        # the container.
        self._do_copy(src=host_path,
                      dst=os.path.join(self.host_path,
                                       container_path.lstrip(os.path.sep)))

        src = os.path.join(constants.CONTAINER_HOST_DIR,
                           container_path.lstrip(os.path.sep))
        dst = container_path

        # In the container, bind-mount from host path to destination.
        # The mount destination must have the correct type (file vs dir).
        if os.path.isdir(host_path):
            self.attach_run('mkdir -p %s' % dst)
        else:
            self.attach_run(
                'mkdir -p %s && touch %s' % (os.path.dirname(dst), dst))
        self.attach_run('mount --bind %s %s' % (src, dst))


    def mount_dir(self, source, destination, readonly=False):
        """Mount a directory in host to a directory in the container.

        @param source: Directory in host to be mounted.
        @param destination: Directory in container to mount the source directory
        @param readonly: Set to True to make a readonly mount, default is False.
        """
        if not self.is_running():
            return super(Zygote, self).mount_dir(source, destination, readonly)

        # Destination path in container must be absolute.
        if not os.path.isabs(destination):
            destination = os.path.join('/', destination)

        # Create directory in container for mount.
        self.attach_run('mkdir -p %s' % destination)

        # Creating read-only shared bind mounts is a two-stage process.  First,
        # the original file/directory is bind-mounted (with the ro option) to an
        # intermediate location in self.host_path_ro.  Then, the intermediate
        # location is bind-mounted into the shared host dir.
        # Replace the original source with this intermediate read-only mount,
        # then continue.
        if readonly:
            source_ro = os.path.join(self.host_path_ro,
                                     source.lstrip(os.path.sep))
            self.mounts.append(lxc_utils.BindMount.create(
                    source, self.host_path_ro, readonly=True))
            source = source_ro

        # Mount the directory into the host dir, then from the host dir into the
        # destination.
        self.mounts.append(
                lxc_utils.BindMount.create(source, self.host_path, destination))

        container_host_path = os.path.join(constants.CONTAINER_HOST_DIR,
                                           destination.lstrip(os.path.sep))
        self.attach_run('mount --bind %s %s' %
                        (container_host_path, destination))


    def _cleanup_host_mount(self):
        """Unmounts and removes the host dirs for this container."""
        # Clean up all intermediate bind mounts into host_path and host_path_ro.
        for mount in self.mounts:
            mount.cleanup()
        # The SSP and other "real" content gets copied into the host dir.  Use
        # rm -r to clear it out.
        if lxc_utils.path_exists(self.host_path):
            utils.run('sudo rm -r "%s"' % self.host_path)
        # The host_path_ro directory only contains intermediate bind points,
        # which should all have been cleared out.  Use rmdir.
        if lxc_utils.path_exists(self.host_path_ro):
            utils.run('sudo rmdir "%s"' % self.host_path_ro)


    def _find_existing_host_dir(self):
        """Finds the host mounts for a pre-existing Zygote.

        The host directory is passed into the Zygote constructor when creating a
        new Zygote.  However, when a Zygote is instantiated on top of an already
        existing LXC container, it has to reconnect to the existing host
        directory.

        @return: The host-side path to the host dir.
        """
        # Look for the mount that targets the "/host" dir within the container.
        for mount in self._get_lxc_config('lxc.mount.entry'):
            mount_cfg = mount.split(' ')
            if mount_cfg[1] == 'host':
                return mount_cfg[0]
        return None


    def _find_existing_bind_mounts(self):
        """Locates bind mounts associated with an existing container.

        When a Zygote object is instantiated on top of an existing LXC
        container, this method needs to be called so that all the bind-mounts
        associated with the container can be reconstructed.  This enables proper
        cleanup later.
        """
        for info in lxc_utils.get_mount_info():
            # Check for bind mounts in the host and host_ro directories, and
            # re-add them to self.mounts.
            if lxc_utils.is_subdir(self.host_path, info.mount_point):
                logging.debug('mount: %s', info.mount_point)
                self.mounts.append(lxc_utils.BindMount.from_existing(
                        self.host_path, info.mount_point))
            elif lxc_utils.is_subdir(self.host_path_ro, info.mount_point):
                logging.debug('mount_ro: %s', info.mount_point)
                self.mounts.append(lxc_utils.BindMount.from_existing(
                        self.host_path_ro, info.mount_point))
