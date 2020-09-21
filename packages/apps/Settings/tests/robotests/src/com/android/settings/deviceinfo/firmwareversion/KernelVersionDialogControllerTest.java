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

package com.android.settings.deviceinfo.firmwareversion;

import static com.android.settings.deviceinfo.firmwareversion.KernelVersionDialogController.KERNEL_VERSION_VALUE_ID;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;

import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.DeviceInfoUtils;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;

@RunWith(SettingsRobolectricTestRunner.class)
public class KernelVersionDialogControllerTest {

    @Mock
    private FirmwareVersionDialogFragment mDialog;

    private Context mContext;
    private KernelVersionDialogController mController;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);
        mContext = RuntimeEnvironment.application;
        when(mDialog.getContext()).thenReturn(mContext);
        mController = new KernelVersionDialogController(mDialog);
    }

    @Test
    public void initialize_shouldUpdateKernelVersionToDialog() {
        mController.initialize();

        verify(mDialog)
            .setText(KERNEL_VERSION_VALUE_ID, DeviceInfoUtils.getFormattedKernelVersion(mContext));
    }
}
