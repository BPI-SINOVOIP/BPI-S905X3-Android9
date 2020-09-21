/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.tv.settings.connectivity;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertEquals;

import android.net.wifi.WifiConfiguration;

import com.android.tv.settings.R;
import com.android.tv.settings.TvSettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(TvSettingsRobolectricTestRunner.class)
public class WifiConfigHelperTest {
    @Mock
    private WifiConfiguration mConfig;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
    }

    @Test
    public void testSetConfigSsid() {
        WifiConfigHelper.setConfigSsid(mConfig, "0000aaaaFFFF");
        assertThat(isDoubleQuoted(mConfig.SSID)).isTrue();
        WifiConfigHelper.setConfigSsid(mConfig, "atvtest");
        assertThat(isDoubleQuoted(mConfig.SSID)).isTrue();
        WifiConfigHelper.setConfigSsid(mConfig, "0000aaaaFFFF1");
        assertThat(isDoubleQuoted(mConfig.SSID)).isTrue();
        WifiConfigHelper.setConfigSsid(mConfig, "1234abcdEFFF");
        assertThat(isDoubleQuoted(mConfig.SSID)).isTrue();
        WifiConfigHelper.setConfigSsid(mConfig, "1234abcdEFFG");
        assertThat(isDoubleQuoted(mConfig.SSID)).isTrue();
        WifiConfigHelper.setConfigSsid(mConfig, "\"atv\"");
        assertThat(isDoubleQuoted(mConfig.SSID)).isTrue();
        String s = mConfig.SSID.substring(1, mConfig.SSID.length() - 1);
        assertThat(isDoubleQuoted(s)).isTrue();
    }

    @Test
    public void testValidate() {
        assertEquals(R.string.proxy_error_invalid_host, WifiConfigHelper.validate(".", "", ""));
        assertEquals(R.string.proxy_error_invalid_host,
                WifiConfigHelper.validate("..", "", ""));
        assertEquals(0, WifiConfigHelper.validate("", "", ""));
        assertEquals(R.string.proxy_error_invalid_host,
                WifiConfigHelper.validate("xyz.#", "", ""));
        assertEquals(R.string.proxy_error_empty_host_set_port,
                WifiConfigHelper.validate("", "293", ""));
        assertEquals(R.string.proxy_error_invalid_port,
                WifiConfigHelper.validate("abc", "293h", ""));
        assertEquals(R.string.proxy_error_invalid_host,
                WifiConfigHelper.validate("..", "12345", ""));
        assertEquals(R.string.proxy_error_invalid_host,
                WifiConfigHelper.validate("*abc", "1234", ""));
        assertEquals(R.string.proxy_error_invalid_exclusion_list,
                WifiConfigHelper.validate("abc", "1234", "&8"));
        assertEquals(0, WifiConfigHelper.validate("Android-TV", "1234", "*.abc"));
    }

    private boolean isDoubleQuoted(String s) {
        if (s.startsWith("\"") && s.endsWith("\"")) {
            return true;
        }
        return false;
    }
}
