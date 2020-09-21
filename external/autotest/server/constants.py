# Flag file to indicate the host is an adb tester.
ANDROID_TESTER_FILEFLAG = '/mnt/stateful_partition/.android_tester'

# Android test environment constants
SL4A_APK = 'sl4a.apk'
SL4A_PACKAGE = 'com.googlecode.android_scripting'
SL4A_ARTIFACT = 'test_zip'

class Labels:
    """
    Constants related to label names.

    @var BOARD_PREFIX The string with which board labels are prefixed.
    @var MODEL_PREFIX The string with which model labels are prefixed.
    @var POOL_PREFIX The stright with which pool labels are prefixed.
    """
    BOARD_PREFIX = 'board:'
    MODEL_PREFIX = 'model:'
    POOL_PREFIX = 'pool:'


class Pools:
    """
    Well-known pool names used in automated inventory management.

    These are general purpose pools of DUTs that are considered
    identical for purposes of testing.  That is, a device in one of
    these pools can be shifted to another pool at will for purposes
    of supplying test demand.

    Devices in these pools are not allowed to have special-purpose
    attachments, or to be part of in any kind of custom fixture.
    Devices in these pools are also required to reside in areas
    managed by the Platforms team (i.e. at the time of this writing,
    only in "Atlantis" or "Destiny").

    CRITICAL_POOLS - Pools that must be kept fully supplied in order
        to guarantee timely completion of tests from builders.
    SPARE_POOL - A low priority pool that is allowed to provide
        spares to replace broken devices in the critical pools.
    MANAGED_POOLS - The set of all the general purpose pools
        monitored for health.
    """
    CRITICAL_POOLS = ['bvt', 'cq', 'continuous', 'cts', 'arc-presubmit']
    SPARE_POOL = 'suites'
    MANAGED_POOLS = CRITICAL_POOLS + [SPARE_POOL]


class Builds:
    """
    Constants related to build type.

    @var FIRMWARE_RW: The string indicating the given build is used to update
                      RW firmware.
    @var CROS: The string indicating the given build is used to update ChromeOS.
    """
    FIRMWARE_RW = 'firmware_rw'
    FIRMWARE_RO = 'firmware_ro'
    CROS = 'cros'
