# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import string

import common
from chromite.lib import gce

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import lsbrelease_utils
from autotest_lib.client.cros import constants as client_constants
from autotest_lib.server.hosts import abstract_ssh

SSH_KEYS_METADATA_KEY = "sshKeys"
TMP_DIR='/usr/local/tmp'

def extract_arguments(args_dict):
    """Extract GCE-specific arguments from arguments dictionary.

    @param args_dict: dictionary of all arguments supplied to the test.
    """

    return {k: v for k, v in args_dict.items()
            if k in ('gce_project', 'gce_instance',
                     'gce_zone', 'gce_key_file')}


class GceHost(abstract_ssh.AbstractSSHHost):
    """GCE-specific subclass of Host."""

    def _initialize(self, hostname, gce_args=None,
                    *args, **dargs):
        """Initializes this instance of GceHost.

        @param hostname: the hostnname to be passed down to AbstractSSHHost.
        @param gce_args: GCE-specific arguments extracted using
               extract_arguments().
        """
        super(GceHost, self)._initialize(hostname=hostname,
                                         *args, **dargs)

        if gce_args:
            self._gce_project = gce_args['gce_project']
            self._gce_zone = gce_args['gce_zone']
            self._gce_instance = gce_args['gce_instance']
            self._gce_key_file = gce_args['gce_key_file']
        else:
            logging.warning("No GCE flags provided, calls to GCE API will fail")
            return

        self.gce = gce.GceContext.ForServiceAccountThreadSafe(
                self._gce_project, self._gce_zone, self._gce_key_file)

    @staticmethod
    def check_host(host, timeout=10):
        """
        Check if the given host is running on GCE.

        @param host: An ssh host representing a device.
        @param timeout: The timeout for the run command.

        @return: True if the host is running on GCE.
        """
        try:
            result = host.run(
                    'grep CHROMEOS_RELEASE_BOARD /etc/lsb-release',
                     timeout=timeout)
            return lsbrelease_utils.is_gce_board(
                    lsb_release_content=result.stdout)
        except (error.AutoservRunError, error.AutoservSSHTimeout):
            return False

    def _modify_ssh_keys(self, to_add, to_remove):
        """Modifies the list of ssh keys.

        @param username: user name to add.
        @param to_add: a list of new enties.
        @param to_remove: a list of enties to be removed.
        """
        keys = self.gce.GetCommonInstanceMetadata(
                SSH_KEYS_METADATA_KEY) or ''
        key_set = set(string.split(keys, '\n'))
        new_key_set = (key_set | set(to_add)) - set(to_remove)
        if key_set != new_key_set:
            self.gce.SetCommonInstanceMetadata(
                    SSH_KEYS_METADATA_KEY,
                    string.join(list(new_key_set), '\n'))

    def add_ssh_key(self, username, ssh_key):
        """Adds a new SSH key in GCE metadata.

        @param username: user name to add.
        @param ssh_key: the key to add.
        """
        self._modify_ssh_keys(['%s:%s' % (username, ssh_key)], [])


    def del_ssh_key(self, username, ssh_key):
        """Deletes the given SSH key from GCE metadata

        @param username: user name to delete.
        @param ssh_key: the key to delete.
        """
        self._modify_ssh_keys([], ['%s:%s' % (username, ssh_key)])


    def get_release_version(self):
        """Get the value of attribute CHROMEOS_RELEASE_VERSION from lsb-release.

        @returns The version string in lsb-release, under attribute
                CHROMEOS_RELEASE_VERSION.
        """
        lsb_release_content = self.run(
                    'cat "%s"' % client_constants.LSB_RELEASE).stdout.strip()
        return lsbrelease_utils.get_chromeos_release_version(
                    lsb_release_content=lsb_release_content)

    def get_tmp_dir(self, parent=TMP_DIR):
        """Return the pathname of a directory on the host suitable
        for temporary file storage.

        The directory and its content will be deleted automatically
        on the destruction of the Host object that was used to obtain
        it.

        @param parent: Parent directory of the returned tmp dir.

        @returns a path to the tmp directory on the host.
        """
        if not parent.startswith(TMP_DIR):
            parent = os.path.join(TMP_DIR, parent.lstrip(os.path.sep))
        self.run("mkdir -p %s" % parent)
        template = os.path.join(parent, 'autoserv-XXXXXX')
        dir_name = self.run_output("mktemp -d %s" % template)
        self.tmp_dirs.append(dir_name)
        return dir_name


    def set_instance_metadata(self, key, value):
        """Sets a single metadata value on the DUT instance.

        @param key: Metadata key to be set.
        @param value: New value, or None if the given key should be removed.
        """
        self.gce.SetInstanceMetadata(self._gce_instance, key, value)

    def stop(self):
        """Stops the DUT instance
        """
        self.gce.StopInstance(self._gce_instance)

    def start(self):
        """Starts the DUT instance
        """
        self.gce.StartInstance(self._gce_instance)
