/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.settings.notification;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.when;

import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.ServiceInfo;

import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(SettingsRobolectricTestRunner.class)
public class SuppressorHelperTest {
    private static final String SUPPRESSOR_NAME = "wear";

    private ComponentName mSuppressor;
    @Mock
    private Context mContext;
    @Mock
    private PackageManager mPackageManager;
    @Mock
    private ServiceInfo mServiceInfo;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mSuppressor = new ComponentName("", "");

        try {
            when(mContext.getPackageManager()).thenReturn(mPackageManager);
            when(mPackageManager.getServiceInfo(mSuppressor, 0)).thenReturn(mServiceInfo);
            when(mServiceInfo.loadLabel(mPackageManager)).thenReturn(SUPPRESSOR_NAME);
        } catch (PackageManager.NameNotFoundException e) {
            // Do nothing. This exception will never happen in mock
        }
    }

    @Test
    public void testGetSuppressionText_SuppressorNull_ReturnNull() {
        final String text = SuppressorHelper.getSuppressionText(mContext, null);
        assertThat(text).isNull();
    }

    @Test
    public void testGetSuppressorCaption_SuppressorNotNull_ReturnSuppressorName() {
        final String text = SuppressorHelper.getSuppressorCaption(mContext, mSuppressor);
        assertThat(text).isEqualTo(SUPPRESSOR_NAME);
    }
}
