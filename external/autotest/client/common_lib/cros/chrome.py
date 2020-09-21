# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, re

from autotest_lib.client.common_lib.cros import arc_util
from autotest_lib.client.cros import constants
from autotest_lib.client.bin import utils
from telemetry.core import cros_interface, exceptions, util
from telemetry.internal.browser import browser_finder, browser_options
from telemetry.internal.browser import extension_to_load

CAP_USERNAME = 'crosautotest@gmail.com'
CAP_URL = ('https://sites.google.com/a/chromium.org/dev/chromium-os'
           '/testing/cros-autotest/cap')

Error = exceptions.Error

def NormalizeEmail(username):
    """Remove dots from username. Add @gmail.com if necessary.

    TODO(achuith): Get rid of this when crbug.com/358427 is fixed.

    @param username: username/email to be scrubbed.
    """
    parts = re.split('@', username)
    parts[0] = re.sub('\.', '', parts[0])

    if len(parts) == 1:
        parts.append('gmail.com')
    return '@'.join(parts)


class Chrome(object):
    """Wrapper for creating a telemetry browser instance with extensions.

    The recommended way to use this class is to create the instance using the
    with statement:

    >>> with chrome.Chrome(...) as cr:
    >>>     # Do whatever you need with cr.
    >>>     pass

    This will make sure all the clean-up functions are called.  If you really
    need to use this class without the with statement, make sure to call the
    close() method once you're done with the Chrome instance.
    """


    BROWSER_TYPE_LOGIN = 'system'
    BROWSER_TYPE_GUEST = 'system-guest'


    def __init__(self, logged_in=True, extension_paths=None, autotest_ext=False,
                 num_tries=3, extra_browser_args=None,
                 clear_enterprise_policy=True, expect_policy_fetch=False,
                 dont_override_profile=False, disable_gaia_services=True,
                 disable_default_apps = True, auto_login=True, gaia_login=False,
                 username=None, password=None, gaia_id=None,
                 arc_mode=None, disable_arc_opt_in=True,
                 init_network_controller=False, login_delay=0):
        """
        Constructor of telemetry wrapper.

        @param logged_in: Regular user (True) or guest user (False).
        @param extension_paths: path of unpacked extension to install.
        @param autotest_ext: Load a component extension with privileges to
                             invoke chrome.autotestPrivate.
        @param num_tries: Number of attempts to log in.
        @param extra_browser_args: Additional argument(s) to pass to the
                                   browser. It can be a string or a list.
        @param clear_enterprise_policy: Clear enterprise policy before
                                        logging in.
        @param expect_policy_fetch: Expect that chrome can reach the device
                                    management server and download policy.
        @param dont_override_profile: Don't delete cryptohome before login.
                                      Telemetry will output a warning with this
                                      option.
        @param disable_gaia_services: For enterprise autotests, this option may
                                      be used to enable policy fetch.
        @param disable_default_apps: For tests that exercise default apps.
        @param auto_login: Does not login automatically if this is False.
                           Useful if you need to examine oobe.
        @param gaia_login: Logs in to real gaia.
        @param username: Log in using this username instead of the default.
        @param password: Log in using this password instead of the default.
        @param gaia_id: Log in using this gaia_id instead of the default.
        @param arc_mode: How ARC instance should be started.  Default is to not
                         start.
        @param disable_arc_opt_in: For opt in flow autotest. This option is used
                                   to disable the arc opt in flow.
        @param login_delay: Time for idle in login screen to simulate the time
                            required for password typing.
        """
        self._autotest_ext_path = None

        # Force autotest extension if we need enable Play Store.
        if (utils.is_arc_available() and (arc_util.should_start_arc(arc_mode)
            or not disable_arc_opt_in)):
            autotest_ext = True

        if extension_paths is None:
            extension_paths = []

        if autotest_ext:
            self._autotest_ext_path = os.path.join(os.path.dirname(__file__),
                                                   'autotest_private_ext')
            extension_paths.append(self._autotest_ext_path)

        finder_options = browser_options.BrowserFinderOptions()
        if utils.is_arc_available() and arc_util.should_start_arc(arc_mode):
            if disable_arc_opt_in:
                finder_options.browser_options.AppendExtraBrowserArgs(
                        arc_util.get_extra_chrome_flags())
            logged_in = True

        self._browser_type = (self.BROWSER_TYPE_LOGIN
                if logged_in else self.BROWSER_TYPE_GUEST)
        finder_options.browser_type = self.browser_type
        if extra_browser_args:
            finder_options.browser_options.AppendExtraBrowserArgs(
                    extra_browser_args)

        # finder options must be set before parse_args(), browser options must
        # be set before Create().
        # TODO(crbug.com/360890) Below MUST be '2' so that it doesn't inhibit
        # autotest debug logs
        finder_options.verbosity = 2
        finder_options.CreateParser().parse_args(args=[])
        b_options = finder_options.browser_options
        b_options.disable_component_extensions_with_background_pages = False
        b_options.create_browser_with_oobe = True
        b_options.clear_enterprise_policy = clear_enterprise_policy
        b_options.dont_override_profile = dont_override_profile
        b_options.disable_gaia_services = disable_gaia_services
        b_options.disable_default_apps = disable_default_apps
        b_options.disable_component_extensions_with_background_pages = disable_default_apps
        b_options.disable_background_networking = False
        b_options.expect_policy_fetch = expect_policy_fetch

        b_options.auto_login = auto_login
        b_options.gaia_login = gaia_login
        b_options.login_delay = login_delay

        if utils.is_arc_available() and not disable_arc_opt_in:
            arc_util.set_browser_options_for_opt_in(b_options)

        self.username = b_options.username if username is None else username
        self.password = b_options.password if password is None else password
        self.username = NormalizeEmail(self.username)
        b_options.username = self.username
        b_options.password = self.password
        self.gaia_id = b_options.gaia_id if gaia_id is None else gaia_id
        b_options.gaia_id = self.gaia_id

        self.arc_mode = arc_mode

        if logged_in:
            extensions_to_load = b_options.extensions_to_load
            for path in extension_paths:
                extension = extension_to_load.ExtensionToLoad(
                        path, self.browser_type)
                extensions_to_load.append(extension)
            self._extensions_to_load = extensions_to_load

        # Turn on collection of Chrome coredumps via creation of a magic file.
        # (Without this, Chrome coredumps are trashed.)
        open(constants.CHROME_CORE_MAGIC_FILE, 'w').close()

        self._browser_to_create = browser_finder.FindBrowser(
            finder_options)
        self._browser_to_create.SetUpEnvironment(b_options)
        for i in range(num_tries):
            try:
                self._browser = self._browser_to_create.Create()
                self._browser_pid = \
                    cros_interface.CrOSInterface().GetChromePid()
                if utils.is_arc_available():
                    if disable_arc_opt_in:
                        if arc_util.should_start_arc(arc_mode):
                            arc_util.enable_play_store(self.autotest_ext, True)
                    else:
                        arc_util.opt_in(self.browser, self.autotest_ext)
                    arc_util.post_processing_after_browser(self)
                break
            except exceptions.LoginException as e:
                logging.error('Timed out logging in, tries=%d, error=%s',
                              i, repr(e))
                if i == num_tries-1:
                    raise
        if init_network_controller:
          self._browser.platform.network_controller.Open()

    def __enter__(self):
        return self


    def __exit__(self, *args):
        self.close()


    @property
    def browser(self):
        """Returns a telemetry browser instance."""
        return self._browser


    def get_extension(self, extension_path):
        """Fetches a telemetry extension instance given the extension path."""
        for ext in self._extensions_to_load:
            if extension_path == ext.path:
                return self.browser.extensions[ext]
        return None


    @property
    def autotest_ext(self):
        """Returns the autotest extension."""
        return self.get_extension(self._autotest_ext_path)


    @property
    def login_status(self):
        """Returns login status."""
        ext = self.autotest_ext
        if not ext:
            return None

        ext.ExecuteJavaScript('''
            window.__login_status = null;
            chrome.autotestPrivate.loginStatus(function(s) {
              window.__login_status = s;
            });
        ''')
        return ext.EvaluateJavaScript('window.__login_status')


    def get_visible_notifications(self):
        """Returns an array of visible notifications of Chrome.

        For specific type of each notification, please refer to Chromium's
        chrome/common/extensions/api/autotest_private.idl.
        """
        ext = self.autotest_ext
        if not ext:
            return None

        ext.ExecuteJavaScript('''
            window.__items = null;
            chrome.autotestPrivate.getVisibleNotifications(function(items) {
              window.__items  = items;
            });
        ''')
        if ext.EvaluateJavaScript('window.__items') is None:
            return None
        return ext.EvaluateJavaScript('window.__items')


    @property
    def browser_type(self):
        """Returns the browser_type."""
        return self._browser_type


    @staticmethod
    def did_browser_crash(func):
        """Runs func, returns True if the browser crashed, False otherwise.

        @param func: function to run.

        """
        try:
            func()
        except Error:
            return True
        return False


    @staticmethod
    def wait_for_browser_restart(func, browser):
        """Runs func, and waits for a browser restart.

        @param func: function to run.

        """
        _cri = cros_interface.CrOSInterface()
        pid = _cri.GetChromePid()
        Chrome.did_browser_crash(func)
        utils.poll_for_condition(lambda: pid != _cri.GetChromePid(), timeout=60)
        browser.WaitForBrowserToComeUp()


    def wait_for_browser_to_come_up(self):
        """Waits for the browser to come up. This should only be called after a
        browser crash.
        """
        def _BrowserReady(cr):
            tabs = []  # Wrapper for pass by reference.
            if self.did_browser_crash(
                    lambda: tabs.append(cr.browser.tabs.New())):
                return False
            try:
                tabs[0].Close()
            except:
                # crbug.com/350941
                logging.error('Timed out closing tab')
            return True
        util.WaitFor(lambda: _BrowserReady(self), timeout=10)


    def close(self):
        """Closes the browser.
        """
        try:
            if utils.is_arc_available():
                arc_util.pre_processing_before_close(self)
        finally:
            # Calling platform.StopAllLocalServers() to tear down the telemetry
            # server processes such as the one started by
            # platform.SetHTTPServerDirectories().  Not calling this function
            # will leak the process and may affect test results.
            # (crbug.com/663387)
            self._browser.platform.StopAllLocalServers()
            self._browser.Close()
            self._browser_to_create.CleanUpEnvironment()
            self._browser.platform.network_controller.Close()
