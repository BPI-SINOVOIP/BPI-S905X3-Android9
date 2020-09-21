# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file

import logging
import time

from telemetry.core import exceptions
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome

DEFAULT_TIMEOUT = 30
SHORT_TIMEOUT = 5


def get_webview_contexts(browser, ext_id):
    """Get all webview contexts for an extension.

    @param browser: Telemetry browser object.
    @param ext_id: Extension id of the kiosk app.
    @return A list of webview contexts.
    """
    ext_contexts = wait_for_kiosk_ext(browser, ext_id)

    for context in ext_contexts:
        context.WaitForDocumentReadyStateToBeInteractiveOrBetter()
        tagName = context.EvaluateJavaScript(
            "document.querySelector('webview') ? 'WEBVIEW' : 'NOWEBVIEW'")
        if tagName == "WEBVIEW":
            def _webview_context():
                try:
                    return context.GetWebviewContexts()
                except (chrome.Error):
                    logging.exception(
                        'An error occured while getting the webview contexts.')
                return None

            return utils.poll_for_condition(
                    _webview_context,
                    exception=error.TestFail('Webview not available.'),
                    timeout=DEFAULT_TIMEOUT,
                    sleep_interval=1)


# TODO(dtosic): deprecate this method in favor of 'get_webview_contexts()'
def get_webview_context(browser, ext_id):
    """Get context for CFM webview.

    @param browser: Telemetry browser object.
    @param ext_id: Extension id of the kiosk app.
    @return webview context.
    """
    ext_contexts = wait_for_kiosk_ext(browser, ext_id)

    for context in ext_contexts:
        context.WaitForDocumentReadyStateToBeInteractiveOrBetter()
        tagName = context.EvaluateJavaScript(
            "document.querySelector('webview') ? 'WEBVIEW' : 'NOWEBVIEW'")
        if tagName == "WEBVIEW":
            def _webview_context():
                try:
                    wb_contexts = context.GetWebviewContexts()
                    if len(wb_contexts) == 1:
                        return wb_contexts[0]
                    if len(wb_contexts) == 2:
                        return wb_contexts[1]

                except (KeyError, chrome.Error):
                    pass
                return None
            return utils.poll_for_condition(
                    _webview_context,
                    exception=error.TestFail('Webview not available.'),
                    timeout=DEFAULT_TIMEOUT,
                    sleep_interval=1)


def wait_for_kiosk_ext(browser, ext_id):
    """Wait for kiosk extension launch.

    @param browser: Telemetry browser object.
    @param ext_id: Extension id of the kiosk app.
    @return extension contexts.
    """
    def _kiosk_ext_contexts():
        try:
            ext_contexts = browser.extensions.GetByExtensionId(ext_id)
            if len(ext_contexts) > 1:
                return ext_contexts
        except (KeyError, chrome.Error):
            pass
        return []
    return utils.poll_for_condition(
            _kiosk_ext_contexts,
            exception=error.TestFail('Kiosk app failed to launch'),
            timeout=DEFAULT_TIMEOUT,
            sleep_interval=1)


def config_riseplayer(browser, ext_id, app_config_id):
    """
    Configure Rise Player app with a specific display id.

    Step through the configuration screen of the Rise Player app
    which is launched within the browser and enter a display id
    within the configuration frame to initiate media display.

    @param browser: browser instance containing the Rise Player kiosk app.
    @param ext_id: extension id of the Rise Player Kiosk App.
    @param app_config_id: display id for the Rise Player app .

    """
    if not app_config_id:
        raise error.TestFail(
                'Error in configuring Rise Player: app_config_id is None')
    config_js = """
                var frameId = 'btn btn-primary display-register-button'
                document.getElementsByClassName(frameId)[0].click();
                $( "input:text" ).val("%s");
                document.getElementsByClassName(frameId)[4].click();
                """ % app_config_id

    kiosk_webview_context = get_webview_context(
            browser, ext_id)
    # Wait for the configuration frame to load.
    time.sleep(SHORT_TIMEOUT)
    kiosk_webview_context.ExecuteJavaScript(config_js)
    # TODO (krishnargv): Find a way to verify that content is playing
    #                    within the RisePlayer app.
    verify_app_config_id = """
            /rvashow.*.display&id=%s.*/.test(location.href)
            """ % app_config_id
    #Verify that Risepplayer successfully validates the display id.
    try:
        kiosk_webview_context.WaitForJavaScriptCondition(
                verify_app_config_id,
                timeout=DEFAULT_TIMEOUT)
    except exceptions.TimeoutException:
        raise error.TestFail('Error in configuring Rise Player with id: %s'
                             % app_config_id)
