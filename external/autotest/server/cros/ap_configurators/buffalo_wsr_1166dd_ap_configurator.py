# Copyright (c) 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class to control the Buffalo WSR1166DD router."""

import logging
import urlparse

import dynamic_ap_configurator
import ap_spec
import time


class BuffaloWSR1166DDAPConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Configurator for Buffalo WSR1166DD router."""
    BAND_IDS = {
        ap_spec.BAND_2GHZ: 'ra0Form',
        ap_spec.BAND_5GHZ: 'ra00_0Form'
    }
    IFACE_IDS = {
        ap_spec.BAND_2GHZ: 'ra0aForm',
        ap_spec.BAND_5GHZ: 'ra00_0aForm'
    }


    def get_number_of_pages(self):
        """Returns the number of available pages."""
        return 1


    def is_update_interval_supported(self):
        """Returns True if setting the PSK refresh interval is supported.

        @return True is supported; False otherwise
        """
        return False


    def get_supported_modes(self):
        """Returns a list of dictionaries with supported bands and modes."""
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_N, ap_spec.MODE_G]},
                {'band': ap_spec.BAND_5GHZ,
                 'modes': [ap_spec.MODE_AC, ap_spec.MODE_N, ap_spec.MODE_A]}]


    def get_supported_bands(self):
        """Returns a list of dictionaries with supported channels per band."""
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': [1, 2, 3, 4, 5, 6, 7, 8, 9 , 10, 11]},
                {'band': ap_spec.BAND_5GHZ,
                 'channels': [36, 40, 44, 48, 149, 153,
                              157, 161, 165]}]


    def is_security_mode_supported(self, security_mode):
        """Returns if a given security_type is supported.

        @param security_mode: one security modes defined in the APSpec

        @return True if the security mode is supported; False otherwise.
        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_MIXED)


    def navigate_to_page(self, page_number):
        """Navigates to the page corresponding to the given page number.

        This method performs the translation between a page number and a url
        to load. This is used internally by apply_settings.

        @param page_number: page number of the page to load

        """
        self.get_url(self.admin_interface_url)
        if self.admin_login_needed(self.admin_interface_url):
            self.ap_login()
        self.click_button_by_xpath('//li[@id="network"]')
        self.wait_for_object_by_id('menuSub')
        self.click_button_by_xpath('//div[@id="menuSub"]/li[2]')
        self.wait_for_object_by_id('ra0Container')


    def save_page(self, page_number):
        """Saves the given page.

        @param page_number: Page number of the page to save.
        """
        xpath = '//fieldset[@id="ra0Container"]/center/input'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//fieldset[@id="ra00_0Container"]/center/input'
        self.click_button_by_xpath(xpath)
        confirmation_xpath = (
            '//div[@id="uiPopup" and contains(., "Settings Applied")]')
        self.wait_for_object_by_xpath(confirmation_xpath)


    def set_mode(self, mode, band=None):
        """Sets the mode.

        @param mode: must be one of the modes listed in __init__().
        @param band: the band to select.

        """
        self.add_item_to_command_list(self._set_mode, (mode, band), 1, 800)


    def _set_mode(self, mode, band=None):
        """Sets the mode.

        @param mode: must be one of the modes listed in __init__().
        @param band: the band to select.

        """
        modes = {
            ap_spec.BAND_5GHZ: {
                ap_spec.MODE_N: '802.11n 20 MHz',
                ap_spec.MODE_A: '802.11a',
                ap_spec.MODE_AC: '802.11ac 40 MHz'
            },
            ap_spec.BAND_2GHZ: {
                ap_spec.MODE_N: '802.11n 20 MHz',
                ap_spec.MODE_G: '802.11g'
            }
        }
        item_value = modes.get(self.current_band).get(mode)
        if not item_value:
            raise RuntimeError('Mode not supported %s' % mode)
        xpath = (
            '//form[@id="%s"]/.//select[@id="htmode"]' %
            self.BAND_IDS.get(self.current_band))
        self.select_item_from_popup_by_xpath(item_value, xpath)


    def set_radio(self, enabled=True):
        """Turns the radio on and off.

        @param enabled: True to turn on the radio; False otherwise.
        """
        self.add_item_to_command_list(self._set_radio, (enabled,), 1, 1)


    def _set_radio(self, enabled=True):
        """Turns the radio on and off.

        @param enabled: True to turn on the radio; False otherwise.
        """
        value = 'Disabled'
        if enabled:
            value = 'Enabled'
        xpath = (
            '//form[@id="%s"]/.//select[@id="disabled"]' %
            self.BAND_IDS.get(self.current_band))
        self.select_item_from_popup_by_xpath(value, xpath)


    def set_ssid(self, ssid):
        """Sets the SSID of the wireless network.

        @param ssid: name of the wireless network.
        """
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        """Sets the SSID of the wireless network.

        @param ssid: name of the wireless network.
        """
        xpath = '//form[@id="ra0aForm"]/div[3]/div[@class="uiField"]/input'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = (
                '//form[@id="ra00_0aForm"]/div[3]/div[@class="uiField"]/input')
        self.set_content_of_text_field_by_xpath(ssid, xpath)
        self._ssid = ssid


    def set_channel(self, channel):
        """Sets the channel of the wireless network.

        @param channel: integer value of the channel.
        """
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        """Sets the channel of the wireless network.

        @param channel: integer value of the channel.
        """
        xpath = (
            '//form[@id="%s"]/.//select[@id="channel"]' %
            self.BAND_IDS.get(self.current_band))
        self.select_item_from_popup_by_xpath(str(channel), xpath)


    def set_ch_width(self, width):
        """Adjusts the channel width.

        @param width: the channel width.
        """
        self.add_item_to_command_list(self._set_ch_width,(width,), 1, 900)


    def _set_ch_width(self, width):
        """Adjusts the channel width.

        @param width: the channel width.
        """
        channel_width_choice = {
            ap_spec.BAND_2GHZ: ['802.11n 40 MHz',
                                '802.11n 20 MHz'],
            ap_spec.BAND_5GHZ: ['802.11ac 80 MHz',
                                '802.11ac 40 MHz',
                                '802.11n 20 MHz']
        }
        xpath = (
            '//form[@id="%s"]/.//select[@id="htmode"]' %
            self.BAND_IDS.get(self.current_band))
        self.select_item_from_popup_by_xpath(
            channel_width_choice.get(self.current_band)[width],
            xpath)


    def set_band(self, band):
        """Sets the band of the wireless network.

        @param band: Constant describing the band type.
        """
        if band == ap_spec.BAND_5GHZ:
            self.current_band = ap_spec.BAND_5GHZ
        elif band == ap_spec.BAND_2GHZ:
            self.current_band = ap_spec.BAND_2GHZ
        else:
            raise RuntimeError('Invalid band sent %s' % band)


    def set_security_disabled(self):
        """Disables the security of the wireless network."""
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 900)


    def _set_security_disabled(self):
        """Disables the security of the wireless network."""
        xpath = (
            '//form[@id="%s"]/.//select[@id="encryption"]' %
            self.IFACE_IDS.get(self.current_band))
        self.select_item_from_popup_by_xpath('None', xpath)


    def set_security_wep(self, key_value, authentication):
        """Enables WEP security for the wireless network.

        @param key_value: encryption key to use.
        @param authentication: one of two supported WEP authentication types:
                               open or shared.
        """
        logging.debug('wep security not supported by this router')


    def set_security_wpapsk(self, security, shared_key, update_interval=None):
        """Enables WPA using a private security key for the wireless network.

        @param security: Required security for AP configuration.
        @param shared_key: shared encryption key to use.
        @param update_interval: number of seconds to wait before updating.
        """
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key,), 1, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        """Enables WPA/WPA2 using a psk for the wireless network.

        @param security: Required security for AP configuration.
        @param shared_key: shared encryption key to use.
        @param update_interval: number of seconds to wait before updating.
        """
        xpath = (
            '//form[@id="%s"]/.//select[@id="encryption"]' %
            self.IFACE_IDS.get(self.current_band))
        self.select_item_from_popup_by_xpath('WPA/WPA2 PSK', xpath)
        xpath = (
            '//form[@id="%s"]/.//input[@id="key"]' %
            self.IFACE_IDS.get(self.current_band))
        self.set_content_of_text_field_by_xpath(shared_key, xpath)


    def is_visibility_supported(self):
        """Returns  True if AP supports setting SSID broadcast.

        @return True if supported; False otherwise.
        """
        return False


    def admin_login_needed(self, page_url):
        """Check if we are on the admin login page.

        @param page_url: string, the page to open.

        @return True if login needed False otherwise.
        """
        login_element = '//input[@id="pass"]'
        apply_element = '//input[@value="Submit"]'
        login_displayed = self.wait_for_objects_by_xpath([login_element,
                                                          apply_element])
        if login_displayed == login_element:
            return True
        elif login_displayed == apply_element:
            return False
        else:
            raise Exception('The page %s did not load' % page_url)


    def ap_login(self):
        """Login as admin before configuring settings."""
        self.set_content_of_text_field_by_id('password', 'pass',
                                             abort_check=True)
        self.click_button_by_xpath('//input[@value="Submit"]')
        self.wait_for_object_by_id('uiContent')
