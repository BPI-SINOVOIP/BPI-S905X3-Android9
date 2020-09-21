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

import collections
import itertools
import logging
import os
import time

from acts.controllers.ap_lib import hostapd_config
from acts.controllers.utils_lib.commands import shell


class Error(Exception):
    """An error caused by hostapd."""


class Hostapd(object):
    """Manages the hostapd program.

    Attributes:
        config: The hostapd configuration that is being used.
    """

    PROGRAM_FILE = '/usr/sbin/hostapd'

    def __init__(self, runner, interface, working_dir='/tmp'):
        """
        Args:
            runner: Object that has run_async and run methods for executing
                    shell commands (e.g. connection.SshConnection)
            interface: string, The name of the interface to use (eg. wlan0).
            working_dir: The directory to work out of.
        """
        self._runner = runner
        self._interface = interface
        self._working_dir = working_dir
        self.config = None
        self._shell = shell.ShellCommand(runner, working_dir)
        self._log_file = 'hostapd-%s.log' % self._interface
        self._ctrl_file = 'hostapd-%s.ctrl' % self._interface
        self._config_file = 'hostapd-%s.conf' % self._interface
        self._identifier = '%s.*%s' % (self.PROGRAM_FILE, self._config_file)

    def start(self, config, timeout=60, additional_parameters=None):
        """Starts hostapd

        Starts the hostapd daemon and runs it in the background.

        Args:
            config: Configs to start the hostapd with.
            timeout: Time to wait for DHCP server to come up.
            additional_parameters: A dictionary of parameters that can sent
                                   directly into the hostapd config file.  This
                                   can be used for debugging and or adding one
                                   off parameters into the config.

        Returns:
            True if the daemon could be started. Note that the daemon can still
            start and not work. Invalid configurations can take a long amount
            of time to be produced, and because the daemon runs indefinitely
            it's impossible to wait on. If you need to check if configs are ok
            then periodic checks to is_running and logs should be used.
        """
        if self.is_alive():
            self.stop()

        self.config = config

        self._shell.delete_file(self._ctrl_file)
        self._shell.delete_file(self._log_file)
        self._shell.delete_file(self._config_file)
        self._write_configs(additional_parameters=additional_parameters)

        hostapd_command = '%s -dd -t "%s"' % (self.PROGRAM_FILE,
                                              self._config_file)
        base_command = 'cd "%s"; %s' % (self._working_dir, hostapd_command)
        job_str = '%s > "%s" 2>&1' % (base_command, self._log_file)
        self._runner.run_async(job_str)

        try:
            self._wait_for_process(timeout=timeout)
            self._wait_for_interface(timeout=timeout)
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

    def pull_logs(self):
        """Pulls the log files from where hostapd is running.

        Returns:
            A string of the hostapd logs.
        """
        # TODO: Auto pulling of logs when stop is called.
        return self._shell.read_file(self._log_file)

    def _wait_for_process(self, timeout=60):
        """Waits for the process to come up.

        Waits until the hostapd process is found running, or there is
        a timeout. If the program never comes up then the log file
        will be scanned for errors.

        Raises: See _scan_for_errors
        """
        start_time = time.time()
        while time.time() - start_time < timeout and not self.is_alive():
            self._scan_for_errors(False)
            time.sleep(0.1)

    def _wait_for_interface(self, timeout=60):
        """Waits for hostapd to report that the interface is up.

        Waits until hostapd says the interface has been brought up or an
        error occurs.

        Raises: see _scan_for_errors
        """
        start_time = time.time()
        while time.time() - start_time < timeout:
            success = self._shell.search_file('Setup of interface done',
                                              self._log_file)
            if success:
                return

            self._scan_for_errors(True)

    def _scan_for_errors(self, should_be_up):
        """Scans the hostapd log for any errors.

        Args:
            should_be_up: If true then hostapd program is expected to be alive.
                          If it is found not alive while this is true an error
                          is thrown.

        Raises:
            Error: Raised when a hostapd error is found.
        """
        # Store this so that all other errors have priority.
        is_dead = not self.is_alive()

        bad_config = self._shell.search_file('Interface initialization failed',
                                             self._log_file)
        if bad_config:
            raise Error('Interface failed to start', self)

        bad_config = self._shell.search_file(
            "Interface %s wasn't started" % self._interface, self._log_file)
        if bad_config:
            raise Error('Interface failed to start', self)

        if should_be_up and is_dead:
            raise Error('Hostapd failed to start', self)

    def _write_configs(self, additional_parameters=None):
        """Writes the configs to the hostapd config file."""
        self._shell.delete_file(self._config_file)

        our_configs = collections.OrderedDict()
        our_configs['interface'] = self._interface
        our_configs['ctrl_interface'] = self._ctrl_file
        packaged_configs = self.config.package_configs()
        if additional_parameters:
            packaged_configs.append(additional_parameters)

        pairs = ('%s=%s' % (k, v) for k, v in our_configs.items())
        for packaged_config in packaged_configs:
            config_pairs = ('%s=%s' % (k, v)
                            for k, v in packaged_config.items())
            pairs = itertools.chain(pairs, config_pairs)

        hostapd_conf = '\n'.join(pairs)

        logging.info('Writing %s' % self._config_file)
        logging.debug('******************Start*******************')
        logging.debug('\n%s' % hostapd_conf)
        logging.debug('*******************End********************')

        self._shell.write_file(self._config_file, hostapd_conf)
