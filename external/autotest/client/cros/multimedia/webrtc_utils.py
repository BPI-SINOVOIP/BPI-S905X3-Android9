# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utils for webrtc-related functionality.

Note that this module is shared by both server side and client side.
Do not put something only usable in client side in this module.

"""

import logging
import time
import uuid


class AppRTCParameters(object):
    """Class to hold parameters for AppRTC webpage."""
    def __init__(self):
        """Initializes an AppRTCParameters."""
        self.debug = 'loopback'
        self.audio = {'googEchoCancellation': False,
                      'googAutoGainControl': False,
                      'googNoiseReduction': False}


    def _get_audio_parameter_string(self):
        """Converts the audio parameters into parameter string used in URL.

        @return: Audio parameter string like "audio=googEchoCancellation=False,..."

        """
        audio_params = []
        for key, value in self.audio.iteritems():
            audio_params.append('%s=%s' % (key, 'true' if value else 'false'))
        audio_params_str = ','.join(audio_params)
        return audio_params_str


    def get_parameter_string(self):
        """Converts the parameters into parameter string used in URL.

        @return: Parameter string used in URL.

        """
        param_str = '?debug=%s' % self.debug
        param_str += '&'
        param_str += 'audio=' + self._get_audio_parameter_string()
        return param_str


class AppRTCController(object):
    """Class to control AppRTC webpage."""

    BASE_URL = 'https://appr.tc/r/'
    WAIT_FOR_JOIN_CALL_SECS = 10
    CLICK_JOIN_BUTTON_TIMEOUT_SECS = 10

    def __init__(self, browser_facade):
        """Initializes an AppRTCController.

        @param browser_facade: A BrowserFacadeNative (for client side) or
                               BrowserFacadeAdapter (for server side).

        """
        # Only use default parameters for now. If different parameter is needed
        # we can takes an AppRTCParameters from argument.
        self.param = AppRTCParameters()
        self.browser_facade = browser_facade


    def new_apprtc_loopback_page(self):
        """Loads a AppRTC webpage in a new tab with loopback enabled."""
        room_name = str(uuid.uuid4())
        url = self.BASE_URL + room_name + self.param.get_parameter_string()
        logging.debug('Open AppRTC loopback page %s', url)
        tab_desc = self.browser_facade.new_tab(url)
        self.click_join_button(tab_desc)
        # After clicking join button, it takes some time to actually join the
        # call.
        time.sleep(self.WAIT_FOR_JOIN_CALL_SECS)


    def click_join_button(self, tab_desc):
        """Clicks 'join' button on the webpage.

        @param tab_desc: Tab descriptor returned by new_tab of browser facade.

        """
        click_button_js = """document.getElementById('confirm-join-button').click();"""
        self.browser_facade.execute_javascript(
                tab_desc, click_button_js, self.CLICK_JOIN_BUTTON_TIMEOUT_SECS)
