# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for constructing links to AFE pages.

This module defines a class for the URL construction logic to loosen the
coupling and make it easier to test, but also defines module functions for
convenience and normal usage.

While there is a root_url property exposed, please refrain from using it to
construct URLs and instead add a suitable method to AfeUrls.

"""

import logging
import urllib
import urlparse
import sys

import common
from autotest_lib.client.common_lib import global_config

logger = logging.getLogger(__name__)


class AfeUrls(object):

    """Class for getting AFE URLs."""

    def __init__(self, root_url):
        """Initialize instance.

        @param root_url: AFE root URL.

        """
        self._root_url_parts = urlparse.urlsplit(root_url)

    _DEFAULT_URL = 'http://%s/afe/'

    def __hash__(self):
        return hash(self.root_url)

    def __eq__(self, other):
        if isinstance(other, type(self)):
            return self.root_url == other.root_url
        else:
            return NotImplemented

    @classmethod
    def from_hostname(cls, hostname):
        """Create AfeUrls given only a hostname, assuming default URL path.

        @param hostname: Hostname of AFE.
        @returns: AfeUrls instance.

        """
        return cls(cls._DEFAULT_URL % (hostname,))

    def _geturl(self, params):
        """Get AFE URL.

        All AFE URLs have the format:

        http://host/afe/#param1=val1&param2=val2

        This function constructs such a URL given a mapping of parameters.

        @param params: Mapping of URL parameters.
        @returns: URL string.

        """
        scheme, netloc, path, query, _fragment = self._root_url_parts
        fragment = urllib.urlencode(params)
        return urlparse.SplitResult(
            scheme, netloc, path, query, fragment).geturl()

    @property
    def root_url(self):
        """Canonical root URL.

        Note that this may not be the same as the URL passed into the
        constructor, due to canonicalization.

        """
        return self._root_url_parts.geturl()

    def get_host_url(self, host_id):
        """Get AFE URL for the given host.

        @param host_id: Host id.
        @returns: URL string.

        """
        return self._geturl({'tab_id': 'view_host', 'object_id': host_id})


_CONFIG = global_config.global_config
_HOSTNAME = _CONFIG.get_config_value('SERVER', 'hostname')
if not _HOSTNAME:
    logger.critical('[SERVER] hostname missing from the config file.')
    sys.exit(1)

_singleton_afe_urls = AfeUrls.from_hostname(_HOSTNAME)
ROOT_URL = _singleton_afe_urls.root_url
get_host_url = _singleton_afe_urls.get_host_url
