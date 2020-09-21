# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import socket

import common
from autotest_lib.client.common_lib import hosts
from autotest_lib.server import utils


class SshVerifier(hosts.Verifier):
    """
    Verifier to test a host's accessibility via `ssh`.

    This verifier checks whether a given host is reachable over `ssh`.
    In the event of failure, it distinguishes one of three distinct
    conditions:
      * The host can't be found with a DNS lookup.
      * The host doesn't answer to ping.
      * The host answers to ping, but not to ssh.
    """

    def verify(self, host):
        if host.is_up():
            return
        msg = 'No answer to ssh from %s'
        try:
            socket.gethostbyname(host.hostname)
        except Exception as e:
            logging.exception('DNS lookup failure')
            msg = 'Unable to look up %%s in DNS: %s' % e
        else:
            if utils.ping(host.hostname, tries=1, deadline=1) != 0:
                msg = 'No answer to ping from %s'
        raise hosts.AutoservVerifyError(msg % host.hostname)


    @property
    def description(self):
        return 'host is available via ssh'


class LegacyHostVerifier(hosts.Verifier):
    """
    Ask a Host instance to perform its own verification.

    This class exists as a temporary legacy during refactoring to
    provide access to code that hasn't yet been rewritten to use the new
    repair and verify framework.
    """

    def verify(self, host):
        host.verify_software()
        host.verify_hardware()


    @property
    def description(self):
        return 'Legacy host verification checks'


class RebootRepair(hosts.RepairAction):
    """Repair a target device by rebooting it."""

    def repair(self, host):
        host.reboot()


    @property
    def description(self):
        return 'Reboot the host'


class RPMCycleRepair(hosts.RepairAction):
    """
    Cycle AC power using the RPM infrastructure.

    This is meant to catch two distinct kinds of failure:
      * If the target has no battery (that is, a chromebox), power
        cycling it may force it back on.
      * If the target has a batter that is discharging or even fully
        drained, power cycling will leave power on, enabling other
        repair procedures.
    """

    def repair(self, host):
        if not host.has_power():
            raise hosts.AutoservRepairError(
                    '%s has no RPM connection.' % host.hostname)
        host.power_cycle()
        if not host.wait_up(timeout=host.BOOT_TIMEOUT):
            raise hosts.AutoservRepairError(
                    '%s is still offline after powercycling' %
                    host.hostname)


    @property
    def description(self):
        return 'Power cycle the host with RPM'
