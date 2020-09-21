# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import absolute_import
from __future__ import print_function

import logging

import common
from autotest_lib.server.hosts import host_info
from chromite.lib import metrics


_METRICS_PREFIX = 'chromeos/autotest/autoserv/host_info/shadowing_store/'
_REFRESH_METRIC_NAME = _METRICS_PREFIX + 'refresh_count'
_COMMIT_METRIC_NAME = _METRICS_PREFIX + 'commit_count'


logger = logging.getLogger(__file__)

class ShadowingStore(host_info.CachingHostInfoStore):
    """A composite CachingHostInfoStore that maintains a main and shadow store.

    ShadowingStore accepts two CachingHostInfoStore objects - primary_store and
    shadow_store. All refresh/commit operations are serviced through
    primary_store.  In addition, shadow_store is updated and compared with this
    information, leaving breadcrumbs when the two differ. Any errors in
    shadow_store operations are logged and ignored so as to not affect the user.

    This is a transitional CachingHostInfoStore that allows us to continue to
    use an AfeStore in practice, but also create a backing FileStore so that we
    can validate the use of FileStore in prod.
    """

    def __init__(self, primary_store, shadow_store,
                 mismatch_callback=None):
        """
        @param primary_store: A CachingHostInfoStore to be used as the primary
                store.
        @param shadow_store: A CachingHostInfoStore to be used to shadow the
                primary store.
        @param mismatch_callback: A callback used to notify whenever we notice a
                mismatch between primary_store and shadow_store. The signature
                of the callback must match:
                    callback(primary_info, shadow_info)
                where primary_info and shadow_info are HostInfo objects obtained
                from the two stores respectively.
                Mostly used by unittests. Actual users don't know / nor care
                that they're using a ShadowingStore.
        """
        super(ShadowingStore, self).__init__()
        self._primary_store = primary_store
        self._shadow_store = shadow_store
        self._mismatch_callback = (
                mismatch_callback if mismatch_callback is not None
                else _log_info_mismatch)
        try:
            self._shadow_store.commit(self._primary_store.get())
        except host_info.StoreError as e:
            metrics.Counter(
                    _METRICS_PREFIX + 'initialization_fail_count').increment()
            logger.exception(
                    'Failed to initialize shadow store. '
                    'Expect primary / shadow desync in the future.')

    def __str__(self):
        return '%s[%s, %s]' % (type(self).__name__, self._primary_store,
                               self._shadow_store)

    def _refresh_impl(self):
        """Obtains HostInfo from the primary and compares against shadow"""
        primary_info = self._refresh_from_primary_store()
        try:
            shadow_info = self._refresh_from_shadow_store()
        except host_info.StoreError:
            logger.exception('Shadow refresh failed. '
                             'Skipping comparison with primary.')
            return primary_info
        self._verify_store_infos(primary_info, shadow_info)
        return primary_info

    def _commit_impl(self, info):
        """Commits HostInfo to both the primary and shadow store"""
        self._commit_to_primary_store(info)
        self._commit_to_shadow_store(info)

    def _commit_to_primary_store(self, info):
        try:
            self._primary_store.commit(info)
        except host_info.StoreError:
            metrics.Counter(_COMMIT_METRIC_NAME).increment(
                    fields={'file_commit_result': 'skipped'})
            raise

    def _commit_to_shadow_store(self, info):
        try:
            self._shadow_store.commit(info)
        except host_info.StoreError:
            logger.exception(
                    'shadow commit failed. '
                    'Expect primary / shadow desync in the future.')
            metrics.Counter(_COMMIT_METRIC_NAME).increment(
                    fields={'file_commit_result': 'fail'})
        else:
            metrics.Counter(_COMMIT_METRIC_NAME).increment(
                    fields={'file_commit_result': 'success'})

    def _refresh_from_primary_store(self):
        try:
            return self._primary_store.get(force_refresh=True)
        except host_info.StoreError:
            metrics.Counter(_REFRESH_METRIC_NAME).increment(
                    fields={'validation_result': 'skipped'})
            raise

    def _refresh_from_shadow_store(self):
        try:
            return self._shadow_store.get(force_refresh=True)
        except host_info.StoreError:
            metrics.Counter(_REFRESH_METRIC_NAME).increment(fields={
                    'validation_result': 'fail_shadow_store_refresh'})
            raise

    def _verify_store_infos(self, primary_info, shadow_info):
        if primary_info == shadow_info:
            metrics.Counter(_REFRESH_METRIC_NAME).increment(
                    fields={'validation_result': 'success'})
        else:
            self._mismatch_callback(primary_info, shadow_info)
            metrics.Counter(_REFRESH_METRIC_NAME).increment(
                    fields={'validation_result': 'fail_mismatch'})
            self._shadow_store.commit(primary_info)


def _log_info_mismatch(primary_info, shadow_info):
    """Log the two HostInfo instances.

    Used as the default mismatch_callback.
    """
    logger.warning('primary / shadow disagree on refresh.')
    logger.warning('primary: %s', primary_info)
    logger.warning('shadow: %s', shadow_info)
