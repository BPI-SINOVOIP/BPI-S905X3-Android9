# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module contains stuff for reporting deprecation.

TODO(ayatane): The reason for putting this module here is so client tests can use it.
"""

import urllib
import warnings


def warn(deprecated_name, stacklevel=3):
    """Convenience function for making deprecation warnings.

    @param deprecated_name: The name of the deprecated function or module
    @param stacklevel: See warnings.warn().
    """
    warnings.warn(APIDeprecationWarning(deprecated_name),
                  stacklevel=stacklevel)


class APIDeprecationWarning(UserWarning):
    """API deprecation warning.

    This is different from DeprecationWarning.  DeprecationWarning is
    for Python deprecations, this class is for our deprecations.
    """

    def __init__(self, deprecated_name):
        """Initialize instance.

        @param deprecated_name: The name of the deprecated function or module
        """
        super(APIDeprecationWarning, self).__init__(deprecated_name)
        self._deprecated_name = deprecated_name

    def __str__(self):
        return ('%s is deprecated; please file a fixit bug: %s'
                % (self._deprecated_name, self._get_fixit_bug_url()))

    def _get_fixit_bug_url(self):
        """Return the URL for making a fixit bug."""
        return ('https://bugs.chromium.org/p/chromium/issues/entry?'
                + urllib.urlencode({
                    'summary': 'Deprecated use of %s' % self._deprecated_name,
                    'description': 'Please paste the warning message below\n',
                    'components': 'Infra>Client>ChromeOS',
                    'labels': 'Pri-3,Type-Bug,Hotlist-Fixit',
                }))
