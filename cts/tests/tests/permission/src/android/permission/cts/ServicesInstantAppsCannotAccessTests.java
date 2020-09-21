/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.permission.cts;

import static android.content.Context.DEVICE_POLICY_SERVICE;
import static android.content.Context.FINGERPRINT_SERVICE;
import static android.content.Context.SHORTCUT_SERVICE;
import static android.content.Context.USB_SERVICE;
import static android.content.Context.WALLPAPER_SERVICE;
import static android.content.Context.WIFI_AWARE_SERVICE;
import static android.content.Context.WIFI_P2P_SERVICE;
import static android.content.Context.WIFI_SERVICE;

import static org.junit.Assert.assertNull;

import android.content.Context;
import android.platform.test.annotations.AppModeInstant;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Some services are not available to instant apps, see {@link Context#getSystemService}.
 */
@AppModeInstant
@RunWith(AndroidJUnit4.class)
public class ServicesInstantAppsCannotAccessTests {
    @Test
    public void cannotGetDevicePolicyManager() {
    assertNull(InstrumentationRegistry.getTargetContext().getSystemService(
            DEVICE_POLICY_SERVICE));
    }

    @Test
    public void cannotGetFingerprintManager() {
        assertNull(InstrumentationRegistry.getTargetContext().getSystemService(
                FINGERPRINT_SERVICE));
    }

    @Test
    public void cannotGetShortcutManager() {
        assertNull(InstrumentationRegistry.getTargetContext().getSystemService(
                SHORTCUT_SERVICE));
    }

    @Test
    public void cannotGetUsbManager() {
        assertNull(InstrumentationRegistry.getTargetContext().getSystemService(
                USB_SERVICE));
    }

    @Test
    public void cannotGetWallpaperManager() {
        assertNull(InstrumentationRegistry.getTargetContext().getSystemService(
                WALLPAPER_SERVICE));
    }

    @Test
    public void cannotGetWifiP2pManager() {
        assertNull(InstrumentationRegistry.getTargetContext().getSystemService(
                WIFI_P2P_SERVICE));
    }

    @Test
    public void cannotGetWifiManager() {
        assertNull(InstrumentationRegistry.getTargetContext().getSystemService(
                WIFI_SERVICE));
    }

    @Test
    public void cannotGetWifiAwareManager() {
        assertNull(InstrumentationRegistry.getTargetContext().getSystemService(
                WIFI_AWARE_SERVICE));
    }
}
