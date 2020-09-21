# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import common
from autotest_lib.frontend.afe.json_rpc import proxy as rpc_proxy
from autotest_lib.server.hosts import host_info
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers

class AfeStore(host_info.CachingHostInfoStore):
    """Directly interact with the (given) AFE for host information."""

    _RETRYING_AFE_TIMEOUT_MIN = 5
    _RETRYING_AFE_RETRY_DELAY_SEC = 10

    def __init__(self, hostname, afe=None):
        """
        @param hostname: The name of the host for which we want to track host
                information.
        @param afe: A frontend.AFE object to make RPC calls. Will create one
                internally if None.
        """
        super(AfeStore, self).__init__()
        self._hostname = hostname
        self._afe = afe
        if self._afe is None:
            self._afe = frontend_wrappers.RetryingAFE(
                    timeout_min=self._RETRYING_AFE_TIMEOUT_MIN,
                    delay_sec=self._RETRYING_AFE_RETRY_DELAY_SEC)


    def __str__(self):
        return '%s[%s]' % (type(self).__name__, self._hostname)


    def _refresh_impl(self):
        """Obtains HostInfo directly from the AFE."""
        try:
            hosts = self._afe.get_hosts(hostname=self._hostname)
        except rpc_proxy.JSONRPCException as e:
            raise host_info.StoreError(e)

        if not hosts:
            raise host_info.StoreError('No hosts founds with hostname: %s' %
                                       self._hostname)

        if len(hosts) > 1:
            logging.warning(
                    'Found %d hosts with the name %s. Picking the first one.',
                    len(hosts), self._hostname)
        host = hosts[0]
        return host_info.HostInfo(host.labels, host.attributes)


    def _commit_impl(self, new_info):
        """Commits HostInfo back to the AFE.

        @param new_info: The new HostInfo to commit.
        """
        # TODO(pprabhu) crbug.com/680322
        # This method has a potentially malignent race condition. We obtain a
        # copy of HostInfo from the AFE and then add/remove labels / attribtes
        # based on that. If another user tries to commit it's changes in
        # parallel, we'll end up with corrupted labels / attributes.
        old_info = self._refresh_impl()
        self._remove_labels_on_afe(
                list(set(old_info.labels) - set(new_info.labels)))
        self._add_labels_on_afe(
                list(set(new_info.labels) - set(old_info.labels)))
        self._update_attributes_on_afe(old_info.attributes, new_info.attributes)


    def _remove_labels_on_afe(self, labels):
        """Requests the AFE to remove the given labels.

        @param labels: Remove these.
        """
        if not labels:
            return

        logging.debug('removing labels: %s', labels)
        try:
            self._afe.run('host_remove_labels', id=self._hostname,
                          labels=labels)
        except rpc_proxy.JSONRPCException as e:
            raise host_info.StoreError(e)


    def _add_labels_on_afe(self, labels):
        """Requests the AFE to add the given labels.

        @param labels: Add these.
        """
        if not labels:
            return

        logging.info('adding labels: %s', labels)
        try:
            self._afe.run('host_add_labels', id=self._hostname, labels=labels)
        except rpc_proxy.JSONRPCException as e:
            raise host_info.StoreError(e)


    def _update_attributes_on_afe(self, old_attributes, new_attributes):
        """Updates host attributes on the afe to give dict.

        @param old_attributes: The current attributes on AFE.
        @param new_attributes: The new host attributes dict to set to.
        """
        left_only, right_only, differing = _dict_diff(old_attributes,
                                                      new_attributes)
        for key in left_only:
            self._afe.set_host_attribute(key, None, hostname=self._hostname)
        for key in right_only | differing:
            self._afe.set_host_attribute(key, new_attributes[key],
                                         hostname=self._hostname)


def _dict_diff(left_dict, right_dict):
    """Return the keys where the given dictionaries differ.

    This function assumes that the values in the dictionary support checking for
    equality.

    @param left_dict: The "left" dictionary in the diff.
    @param right_dict: The "right" dictionary in the diff.
    @returns: A 3-tuple (left_only, right_only, differing) of keys where
            left_only contains the keys that exist in left_dict only, right_only
            contains keys that exist in right_dict only and differing contains
            keys that exist in both, but where values differ.
    """
    left_keys = set(left_dict)
    right_keys = set(right_dict)
    differing_keys = {key for key in left_keys & right_keys
                      if left_dict[key] != right_dict[key]}
    return left_keys - right_keys, right_keys - left_keys, differing_keys
