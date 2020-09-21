# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

SDK_TOOLS_DIR = 'gs://chromeos-arc-images/builds/git_nyc-mr1-arc-linux-static_sdk_tools/3544738'
SDK_TOOLS_FILES = ['aapt']

# To stabilize adb behavior, we use dynamically linked adb.
ADB_DIR = 'gs://chromeos-arc-images/builds/git_nyc-mr1-arc-linux-cheets_arm-user/3544738'
ADB_FILES = ['adb']

ADB_POLLING_INTERVAL_SECONDS = 1
ADB_READY_TIMEOUT_SECONDS = 60
ANDROID_ADB_KEYS_PATH = '/data/misc/adb/adb_keys'

ARC_POLLING_INTERVAL_SECONDS = 1
ARC_READY_TIMEOUT_SECONDS = 60

TRADEFED_PREFIX = 'autotest-tradefed-install_'
# While running CTS tradefed creates state in the installed location (there is
# currently no way to specify a dedicated result directory for all changes).
# For this reason we start each test with a clean copy of the CTS/GTS bundle.
TRADEFED_CACHE_LOCAL = '/tmp/autotest-tradefed-cache'
# On lab servers and moblab all server tests run inside of lxc instances
# isolating file systems from each other. To avoid downloading CTS artifacts
# repeatedly for each test (or lxc instance) we share a common location
# /usr/local/autotest/results/shared which is visible to all lxc instances on
# that server. It needs to be writable as the cache is maintained jointly by
# all CTS/GTS tests. Currently both read and write access require taking the
# lock. Writes happen rougly monthly while reads are many times a day. If this
# becomes a bottleneck we could examine allowing concurrent reads.
TRADEFED_CACHE_CONTAINER = '/usr/local/autotest/results/shared/cache'
TRADEFED_CACHE_CONTAINER_LOCK = '/usr/local/autotest/results/shared/lock'
# The maximum size of the shared global cache. It needs to be able to hold
# N, M, x86, arm CTS bundles (500MB), the GTS bundle and media stress videos
# (2GB) zipped to not thrash. In addition it needs to be able to hold one
# different revision per Chrome OS branch. While this sounds  like a lot,
# only a single bundle is copied into each lxc instance (500MB), hence the
# impact of running say 100 CTS tests in parallel is acceptable (quarter
# servers have 500GB of disk, while full servers have 2TB).
TRADEFED_CACHE_MAX_SIZE = (20 * 1024 * 1024 * 1024)

# It looks like the GCE builder can be very slow and login on VMs take much
# longer than on hardware or bare metal.
LOGIN_BOARD_TIMEOUT = {'betty': 300}
LOGIN_DEFAULT_TIMEOUT = 90
