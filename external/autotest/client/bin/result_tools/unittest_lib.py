# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


SIZE = 10

def create_file(path, size=SIZE):
    """Create a temp file at given path with the given size.

    @param path: Path to the temp file.
    @param size: Size of the temp file, default to SIZE.
    """
    with open(path, 'w') as f:
        f.write('A' * size)