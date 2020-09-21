#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import time
from acts.controllers.utils_lib.commands import shell

_ROUTER_DNS = '8.8.8.8, 4.4.4.4'


class Error(Exception):
    """An error caused by the dhcp server."""


class NoInterfaceError(Exception):
    """Error thrown when the dhcp server has no interfaces on any subnet."""


class DhcpServer(object):
    """Manages the dhcp server program.

    Only one of these can run in an environment at a time.

    Attributes:
        config: The dhcp server configuration that is being used.
    """

    PROGRAM_FILE = 'dhcpd'

    def __init__(self, runner, interface, working_dir='/tmp'):
        """
        Args:
            runner: Object that has a run_async and run methods for running
                    shell commands.
            interface: string, The name of the interface to use.
            working_dir: The directory to work out of.
        """
        self._runner = runner
        self._working_dir = working_dir
        self._shell = shell.ShellCommand(runner, working_dir)
        self._log_file = 'dhcpd_%s.log' % interface
        self._config_file = 'dhcpd_%s.conf' % interface
        self._lease_file = 'dhcpd_%s.leases' % interface
        self._identifier = '%s.*%s' % (self.PROGRAM_FILE, self._config_file)

    def start(self, config, timeout=60):
        """Starts the dhcp server.

        Starts the dhcp server daemon and runs it in the background.

        Args:
            config: dhcp_config.DhcpConfig, Configs to start the dhcp server
                    with.

        Returns:
            True if the daemon could be started. Note that the daemon can still
            start and not work. Invalid configurations can take a long amount
            of time to be produced, and because the daemon runs indefinitely
            it's infeasible to wait on. If you need to check if configs are ok
            then periodic checks to is_running and logs should be used.
        """
        if self.is_alive():
            self.stop()

        self._write_configs(config)
        self._shell.delete_file(self._log_file)
        self._shell.touch_file(self._lease_file)

        dhcpd_command = '%s -cf "%s" -lf %s -f""' % (self.PROGRAM_FILE,
                                                     self._config_file,
                                                     self._lease_file)
        base_command = 'cd "%s"; %s' % (self._working_dir, dhcpd_command)
        job_str = '%s > "%s" 2>&1' % (base_command, self._log_file)
        self._runner.run_async(job_str)

        try:
            self._wait_for_process(timeout=timeout)
            self._wait_for_server(timeout=timeout)
        except:
            self.stop()
            raise

    def stop(self):
        """Kills the daemon if it is running."""
        self._shell.kill(self._identifier)

    def is_alive(self):
        """
        Returns:
            True if the daemon is running.
        """
        return self._shell.is_alive(self._identifier)

    def get_logs(self):
        """Pulls the log files from where dhcp server is running.

        Returns:
            A string of the dhcp server logs.
        """
        return self._shell.read_file(self._log_file)

    def _wait_for_process(self, timeout=60):
        """Waits for the process to come up.

        Waits until the dhcp server process is found running, or there is
        a timeout. If the program never comes up then the log file
        will be scanned for errors.

        Raises: See _scan_for_errors
        """
        start_time = time.time()
        while time.time() - start_time < timeout and not self.is_alive():
            self._scan_for_errors(False)
            time.sleep(0.1)

        self._scan_for_errors(True)

    def _wait_for_server(self, timeout=60):
        """Waits for dhcp server to report that the server is up.

        Waits until dhcp server says the server has been brought up or an
        error occurs.

        Raises: see _scan_for_errors
        """
        start_time = time.time()
        while time.time() - start_time < timeout:
            success = self._shell.search_file(
                'Wrote [0-9]* leases to leases file', self._log_file)
            if success:
                return

            self._scan_for_errors(True)

    def _scan_for_errors(self, should_be_up):
        """Scans the dhcp server log for any errors.

        Args:
            should_be_up: If true then dhcp server is expected to be alive.
                          If it is found not alive while this is true an error
                          is thrown.

        Raises:
            Error: Raised when a dhcp server error is found.
        """
        # If this is checked last we can run into a race condition where while
        # scanning the log the process has not died, but after scanning it
        # has. If this were checked last in that condition then the wrong
        # error will be thrown. To prevent this we gather the alive state first
        # so that if it is dead it will definitely give the right error before
        # just giving a generic one.
        is_dead = not self.is_alive()

        no_interface = self._shell.search_file(
            'Not configured to listen on any interfaces', self._log_file)
        if no_interface:
            raise NoInterfaceError(
                'Dhcp does not contain a subnet for any of the networks the'
                ' current interfaces are on.')

        if should_be_up and is_dead:
            raise Error('Dhcp server failed to start.', self)

    def _write_configs(self, config):
        """Writes the configs to the dhcp server config file."""

        self._shell.delete_file(self._config_file)

        lines = []

        if config.default_lease_time:
            lines.append('default-lease-time %d;' % config.default_lease_time)
        if config.max_lease_time:
            lines.append('max-lease-time %s;' % config.max_lease_time)

        for subnet in config.subnets:
            address = subnet.network.network_address
            mask = subnet.network.netmask
            router = subnet.router
            start = subnet.start
            end = subnet.end
            lease_time = subnet.lease_time

            lines.append('subnet %s netmask %s {' % (address, mask))
            lines.append('\toption subnet-mask %s;' % mask)
            lines.append('\toption routers %s;' % router)
            lines.append('\toption domain-name-servers %s;' % _ROUTER_DNS)
            lines.append('\trange %s %s;' % (start, end))
            if lease_time:
                lines.append('\tdefault-lease-time %d;' % lease_time)
                lines.append('\tmax-lease-time %d;' % lease_time)
            lines.append('}')

        for mapping in config.static_mappings:
            identifier = mapping.identifier
            fixed_address = mapping.ipv4_address
            host_fake_name = 'host%s' % identifier.replace(':', '')
            lease_time = mapping.lease_time

            lines.append('host %s {' % host_fake_name)
            lines.append('\thardware ethernet %s;' % identifier)
            lines.append('\tfixed-address %s;' % fixed_address)
            if lease_time:
                lines.append('\tdefault-lease-time %d;' % lease_time)
                lines.append('\tmax-lease-time %d;' % lease_time)
            lines.append('}')

        config_str = '\n'.join(lines)

        self._shell.write_file(self._config_file, config_str)
