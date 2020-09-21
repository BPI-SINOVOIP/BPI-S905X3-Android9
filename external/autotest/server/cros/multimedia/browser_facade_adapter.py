# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""An adapter to remotely access the browser facade on DUT."""


class BrowserFacadeRemoteAdapter(object):
    """BrowserFacadeRemoteAdapter is an adapter to remotely control DUT browser.

    The Autotest host object representing the remote DUT, passed to this
    class on initialization, can be accessed from its _client property.

    """
    def __init__(self, remote_facade_proxy):
        """Construct an BrowserFacadeRemoteAdapter.

        @param remote_facade_proxy: RemoteFacadeProxy object.

        """
        self._proxy = remote_facade_proxy


    @property
    def _browser_proxy(self):
        """Gets the proxy to DUT browser facade.

        @return XML RPC proxy to DUT browser facade.

        """
        return self._proxy.browser


    def start_custom_chrome(self, kwargs):
        """Start a custom Chrome with given arguments.

        @param kwargs: A dict of keyword arguments passed to Chrome.
        @return: True on success, False otherwise.

        """
        return self._browser_proxy.start_custom_chrome(kwargs)


    def start_default_chrome(self, restart=False, extra_browser_args=None):
        """Start the default Chrome.

        @param restart: True to start Chrome without clearing previous state.
        @param extra_browser_args: A list containing extra browser args passed
                                   to Chrome in addition to default ones.
        @return: True on success, False otherwise.

        """
        return self._browser_proxy.start_default_chrome(
                restart, extra_browser_args)


    def new_tab(self, url):
        """Opens a new tab and loads URL.

        @param url: The URL to load.
        @return a str, the tab descriptor of the opened tab.

        """
        return self._browser_proxy.new_tab(url)


    def close_tab(self, tab_descriptor):
        """Closes a previously opened tab.

        @param tab_descriptor: Indicate which tab to be closed.

        """
        self._browser_proxy.close_tab(tab_descriptor)


    def wait_for_javascript_expression(
            self, tab_descriptor, expression, timeout):
        """Waits for the given JavaScript expression to be True on the given tab

        @param tab_descriptor: Indicate on which tab to wait for the expression.
        @param expression: Indiate for what expression to wait.
        @param timeout: Indicate the timeout of the expression.
        """
        self._browser_proxy.wait_for_javascript_expression(
                tab_descriptor, expression, timeout)


    def execute_javascript(self, tab_descriptor, statement, timeout):
        """Executes a JavaScript statement on the given tab.

        @param tab_descriptor: Indicate on which tab to execute the statement.
        @param statement: Indiate what statement to execute.
        @param timeout: Indicate the timeout of the statement.
        """
        self._browser_proxy.execute_javascript(
                tab_descriptor, statement, timeout)


    def evaluate_javascript(self, tab_descriptor, expression, timeout):
        """Evaluates a JavaScript expression on the given tab.

        @param tab_descriptor: Indicate on which tab to evaluate the expression.
        @param expression: Indiate what expression to evaluate.
        @param timeout: Indicate the timeout of the expression.
        @return the JSONized result of the given expression
        """
        return self._browser_proxy.evaluate_javascript(
                tab_descriptor, expression, timeout)
