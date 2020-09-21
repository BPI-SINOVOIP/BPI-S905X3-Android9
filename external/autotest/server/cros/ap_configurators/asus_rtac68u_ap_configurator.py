# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

"""Class to control Asus RT-AC68U router."""

import logging

import ap_spec
import dynamic_ap_configurator

from selenium.common.exceptions import UnexpectedAlertPresentException


class AsusRTAC68UAPConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Configurator for Asus RT-AC68U router."""


    def _alert_handler(self, alert):
        """Checks for any modal dialogs which popup to alert the user and
        either raises a RuntimeError or ignores the alert.

        @param alert: The modal dialog's contents.
        """
        text = alert.text
        if 'Open System' in text:
            alert.accept()
        else:
            raise RuntimeError('Unhandled alert message: %s' % text)


    def get_number_of_pages(self):
        """Returns the number of available pages."""
        return 1


    def get_supported_bands(self):
        """Returns a list of dictionaries describing the supported bands."""
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]},
                {'band': ap_spec.BAND_5GHZ,
                 'channels': [36, 40, 44, 48, 149, 153, 157, 161, 165]}]


    def get_supported_modes(self):
        """Returns a list of dictionaries with the supported bands and modes."""
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_N]},
                {'band': ap_spec.BAND_5GHZ,
                 'modes': [ap_spec.MODE_N, ap_spec.MODE_N|ap_spec.MODE_AC]}]


    def get_supported_channel_widths(self):
        """
        Returns a dictionary describing supported channel widths based on
        band and mode.

        @return a dictionary as described above.
        """
        return {ap_spec.BAND_2GHZ: {ap_spec.MODE_N: [20]},
                ap_spec.BAND_5GHZ: {ap_spec.MODE_N: [20],
                                    ap_spec.MODE_N|ap_spec.MODE_AC: [20]}}


    def is_security_mode_supported(self, security_mode):
        """Returns if a given security_type is supported for 2.4GHz and
        5GHz bands.

        @param security_mode: one security modes defined in the ap_spec.

        @return True if the security mode is supported; False otherwise.
        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPA2PSK,
                                 ap_spec.SECURITY_TYPE_MIXED)


    def navigate_to_page(self, page_number):
        """Navigates to the page corresponding to the given page number.

        This method performs the translation between a page number and a url
        to load. This is used internally by apply_setting.

        @param page_number: page number of the page to load.
        """
        self.get_url(self.admin_interface_url)
        self._ap_login()
        xpath = '//div[@id="subMenu"]//div[@id="option_str1"]'
        self.click_button_by_xpath(xpath)
        ssid_xpath = '//table[@id="WLgeneral"]//input[@id="wl_ssid"]'
        self.wait_for_object_by_xpath(ssid_xpath)


    def _ap_login(self):
        """Login as admin to configure settings."""
        username = '//input[@id="login_username"]'
        self.set_content_of_text_field_by_xpath('admin', username,
                                                abort_check=True)
        password = '//div[@class="password_gap"]//input[@name="login_passwd"]'
        self.set_content_of_text_field_by_xpath('password', password,
                                                abort_check=True)
        self.click_button_by_xpath('//div[@class="button"]')


    def _is_alert_present(self, xpath):
        """Handle alerts if there is an alert on Save."""
        try:
            self.driver.find_element_by_xpath(xpath)
            return
        except UnexpectedAlertPresentException:
            alert = self.driver.switch_to_alert()
            self._alert_handler(alert)


    def save_page(self, page_number):
        """Saves the given page.

        @param page_number: Page number of the page to be saved.
        """
        apply = '//div[@class="apply_gen"]//input[@id="applyButton"]'
        self.click_button_by_xpath(apply)
        self._is_alert_present(apply)
        self.wait_for_object_by_xpath(apply)


    def set_mode(self, mode, band=None):
        """Sets the network mode.

         @param mode: must be one of the modes listed in __init__().
         @param band: the band to select.
        """
        self.add_item_to_command_list(self._set_mode, (mode, band), 1, 800)


    def _set_mode(self, mode, band=None):
        """Sets the network mode.

        @param mode: must be one of the modes listed in __init__().
        @param band: the band to select.
        """
        modes_2GHZ = {ap_spec.MODE_N:'N only'}
        modes_5GHZ = {ap_spec.MODE_N:'N only',
                      ap_spec.MODE_N|ap_spec.MODE_AC:'N/AC mixed'}
        modes = {ap_spec.BAND_2GHZ: modes_2GHZ,
                 ap_spec.BAND_5GHZ: modes_5GHZ}
        mode_selected = modes.get(self.current_band).get(mode)
        if not mode_selected:
            raise RunTimeError('Mode %s with Band %s is not supported'
                               % (mode,band))
        self.driver.switch_to_default_content()
        xpath = '//select[@name="wl_nmode_x"]'
        self.wait_for_object_by_xpath(xpath)
        self.select_item_from_popup_by_xpath(mode_selected, xpath)


    def set_radio(self, enabled=True):
        """Turns the radio on and off.

        @param enabled: True to turn on the radio; False otherwise.
        """
        self.add_item_to_command_list(self._set_radio, (enabled,), 1, 700)


    def _set_radio(self, enabled=True):
        """Turns the radio on and off.

        @param enabled: True to turn on the radio; False otherwise.
        """
        xpath = '//div[@id="tabMenu"]/table/tbody/tr/td[6]/div/span'
        self.wait_for_object_by_xpath(xpath)
        self.click_button_by_xpath(xpath)

        if enabled:
            xpath = '//input[@name="wl_radio" and @value="1"]'
        else:
            xpath = '//input[@name="wl_radio" and @value="0"]'
        self.click_button_by_xpath(xpath)

        xpath = '//div[@class="apply_gen"]//input[@class="button_gen"]'
        self.click_button_by_xpath(xpath)

        self.set_wait_time(30)
        self.wait.until(lambda _:
                        self.wait_for_object_by_xpath(xpath).is_displayed())
        self.restore_default_wait_time()

        xpath = '//div[@id="tabMenu"]/table/tbody/tr/td[1]/div/span/table/\
                 tbody/tr/td'
        self.click_button_by_xpath(xpath)


    def set_ssid(self, ssid):
        """Sets the SSID of the wireless network.

        @param ssid: name of the wireless network.
        """
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        """Sets the SSID of the wireless network.

        @param ssid: name of the wireless network.
        """
        xpath = '//input[@id="wl_ssid"]'
        self.set_content_of_text_field_by_xpath(ssid, xpath)
        self._ssid = ssid


    def set_channel(self, channel):
        """Sets the channel of the wireless network.

        @param channel: Integer value of the channel.
        """
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        """Sets the channel of the wireless network.

        @param channel: Integer value of the channel.
        """
        xpath = '//select[@name="wl_channel"]'
        self.select_item_from_popup_by_xpath(str(channel), xpath)


    def set_channel_width(self, channel_width):
        """Adjusts the channel width.

        @param width: the channel width.
        """
        self.add_item_to_command_list(self._set_channel_width,
                                      (channel_width,), 1, 900)


    def _set_channel_width(self, channel_width):
        """Adjusts the channel width.

        @param width: the channel width.
        """
        channel_width_choice = {20:'20 MHz'}
        xpath = '//tr[@id="wl_bw_field"]//select[@name="wl_bw"]'
        self.select_item_from_popup_by_xpath(
                channel_width_choice.get(channel_width), xpath)


    def set_band(self, band):
        """Sets the band of the wireless network.

        @param band: Constant describing band type.
        """
        if band == ap_spec.BAND_5GHZ:
            self.current_band = ap_spec.BAND_5GHZ
        elif band == ap_spec.BAND_2GHZ:
            self.current_band = ap_spec.BAND_2GHZ
        else:
            raise RuntimeError('Invalid band sent %s' % band)
        self.add_item_to_command_list(self._set_band, (), 1, 700)


    def _set_band(self):
        """Sets the band of the wireless network."""
        xpath = '//tr[@id="wl_unit_field"]//select[@name="wl_unit"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            selected_band = '5GHz'
        else:
            selected_band = '2.4GHz'
        self.select_item_from_popup_by_xpath(selected_band, xpath)


    def set_security_disabled(self):
        """Disables the security of the wireless network."""
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 900)


    def _set_security_disabled(self):
        """Disables the security of the wireless network."""
        xpath = ' //select[@name="wl_auth_mode_x"]'
        self.select_item_from_popup_by_xpath('Open System', xpath)


    def set_security_wep(self, key_value, authentication):
        """Enables WEP security for the wireless network.

        @param key_value: encryption key to use.
        @param authentication: Open or Shared authentication types.
        """
        logging.debug('WEP mode is not supported.')


    def set_security_wpapsk(self, security, shared_key, update_interval=None):
        """Enables WPA2-Personal security type for the wireless network.

        @param security: security type to configure.
        @param shared_key: shared encryption key to use.
        @param updat_interval: number of seconds to wait before updating.
        """
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key, update_interval),
                                       1, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        """Enables WPA2-Personal and WPA-Auto-Personal(mixed mode) security type
        for the wireless network.

        @param security: security type to configure.
        @param shared_key: shared encryption key to use.
        @param update_interval: number of seconds to wait before updating.
        """
        xpath = '//select[@name="wl_auth_mode_x"]'
        passphrase = '//input[@name="wl_wpa_psk" and @class="input_32_table"]'

        if security == ap_spec.SECURITY_TYPE_WPA2PSK:
            popup_value  = 'WPA2-Personal'
        elif security == ap_spec.SECURITY_TYPE_MIXED:
            popup_value = 'WPA-Auto-Personal'
        else:
            raise RunTimeError('Invalid Security Mode %s passed' % security)

        self.select_item_from_popup_by_xpath(popup_value, xpath)
        self.set_content_of_text_field_by_xpath(shared_key, passphrase,
                                                abort_check=True)


    def set_visibility(self, visible=True):
        """Sets the SSID broadcast ON

        @param visible: True to enable SSID broadcast. False otherwise.
        """
        self.add_item_to_command_list(self._set_visibility, (visible,), 1, 900)


    def _set_visibility(self, visible=True):
        """Sets the SSID broadcast ON

        @param visible: True to enable SSID broadcast. False otherwise.
        """
        if visible == True:
            xpath = '//input[@name="wl_closed" and @value="0"]'
        else:
            xpath = '//input[@name="wl_closed" and @value="1"]'
        self.click_button_by_xpath(xpath)
