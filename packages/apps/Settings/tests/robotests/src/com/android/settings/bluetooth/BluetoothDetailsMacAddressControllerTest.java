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
package com.android.settings.bluetooth;

import static com.google.common.truth.Truth.assertThat;

import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.SettingsShadowBluetoothDevice;
import com.android.settingslib.widget.FooterPreference;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = SettingsShadowBluetoothDevice.class)
public class BluetoothDetailsMacAddressControllerTest extends BluetoothDetailsControllerTestBase {

  private BluetoothDetailsMacAddressController mController;

  @Override
  public void setUp() {
    super.setUp();
    mController =
        new BluetoothDetailsMacAddressController(mContext, mFragment, mCachedDevice, mLifecycle);
    setupDevice(mDeviceConfig);
  }

  @Test
  public void macAddress() {
    showScreen(mController);
    FooterPreference footer =
        (FooterPreference) mScreen.findPreference(mController.getPreferenceKey());
    assertThat(footer.getTitle().toString()).endsWith(mDeviceConfig.getAddress());
  }
}
