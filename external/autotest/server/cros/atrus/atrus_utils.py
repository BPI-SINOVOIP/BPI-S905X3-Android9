# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus

from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.cros import dbus_send
from autotest_lib.client.common_lib.cros.cfm.usb import usb_device_collector
from autotest_lib.client.common_lib.cros.cfm.usb import cfm_usb_devices

ATRUS = cfm_usb_devices.ATRUS

def check_dbus_available(host):
    """
    Checks if Atrusd with dbus support is available.

    @param host: Host object to run a particular host.
    @returns True if dbus is supported in atrusd. False otherwise.
    """
    result = dbus_send.dbus_send(
            'org.freedesktop.DBus',
            'org.freedesktop.DBus',
            '/org/freedesktop/DBus',
            'ListNames',
            args=None,
            host=host,
            timeout_seconds=2,
            tolerate_failures=False)

    return 'org.chromium.Atrusctl' in result.response

def force_upgrade_atrus(host):
    """
    Executes a forced upgrade of the Atrus device.

    @param host: Host object to run a particular host.
    @returns True if the upgrade succeeded. False otherwise.
    """
    args = [dbus.String('/lib/firmware/google/atrus-fw-bundle-latest.bin')]
    result = dbus_send.dbus_send(
            'org.chromium.Atrusctl',
            'org.chromium.Atrusctl',
            '/org/chromium/Atrusctl',
            'ForceFirmwareUpgrade',
            args=args,
            host=host,
            timeout_seconds=90,
            tolerate_failures=True)

    return result.response

def wait_for_atrus_enumeration(host, timeout_seconds=30):
    """
    Wait for an Atrus device to enumerate.

    @param host: Host object to run a particular host.
    @param timeout_seconds: Maximum number of seconds to wait.
    """
    device_manager = usb_device_collector.UsbDeviceCollector(host)

    utils.poll_for_condition(
            lambda: len(device_manager.get_devices_by_spec(ATRUS)) >= 1,
            timeout=timeout_seconds,
            sleep_interval=1.0,
            desc='No atrus device enumerated')
