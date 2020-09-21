# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import math
import re

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error

PLATFORM_LINUX = 'LINUX'
PLATFORM_MACOS = 'MAC_OS'


def _get_platform_delegate(platform):
    if platform == PLATFORM_LINUX:
        return LinuxPingDelegate
    elif platform == PLATFORM_MACOS:
        return MacPingDelegate
    else:
      raise error.TestError('%s is not a valid platform type', platform)


def _regex_int_from_string(pattern, line):
    """Retrieve an integer from a string, using regex.

    @param pattern: The regular expression to apply to the input string.
    @param line: String input to retrieve an integer from.
    @return integer retrieved from the input string, or None if there is no
        match.
    """
    m = re.search(pattern, line)
    if m is not None:
        return int(m.group(1))
    return None


def _regex_float_from_string(pattern, line):
    """Retrieve a float from a string, using regex.

    @param pattern: The regular expression to apply to the input string.
    @param line: String input to retrieve a float from.
    @return float retrieved from the input string, or None if there is no
        match.
    """
    m = re.search(pattern, line)
    if m is not None:
        return float(m.group(1))
    return None


class MacPingDelegate(object):
    """Implement ping functionality for MacOS hosts."""

    @staticmethod
    def ping_arguments(ping_config):
        """
        @param ping_config PingConfig object describing the ping test for which
           arguments are needed.
        @return list of parameters to ping.
        """
        args = []
        args.append('-c %d' % ping_config.count)
        if ping_config.size is not None:
            args.append('-s %d' % ping_config.size)
        if ping_config.interval is not None:
            args.append('-i %f' % ping_config.interval)
        if ping_config.qos is not None:
            if ping_config.qos == 'be':
                ping_config.append('-k 0')
            elif ping_config.qos == 'bk':
                ping_config.append('-k 1')
            elif ping_config.qos == 'vi':
                args.append('-k 2')
            elif ping_config.qos == 'vo':
                args.append('-k 3')
            else:
                raise error.TestFail('Unknown QoS value: %s' % ping_config.qos)

        # The last argument is the IP address to ping.
        args.append(ping_config.target_ip)
        return args


    @staticmethod
    def parse_from_output(ping_output):
        """Extract the ping results from stdout.

        @param ping_output string stdout from a ping/ping6 command.

        stdout from ping command looks like:

        PING 8.8.8.8 (8.8.8.8): 56 data bytes
        64 bytes from 8.8.8.8: icmp_seq=0 ttl=57 time=3.770 ms
        64 bytes from 8.8.8.8: icmp_seq=1 ttl=57 time=4.165 ms
        64 bytes from 8.8.8.8: icmp_seq=2 ttl=57 time=4.901 ms

        --- 8.8.8.8 ping statistics ---
        3 packets transmitted, 3 packets received, 0.0% packet loss
        round-trip min/avg/max/stddev = 3.770/4.279/4.901/0.469 ms

        stdout from ping6 command looks like:

        16 bytes from fdd2:8741:1993:8::, icmp_seq=16 hlim=64 time=1.783 ms
        16 bytes from fdd2:8741:1993:8::, icmp_seq=17 hlim=64 time=2.150 ms
        16 bytes from fdd2:8741:1993:8::, icmp_seq=18 hlim=64 time=2.516 ms
        16 bytes from fdd2:8741:1993:8::, icmp_seq=19 hlim=64 time=1.401 ms

        --- fdd2:8741:1993:8:: ping6 statistics ---
        20 packets transmitted, 20 packets received, 0.0% packet loss
        round-trip min/avg/max/std-dev = 1.401/2.122/3.012/0.431 ms

        This function will look for both 'stdev' and 'std-dev' in test results
        to support both ping and ping6 commands.
        """
        loss_line = (filter(lambda x: x.find('packets transmitted') > 0,
                            ping_output.splitlines()) or [''])[0]
        sent = _regex_int_from_string('([0-9]+) packets transmitted', loss_line)
        received = _regex_int_from_string('([0-9]+) packets received',
                                          loss_line)
        loss = _regex_float_from_string('([0-9]+\.[0-9]+)% packet loss',
                                        loss_line)
        if None in (sent, received, loss):
            raise error.TestFail('Failed to parse transmission statistics.')
        m = re.search('round-trip min\/avg\/max\/std-?dev = ([0-9.]+)\/([0-9.]+)'
                      '\/([0-9.]+)\/([0-9.]+) ms', ping_output)
        if m is not None:
            return PingResult(sent, received, loss,
                              min_latency=float(m.group(1)),
                              avg_latency=float(m.group(2)),
                              max_latency=float(m.group(3)),
                              dev_latency=float(m.group(4)))
        if received > 0:
            raise error.TestFail('Failed to parse latency statistics.')

        return PingResult(sent, received, loss)


class LinuxPingDelegate(object):
    """Implement ping functionality specific to the linux platform."""
    @staticmethod
    def ping_arguments(ping_config):
        """
        @param ping_config PingConfig object describing the ping test for which
           arguments are needed.
        @return list of parameters to ping.
        """
        args = []
        args.append('-c %d' % ping_config.count)
        if ping_config.size is not None:
            args.append('-s %d' % ping_config.size)
        if ping_config.interval is not None:
            args.append('-i %f' % ping_config.interval)
        if ping_config.qos is not None:
            if ping_config.qos == 'be':
                args.append('-Q 0x04')
            elif ping_config.qos == 'bk':
                args.append('-Q 0x02')
            elif ping_config.qos == 'vi':
                args.append('-Q 0x08')
            elif ping_config.qos == 'vo':
                args.append('-Q 0x10')
            else:
                raise error.TestFail('Unknown QoS value: %s' % ping_config.qos)

        # The last argument is the IP address to ping.
        args.append(ping_config.target_ip)
        return args


    @staticmethod
    def parse_from_output(ping_output):
        """Extract the ping results from stdout.

        @param ping_output string stdout from a ping command.
        On error, some statistics may be missing entirely from the output.

        An example of output with some errors is:

        PING 192.168.0.254 (192.168.0.254) 56(84) bytes of data.
        From 192.168.0.124 icmp_seq=1 Destination Host Unreachable
        From 192.168.0.124 icmp_seq=2 Destination Host Unreachable
        From 192.168.0.124 icmp_seq=3 Destination Host Unreachable
        64 bytes from 192.168.0.254: icmp_req=4 ttl=64 time=1171 ms
        [...]
        64 bytes from 192.168.0.254: icmp_req=10 ttl=64 time=1.95 ms

        --- 192.168.0.254 ping statistics ---
        10 packets transmitted, 7 received, +3 errors, 30% packet loss,
            time 9007ms
        rtt min/avg/max/mdev = 1.806/193.625/1171.174/403.380 ms, pipe 3

        A more normal run looks like:

        PING google.com (74.125.239.137) 56(84) bytes of data.
        64 bytes from 74.125.239.137: icmp_req=1 ttl=57 time=1.77 ms
        64 bytes from 74.125.239.137: icmp_req=2 ttl=57 time=1.78 ms
        [...]
        64 bytes from 74.125.239.137: icmp_req=5 ttl=57 time=1.79 ms

        --- google.com ping statistics ---
        5 packets transmitted, 5 received, 0% packet loss, time 4007ms
        rtt min/avg/max/mdev = 1.740/1.771/1.799/0.042 ms

        We also sometimes see result lines like:
        9 packets transmitted, 9 received, +1 duplicates, 0% packet loss,
            time 90 ms

        """
        loss_line = (filter(lambda x: x.find('packets transmitted') > 0,
                            ping_output.splitlines()) or [''])[0]
        sent = _regex_int_from_string('([0-9]+) packets transmitted', loss_line)
        received = _regex_int_from_string('([0-9]+) received', loss_line)
        loss = _regex_int_from_string('([0-9]+)% packet loss', loss_line)
        if None in (sent, received, loss):
            raise error.TestFail('Failed to parse transmission statistics.')

        m = re.search('(round-trip|rtt) min[^=]*= '
                      '([0-9.]+)/([0-9.]+)/([0-9.]+)/([0-9.]+)', ping_output)
        if m is not None:
            return PingResult(sent, received, loss,
                              min_latency=float(m.group(2)),
                              avg_latency=float(m.group(3)),
                              max_latency=float(m.group(4)),
                              dev_latency=float(m.group(5)))
        if received > 0:
            raise error.TestFail('Failed to parse latency statistics.')

        return PingResult(sent, received, loss)


class PingConfig(object):
    """Describes the parameters for a ping command."""

    DEFAULT_COUNT = 10
    PACKET_WAIT_MARGIN_SECONDS = 120

    def __init__(self, target_ip, count=DEFAULT_COUNT, size=None,
                 interval=None, qos=None,
                 ignore_status=False, ignore_result=False):
        super(PingConfig, self).__init__()
        self.target_ip = target_ip
        self.count = count
        self.size = size
        self.interval = interval
        if qos:
            qos = qos.lower()
        self.qos = qos
        self.ignore_status = ignore_status
        self.ignore_result = ignore_result
        interval_seconds = self.interval or 1
        command_time = math.ceil(interval_seconds * self.count)
        self.command_timeout_seconds = int(command_time +
                                           self.PACKET_WAIT_MARGIN_SECONDS)


class PingResult(object):
    """Represents a parsed ping command result."""
    def __init__(self, sent, received, loss,
                 min_latency=-1.0, avg_latency=-1.0,
                 max_latency=-1.0, dev_latency=-1.0):
        """Construct a PingResult.

        @param sent: int number of packets sent.
        @param received: int number of replies received.
        @param loss: int loss as a percentage (0-100)
        @param min_latency: float min response latency in ms.
        @param avg_latency: float average response latency in ms.
        @param max_latency: float max response latency in ms.
        @param dev_latency: float response latency deviation in ms.

        """
        super(PingResult, self).__init__()
        self.sent = sent
        self.received = received
        self.loss = loss
        self.min_latency = min_latency
        self.avg_latency = avg_latency
        self.max_latency = max_latency
        self.dev_latency = dev_latency


    def __repr__(self):
        return '%s(%s)' % (self.__class__.__name__,
                           ', '.join(['%s=%r' % item
                                      for item in vars(self).iteritems()]))


class PingRunner(object):
    """Delegate to run the ping command on a local or remote host."""
    DEFAULT_PING_COMMAND = 'ping'
    PING_LOSS_THRESHOLD = 20  # A percentage.


    def __init__(self, command_ping=DEFAULT_PING_COMMAND, host=None,
                 platform=PLATFORM_LINUX):
        """Construct a PingRunner.

        @param command_ping optional path or alias of the ping command.
        @param host optional host object when a remote host is desired.

        """
        super(PingRunner, self).__init__()
        self._run = utils.run
        if host is not None:
            self._run = host.run
        self.command_ping = command_ping
        self._platform_delegate = _get_platform_delegate(platform)


    def simple_ping(self, host_name):
        """Quickly test that a hostname or IPv4 address responds to ping.

        @param host_name: string name or IPv4 address.
        @return True if host_name responds to at least one ping.

        """
        ping_config = PingConfig(host_name, count=3, interval=0.5,
                                 ignore_status=True, ignore_result=True)
        ping_result = self.ping(ping_config)
        if ping_result is None or ping_result.received == 0:
            return False
        return True


    def ping(self, ping_config):
        """Run ping with the given |ping_config|.

        Will assert that the ping had reasonable levels of loss unless
        requested not to in |ping_config|.

        @param ping_config PingConfig object describing the ping to run.

        """
        command_pieces = ([self.command_ping] +
                          self._platform_delegate.ping_arguments(ping_config))
        command = ' '.join(command_pieces)
        command_result = self._run(command,
                                   timeout=ping_config.command_timeout_seconds,
                                   ignore_status=True,
                                   ignore_timeout=True)
        if not command_result:
            if ping_config.ignore_status:
                logging.warning('Ping command timed out; cannot parse output.')
                return PingResult(ping_config.count, 0, 100)

            raise error.TestFail('Ping command timed out unexpectedly.')

        if not command_result.stdout:
            logging.warning('Ping command returned no output; stderr was %s.',
                            command_result.stderr)
            if ping_config.ignore_result:
                return PingResult(ping_config.count, 0, 100)
            raise error.TestFail('Ping command failed to yield any output')

        if command_result.exit_status and not ping_config.ignore_status:
            raise error.TestFail('Ping command failed with code=%d' %
                                 command_result.exit_status)

        ping_result = self._platform_delegate.parse_from_output(
                command_result.stdout)
        if ping_config.ignore_result:
            return ping_result

        if ping_result.loss > self.PING_LOSS_THRESHOLD:
            raise error.TestFail('Lost ping packets: %r.' % ping_result)

        logging.info('Ping successful.')
        return ping_result
