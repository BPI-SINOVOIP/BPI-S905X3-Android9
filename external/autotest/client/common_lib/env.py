# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Convenience functions that determine things about runtime environment.

Avoid adding any unneeded dependencies to this module, since we may want to
import it in a variety of unusual environments.
"""

IN_MOD_WSGI = False
try:
    # Per mod_wsgi documentation, this import will only suceed if we are running
    # inside a mod_wsgi process.
    # pylint: disable=unused-import
    from mod_wsgi import version
    IN_MOD_WSGI = True
except:
    pass
