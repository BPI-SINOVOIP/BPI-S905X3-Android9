# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class to control Netgear WNR1000V4 router."""

import logging

import ap_spec
import dynamic_ap_configurator


class NetgearWNR1000V4APConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Configurator for Netgear WNR1000V4 router."""


    def get_number_of_pages(self):
        """Returns the number of available pages."""
        return 1


    def is_update_interval_supported(self):
        """
        Returns True if setting the PSK refresh interval is supported.

        @return True is supported; False otherwise.
        """
        return False


    def get_supported_bands(self):
        """Returns a list of dictionary with the supported channel per band."""
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]}]


    def get_supported_modes(self):
        """Returns a list of dictionary with the supported band and modes."""
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_B, ap_spec.MODE_G, ap_spec.MODE_N]}]


    def is_security_mode_supported(self, security_mode):
        """Returns if a given security_type is supported.

        @param security_mode: one security modes defined in the APSpec.

        @return True if the security mode is supported; False otherwise.
        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPA2PSK,
                                 ap_spec.SECURITY_TYPE_MIXED)


    def is_spec_supported(self, spec):
        """
        Returns if a given spec is supported by the router.

        @param spec: an instance of the
        autotest_lib.server.cros.ap_configurators.APSpec class.

        @return: True if supported. False otherwise.
        """
        if spec.security == ap_spec.SECURITY_TYPE_MIXED and spec.mode == ap_spec.MODE_N:
            return False
        return True


    def _switch_to_frame1(self):
        """Switch to iframe formframe."""
        self.driver.switch_to_default_content()
        self.wait_for_object_by_xpath('//div[@id="formframe_div"]')
        frame = self.driver.find_element_by_xpath('//iframe[@name="formframe"]')
        self.driver.switch_to_frame(frame)


    def _switch_to_frame2(self):
        """Switch to wl2gsetting iframe within formframe."""
        self._switch_to_frame1()
        xpath = '//div[@id="main"]//iframe[@id="2g_setting"]'
        self.wait_for_object_by_xpath(xpath)
        setframe = self.driver.find_element_by_xpath(xpath)
        self.driver.switch_to_frame(setframe)


    def _logout_from_previous_netgear_login(self):
        """Logout if already logged into this router."""
        self.click_button_by_id('yes')
        self.navigate_to_page(1)


    def navigate_to_page(self, page_number):
        """Navigates to the page corresponding to the given page number.

        This method performs the translation between a page number and a url to
        load. This is used internally by apply_settings.

        @param page_number: page number of the page to load.

        """
        if self.object_by_id_exist('yes'):
            self._logout_from_previous_netgear_login()

        self.get_url(self.admin_interface_url)
        self.wait_for_object_by_xpath('//div[@id="wireless"]')
        self.click_button_by_xpath('//div[@id="wireless"]')
        self._switch_to_frame1()
        self.wait_for_object_by_xpath('//input[@name="ssid"]')


    def save_page(self, page_number):
        """Saves the given page.

        @param page_number: Page number of the page to save.
        """
        self.driver.switch_to_default_content()
        self._switch_to_frame1()
        xpath = '//input[@name="Apply"]'
        self.wait_for_object_by_xpath(xpath)
        self.click_button_by_xpath(xpath)
        self.wait_for_object_by_xpath(xpath, wait_time=30)


    def set_radio(self, enabled=True):
        """Turns the radio on and off.

        @param enabled: True to turn on the radio; False otherwise.
        """
        logging.debug('Cannot turn OFF radio!')


    def set_ssid(self, ssid):
        """Sets the SSID of the wireless network.

        @param ssid: name of the wireless network.
        """
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        """Sets the SSID of the wireless network.

        @param ssid: name of the wireless network.
        """
        xpath = '//input[@maxlength="32" and @name="ssid"]'
        self.set_content_of_text_field_by_xpath(ssid, xpath, abort_check=True)
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
        position = self._get_channel_popup_position(channel)
        channel_choices = [ '01', '02', '03', '04', '05', '06',
                           '07', '08', '09', '10', '11']
        xpath = '//select[@name="w_channel"]'
        # Channel pop up is empty at this point, Save and Reload page
        if self.number_of_items_in_popup_by_xpath(xpath) == 0:
              self.save_page(1)
              self._switch_to_frame2()
              self.select_item_from_popup_by_xpath(channel_choices[position],
                                                   xpath)


    def set_mode(self, mode, band=None):
        """Sets the mode.

        @param mode: must be one of the modes listed in __init__().
        @param band: the band to select.

        """
        self.add_item_to_command_list(self._set_mode, (mode,), 1, 900)


    def _set_mode(self, mode, band=None):
        """Sets the mode.

        @param mode: must be one of the modes listed in __init__().
        @param band: the band to select.

        """
        if mode == ap_spec.MODE_G or mode == ap_spec.MODE_B:
            mode = 'Up to 54 Mbps'
        elif mode == ap_spec.MODE_N:
            mode = 'Up to 300 Mbps'
        else:
            raise RuntimeError('Unsupported mode passed.')
        xpath = '//select[@name="opmode"]'
        self.select_item_from_popup_by_xpath(mode, xpath)


    def set_band(self, band):
        """Sets the band of the wireless network.

        @param band: Constant describing the band type.
        """
        self.current_band = ap_spec.BAND_2GHZ


    def set_security_disabled(self):
        """Disables the security of the wireless network."""
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 900)


    def _set_security_disabled(self):
        """Disables the security of the wireless network."""
        self.driver.switch_to_default_content()
        self._switch_to_frame2()
        xpath = '//input[@name="security_type" and @value="Disable"]'
        self.click_button_by_xpath(xpath)


    def set_security_wep(self, value, authentication):
        """Enables WEP security for the wireless network.

        @param key_value: encryption key to use.
        @param authentication: one of two supported WEP authentication types:
                               open or shared.
        """
        logging.debug('WEP mode is not supported.')


    def set_security_wpapsk(self, security, key, update_interval=None):
        """Enables WPA2 and Mixed modes using a private security key for
        the wireless network.

        @param security: Required security for AP configuration.
        @param shared_key: shared encryption key to use.
        @param update_interval: number of seconds to wait before updating.
        """
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, key,),
                                      1, 900)


    def _set_security_wpapsk(self, security, key, update_interval=None):
        """Enables WPA2 and Mixed modes using a private security key for
        the wireless network.

        @param security: Required security for AP configuration.
        @param shared_key: shared encryption key to use.
        @param update_interval: number of seconds to wait before updating.
        """
        self._switch_to_frame2()

        if security == ap_spec.SECURITY_TYPE_WPA2PSK:
            xpath = '//input[@name="security_type" and @value="WPA2-PSK"]'
        elif security == ap_spec.SECURITY_TYPE_MIXED:
            xpath = '//input[@name="security_type" and @value="AUTO-PSK"]'
        else:
            raise RunTimeException('Invalid security mode %s sent' % security)

        self.click_button_by_xpath(xpath)
        xpath = '//input[@name="passphrase"]'
        self.set_content_of_text_field_by_xpath(key, xpath, abort_check=True)


    def is_visibility_supported(self):
        """
        Returns if AP supports setting the visibility (SSID broadcast).

        @return True if supported; False otherwise.
        """
        return True


    def set_visibility(self, visible=True):
        """
        Sets SSID broadcast ON by clicking the checkbox.

        @param visible: True to enable SSID broadcast. False otherwise.
        """
        self.add_item_to_command_list(self._set_visibility, (visible,), 1, 900)


    def _set_visibility(self, visible=True):
        """
        Sets SSID broadcast ON by clicking the checkbox.

        @param visible: True to enable SSID broadcast. False otherwise.
        """
        self._switch_to_frame2()
        xpath = '//input[@name="ssid_bc" and @type="checkbox"]'
        check_box = self.wait_for_object_by_xpath(xpath)
        value = check_box.is_selected()
        if (visible and not value) or (not visible and value):
            self.click_button_by_xpath(xpath)
