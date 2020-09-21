# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging
import os.path
import pwd
import socket

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import constants, login


class security_SessionManagerDbusEndpoints(test.test):
    """Verifies SessionManager DBus endpoints are not exposed.
    """
    version = 1

    _FLAGFILE = '/tmp/security_SessionManagerDbusEndpoints_regression'


    def _set_user_environment(self, username):
        for name in ('LOGNAME', 'USER', 'LNAME', 'USERNAME'):
            if name in os.environ:
                os.environ[name] = username


    def _set_user(self, username):
        user_info = pwd.getpwnam(username)
        os.setegid(user_info[3])
        os.seteuid(user_info[2])
        self._set_user_environment(username)


    def _reset_user(self):
        uid = os.getuid()
        username = pwd.getpwuid(uid)[0]
        os.seteuid(uid)
        os.setegid(os.getgid())
        self._set_user_environment(username)


    def _ps(self, proc=constants.BROWSER):
        """Grab the oldest pid for process |proc|."""
        pscmd = 'ps -C %s -o pid --no-header | head -1' % proc
        return utils.system_output(pscmd)


    def run_once(self):
        """Main test code."""
        login.wait_for_browser()
        passed_enable_chrome_testing = self.test_enable_chrome_testing()
        passed_restart_job = self.test_restart_job()

        if not passed_enable_chrome_testing or not passed_restart_job:
            raise error.TestFail('SessionManager DBus endpoints can be abused, '
                                 'see error log')


    def test_restart_job(self):
        """Test SessionManager.RestartJob."""
        bus = dbus.SystemBus()
        proxy = bus.get_object('org.chromium.SessionManager',
                               '/org/chromium/SessionManager')
        session_manager = dbus.Interface(proxy,
                                         'org.chromium.SessionManagerInterface')

        # Craft a malicious replacement for the target process.
        cmd = ['touch', self._FLAGFILE]

        # Try to get our malicious replacement to run via RestartJob.
        try:
            remote, local = socket.socketpair(socket.AF_UNIX)
            logging.info('Calling RestartJob(<socket>, %r)', cmd)
            session_manager.RestartJob(dbus.types.UnixFd(remote), cmd)
            # Fails if the RestartJob call doesn't generate an error.
            logging.error(
                'RestartJob did not fail when passed an arbitrary command')
            return False
        except dbus.DBusException as e:
            logging.info(e.get_dbus_message())
            pass
        except OSError as e:
            raise error.TestError('Could not create sockets for creds: %s', e)
        finally:
            try:
                local.close()
            except OSError:
                pass

        if os.path.exists(self._FLAGFILE):
            logging.error('RestartJob ran an arbitrary command')
            return False

        return True


    def test_enable_chrome_testing(self):
        """Test SessionManager.EnableChromeTesting."""
        self._set_user('chronos')

        bus = dbus.SystemBus()
        proxy = bus.get_object('org.chromium.SessionManager',
                               '/org/chromium/SessionManager')
        session_manager = dbus.Interface(proxy,
                                         'org.chromium.SessionManagerInterface')

        chrome_pid = self._ps()

        # Try DBus call and make sure it fails.
        try:
            # DBus cannot infer the type of an empty Python list.
            # Pass an empty dbus.Array with the correct signature, taken from
            # platform2/login_manager/dbus_bindings/org.chromium.SessionManagerInterface.xml.
            empty_string_array = dbus.Array(signature="as")
            path = session_manager.EnableChromeTesting(True, empty_string_array,
                                                       empty_string_array)
        except dbus.exceptions.DBusException as dbe:
            logging.info(dbe)
        else:
            logging.error('EnableChromeTesting '
                          'succeeded when it should have failed')
            return False

        # Make sure Chrome didn't restart.
        if chrome_pid != self._ps():
            logging.error('Chrome restarted during test.')
            return False

        self._reset_user()
        return True
