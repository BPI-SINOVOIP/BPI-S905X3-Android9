# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Library for processing, verifying and applying Chrome OS update payloads."""

# Just raise the interface classes to the root namespace.
from update_payload.checker import CHECKS_TO_DISABLE
from update_payload.error import PayloadError
from update_payload.payload import Payload
