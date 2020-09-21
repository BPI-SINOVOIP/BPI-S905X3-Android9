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
package com.android.phone;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.os.PersistableBundle;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.telephony.CarrierConfigManager;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class CdmaOptionsTest {
    @Mock
    private Phone mMockPhone;

    private CdmaOptions mCdmaOptions;
    private Context mContext;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = InstrumentationRegistry.getContext();
        mCdmaOptions = new CdmaOptions(mMockPhone);
    }

    @Test
    public void shouldAddApnExpandPreference_doesNotExpandOnGsm() {
        when(mMockPhone.getPhoneType()).thenReturn(PhoneConstants.PHONE_TYPE_GSM);
        PersistableBundle bundle = new PersistableBundle();
        bundle.putBoolean(CarrierConfigManager.KEY_SHOW_APN_SETTING_CDMA_BOOL, true);
        assertFalse(mCdmaOptions.shouldAddApnExpandPreference(bundle));

        when(mMockPhone.getPhoneType()).thenReturn(PhoneConstants.PHONE_TYPE_CDMA);
        bundle.putBoolean(CarrierConfigManager.KEY_SHOW_APN_SETTING_CDMA_BOOL, false);
        assertFalse(mCdmaOptions.shouldAddApnExpandPreference(bundle));

        when(mMockPhone.getPhoneType()).thenReturn(PhoneConstants.PHONE_TYPE_CDMA);
        bundle.putBoolean(CarrierConfigManager.KEY_SHOW_APN_SETTING_CDMA_BOOL, true);
        assertTrue(mCdmaOptions.shouldAddApnExpandPreference(bundle));
    }
}
