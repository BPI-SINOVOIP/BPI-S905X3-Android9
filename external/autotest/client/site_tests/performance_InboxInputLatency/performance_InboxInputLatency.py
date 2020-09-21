# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import numpy
import os
import tempfile
import time

from autotest_lib.client.bin import test
from autotest_lib.client.cros.input_playback import keyboard
from autotest_lib.client.cros.input_playback import stylus
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.common_lib.cros import chrome
from telemetry.timeline import model as model_module
from telemetry.timeline import tracing_config


_BODY_EDITABLE_CLASS = 'aR CoXcW editable'
_COMPOSE_BUTTON_CLASS = 'y hC'
_DISCARD_BUTTON_CLASS = 'ezvJwb bG AK ew IB'
_IDLE_FOR_INBOX_STABLIZED = 30
_INBOX_URL = 'https://inbox.google.com'
_KEYIN_TEST_DATA = 'input_test_data'
_SCRIPT_TIMEOUT = 5
_TARGET_EVENT = 'InputLatency::Char'
_TARGET_TRACING_CATEGORIES = 'input, latencyInfo'
_TRACING_TIMEOUT = 60


class performance_InboxInputLatency(test.test):
    """Invoke Inbox composer, inject key events then measure latency."""
    version = 1

    def initialize(self):
        # Instantiate Chrome browser.
        with tempfile.NamedTemporaryFile() as cap:
            file_utils.download_file(chrome.CAP_URL, cap.name)
            password = cap.read().rstrip()

        self.browser = chrome.Chrome(gaia_login=True,
                                     username=chrome.CAP_USERNAME,
                                     password=password)
        self.tab = self.browser.browser.tabs[0]

        # Setup Chrome Tracing.
        config = tracing_config.TracingConfig()
        category_filter = config.chrome_trace_config.category_filter
        category_filter.AddFilterString(_TARGET_TRACING_CATEGORIES)
        config.enable_chrome_trace = True
        self.target_tracing_config = config

        # Create a virtual keyboard device for key event playback.
        self.keyboard = keyboard.Keyboard()

        # Create a virtual stylus
        self.stylus = stylus.Stylus()

    def cleanup(self):
        if hasattr(self, 'browser'):
            self.browser.close()
        if self.keyboard:
            self.keyboard.close()
        if self.stylus:
            self.stylus.close()

    def click_button_by_class_name(self, class_name):
        """
        Locate a button by its class name and click it.

        @param class_name: the class name of the button.

        """
        button_query = 'document.getElementsByClassName("' + class_name +'")'
        # Make sure the target button is available
        self.tab.WaitForJavaScriptCondition(button_query + '.length == 1',
                                            timeout=_SCRIPT_TIMEOUT)

        query = ('(function(element) {'
                 '  var rect = element.getBoundingClientRect();'
                 '  return {left: rect.left, top: rect.top,'
                 '          width: rect.width, height: rect.height};'
                 ' })(%s[0]);' % button_query)
        target = self.tab.EvaluateJavaScript(query)
        logging.info(target)

        query = ('(function(element) {'
                 '  var rect = element.getBoundingClientRect();'
                 '  return {width: rect.width,'
                 '          height: rect.height};'
                 ' })(document.body);')
        window = self.tab.EvaluateJavaScript(query)
        logging.info(window)

        # Click on the center of the target component in the window
        percent_x = (target['left'] + target['width'] / 2.0) / window['width']
        percent_y = (target['top'] + target['height'] / 2.0) / window['height']
        self.stylus.click_with_percentage(percent_x, percent_y)

    def setup_inbox_composer(self):
        """Navigate to Inbox, and click the compose button."""
        self.tab.Navigate(_INBOX_URL)
        tracing_controller = self.tab.browser.platform.tracing_controller
        if not tracing_controller.IsChromeTracingSupported():
            raise Exception('Chrome tracing not supported')

        # Idle for making inbox tab stablized, i.e. not busy in syncing with
        # backend inbox server.
        time.sleep(_IDLE_FOR_INBOX_STABLIZED)

        # Bring back the focus to browser window
        self.stylus.click(1, 1)

        # Make inbox tab fullscreen by pressing F4 key
        fullscreen = self.tab.EvaluateJavaScript('document.webkitIsFullScreen')
        if not fullscreen:
            self.keyboard.press_key('f4')

        # Click Compose button to instantiate a draft mail.
        self.click_button_by_class_name(_COMPOSE_BUTTON_CLASS)

        # Click the mail body area for focus
        self.click_button_by_class_name(_BODY_EDITABLE_CLASS)

    def teardown_inbox_composer(self):
        """Discards the draft mail."""
        self.click_button_by_class_name(_DISCARD_BUTTON_CLASS)
        # Cancel Fullscreen
        query = ('if (document.webkitIsFullScreen)'
                 '  document.webkitCancelFullScreen();')
        self.tab.EvaluateJavaScript(query)

    def measure_input_latency(self):
        """Injects key events then measure and report the latency."""
        tracing_controller = self.tab.browser.platform.tracing_controller
        tracing_controller.StartTracing(self.target_tracing_config,
                                        timeout=_TRACING_TIMEOUT)
        # Inject pre-recorded test key events
        current_dir = os.path.dirname(os.path.realpath(__file__))
        data_file = os.path.join(current_dir, _KEYIN_TEST_DATA)
        self.keyboard.playback(data_file)
        results = tracing_controller.StopTracing()

        # Iterate recorded events and output target latency events
        timeline_model = model_module.TimelineModel(results)
        event_iter = timeline_model.IterAllEvents(
                event_type_predicate=model_module.IsSliceOrAsyncSlice)

        # Extract and report the latency information
        latency_data = []
        previous_start = 0.0
        for event in event_iter:
            if event.name == _TARGET_EVENT and event.start != previous_start:
                logging.info('input char latency = %f ms', event.duration)
                latency_data.append(event.duration)
                previous_start = event.start
        operators = ['mean', 'std', 'max', 'min']
        for operator in operators:
            description = 'input_char_latency_' + operator
            value = getattr(numpy, operator)(latency_data)
            logging.info('%s = %f', description, value)
            self.output_perf_value(description=description,
                                   value=value,
                                   units='ms',
                                   higher_is_better=False)

    def run_once(self):
        self.setup_inbox_composer()
        self.measure_input_latency()
        self.teardown_inbox_composer()
