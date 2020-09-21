# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""An interface to access the local browser facade."""

import logging

class BrowserFacadeNativeError(Exception):
    """Error in BrowserFacadeNative."""
    pass


class BrowserFacadeNative(object):
    """Facade to access the browser-related functionality."""

    def __init__(self, resource):
        """Initializes the USB facade.

        @param resource: A FacadeResource object.

        """
        self._resource = resource


    def start_custom_chrome(self, kwargs):
        """Start a custom Chrome with given arguments.

        @param kwargs: A dict of keyword arguments passed to Chrome.
        @return: True on success, False otherwise.

        """
        return self._resource.start_custom_chrome(kwargs)


    def start_default_chrome(self, restart=False, extra_browser_args=None):
        """Start the default Chrome.

        @param restart: True to start Chrome without clearing previous state.
        @param extra_browser_args: A list containing extra browser args passed
                                   to Chrome in addition to default ones.
        @return: True on success, False otherwise.

        """
        return self._resource.start_default_chrome(restart, extra_browser_args)


    def new_tab(self, url):
        """Opens a new tab and loads URL.

        @param url: The URL to load.
        @return a str, the tab descriptor of the opened tab.

        """
        logging.debug('Load URL %s', url)
        return self._resource.load_url(url)


    def close_tab(self, tab_descriptor):
        """Closes a previously opened tab.

        @param tab_descriptor: Indicate which tab to be closed.

        """
        tab = self._resource.get_tab_by_descriptor(tab_descriptor)
        logging.debug('Closing URL %s', tab.url)
        self._resource.close_tab(tab_descriptor)


    def wait_for_javascript_expression(
            self, tab_descriptor, expression, timeout):
        """Waits for the given JavaScript expression to be True on the
        given tab.

        @param tab_descriptor: Indicate on which tab to wait for the expression.
        @param expression: Indiate for what expression to wait.
        @param timeout: Indicate the timeout of the expression.
        """
        self._resource.wait_for_javascript_expression(
                tab_descriptor, expression, timeout)


    def execute_javascript(self, tab_descriptor, statement, timeout):
        """Executes a JavaScript statement on the given tab.

        @param tab_descriptor: Indicate on which tab to execute the statement.
        @param statement: Indiate what statement to execute.
        @param timeout: Indicate the timeout of the statement.
        """
        self._resource.execute_javascript(
                tab_descriptor, statement, timeout)


    def evaluate_javascript(self, tab_descriptor, expression, timeout):
        """Evaluates a JavaScript expression on the given tab.

        @param tab_descriptor: Indicate on which tab to evaluate the expression.
        @param expression: Indiate what expression to evaluate.
        @param timeout: Indicate the timeout of the expression.
        @return the JSONized result of the given expression
        """
        return self._resource.evaluate_javascript(
                tab_descriptor, expression, timeout)
