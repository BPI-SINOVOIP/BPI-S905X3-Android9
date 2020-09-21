# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server import autotest
from autotest_lib.server.cros import tradefed_constants as constants


class ChromeLogin(object):
    """Context manager to handle Chrome login state."""

    def need_reboot(self):
        """Marks state as "dirty" - reboot needed during/after test."""
        logging.info('Will reboot DUT when Chrome stops.')
        self._need_reboot = True

    def need_restart(self):
        """Marks state as "dirty" - restart needed after test."""
        self._need_restart = True

    def __init__(self, host, kwargs):
        """Initializes the _ChromeLogin object.

        @param reboot: indicate if a reboot before destruction is required.
        @param restart: indicate if a restart before destruction is required.
        @param board: optional parameter to extend timeout for login for slow
                      DUTs. Used in particular for virtual machines.
        """
        self._host = host
        self._cts_helper_kwargs = kwargs
        # We will override reboot/restart options to some degree. Keep track
        # of them in a local copy.
        self._need_reboot = False
        if kwargs.get('reboot'):
            self.need_reboot()
        self._need_restart = False
        if kwargs.get('restart'):
            self.need_restart()
        self._timeout = constants.LOGIN_DEFAULT_TIMEOUT
        board = kwargs.get('board')
        if board in constants.LOGIN_BOARD_TIMEOUT:
            self._timeout = constants.LOGIN_BOARD_TIMEOUT[board]

    def _cmd_builder(self, verbose=False):
        """Gets remote command to start browser with ARC enabled."""
        # If autotest is not installed on the host, as with moblab at times,
        # getting the autodir will raise an exception.
        cmd = autotest.Autotest.get_installed_autodir(self._host)
        cmd += '/bin/autologin.py --arc'
        if self._cts_helper_kwargs.get('dont_override_profile'):
            logging.info('Using --dont_override_profile to start Chrome.')
            cmd += ' --dont_override_profile'
        else:
            logging.info('Not using --dont_override_profile to start Chrome.')
        if not verbose:
            cmd += ' > /dev/null 2>&1'
        return cmd

    def login(self, timeout=None, raise_exception=False, verbose=False):
        """Logs into Chrome."""
        if not timeout:
            timeout = self._timeout
        try:
            # We used to call cheets_StartAndroid, but it is a little faster to
            # call a script on the DUT. This also saves CPU time on the server.
            self._host.run(
                self._cmd_builder(),
                ignore_status=False,
                verbose=verbose,
                timeout=timeout)
            return True
        except autotest.AutodirNotFoundError:
            # Autotest is not installed (can happen on moblab after image
            # install). Fall back to logging in via client test, which as a side
            # effect installs autotest in the right place, so we should not hit
            # this slow path repeatedly.
            logging.warning('Autotest not installed, fallback to slow path...')
            try:
                autotest.Autotest(self._host).run_timed_test(
                    'cheets_StartAndroid',
                    timeout=2 * timeout,
                    check_client_result=True,
                    **self._cts_helper_kwargs)
                return True
            except:
                # We were unable to start the browser/Android. Maybe we can
                # salvage the DUT by rebooting. This can hide some failures.
                self.reboot()
                if raise_exception:
                    raise
        except:
            # We were unable to start the browser/Android. Maybe we can
            # salvage the DUT by rebooting. This can hide some failures.
            self.reboot()
            if raise_exception:
                raise
        return False

    def __enter__(self):
        """Logs into Chrome with retry."""
        timeout = self._timeout
        logging.info('Ensure Android is running (timeout=%d)...', timeout)
        if not self.login(timeout=timeout):
            timeout *= 2
            # The DUT reboots after unsuccessful login, try with more time.
            logging.info('Retrying failed login (timeout=%d)...', timeout)
            self.login(timeout=timeout, raise_exception=True, verbose=True)
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        """On exit restart the browser or reboot the machine.

        @param exc_type: Exception type if an exception is raised from the
                         with-block.
        @param exc_value: Exception instance if an exception is raised from
                          the with-block.
        @param traceback: Stack trace info if an exception is raised from
                          the with-block.
        @return None, indicating not to ignore an exception from the with-block
                if raised.
        """
        if not self._need_reboot:
            logging.info('Skipping reboot, restarting browser.')
            try:
                self.restart()
            except:
                logging.error('Restarting browser has failed.')
                self.need_reboot()
        if self._need_reboot:
            self.reboot(exc_type, exc_value, traceback)

    def restart(self):
        # We clean up /tmp (which is memory backed) from crashes and
        # other files. A reboot would have cleaned /tmp as well.
        # TODO(ihf): Remove "start ui" which is a nicety to non-ARC tests (i.e.
        # now we wait on login screen, but login() above will 'stop ui' again
        # before launching Chrome with ARC enabled).
        script = 'stop ui'
        script += '&& find /tmp/ -mindepth 1 -delete '
        script += '&& start ui'
        self._host.run(script, ignore_status=False, verbose=False, timeout=120)

    def reboot(self, exc_type=None, exc_value=None, traceback=None):
        """Reboot the machine.

        @param exc_type: Exception type if an exception is raised from the
                         with-block.
        @param exc_value: Exception instance if an exception is raised from
                          the with-block.
        @param traceback: Stack trace info if an exception is raised from
                          the with-block.
        @return None, indicating not to ignore an exception from the with-block
                if raised.
        """
        logging.info('Rebooting...')
        try:
            self._host.reboot()
            self._need_reboot = False
        except Exception:
            if exc_type is None:
                raise
            # If an exception is raise from the with-block, just record the
            # exception for the rebooting to avoid ignoring the original
            # exception.
            logging.exception('Rebooting failed.')
