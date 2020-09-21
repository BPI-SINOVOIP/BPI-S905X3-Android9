# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#!/bin/sh
libtoolize && aclocal && autoconf && automake --add-missing
