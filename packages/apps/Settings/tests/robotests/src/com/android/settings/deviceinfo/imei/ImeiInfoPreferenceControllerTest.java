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

package com.android.settings.deviceinfo.imei;

import static android.telephony.TelephonyManager.PHONE_TYPE_CDMA;
import static android.telephony.TelephonyManager.PHONE_TYPE_GSM;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Fragment;
import android.app.FragmentManager;
import android.content.Context;
import android.os.UserManager;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.telephony.TelephonyManager;

import com.android.settings.R;
import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Answers;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.util.ReflectionHelpers;

@RunWith(SettingsRobolectricTestRunner.class)
public class ImeiInfoPreferenceControllerTest {

    @Mock
    private Preference mPreference;
    @Mock
    private Preference mSecondSimPreference;
    @Mock
    private PreferenceScreen mScreen;
    @Mock
    private TelephonyManager mTelephonyManager;
    @Mock
    private UserManager mUserManager;
    @Mock
    private Fragment mFragment;

    private Context mContext;
    private ImeiInfoPreferenceController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = spy(RuntimeEnvironment.application);
        doReturn(mUserManager).when(mContext).getSystemService(UserManager.class);
        mController = spy(new ImeiInfoPreferenceController(mContext, mFragment));
        doReturn(true).when(mController).isAvailable();
        when(mScreen.getContext()).thenReturn(mContext);
        doReturn(mSecondSimPreference).when(mController).createNewPreference(mContext);
        ReflectionHelpers.setField(mController, "mTelephonyManager", mTelephonyManager);
        when(mScreen.findPreference(mController.getPreferenceKey())).thenReturn(mPreference);
        final String prefKey = mController.getPreferenceKey();
        when(mPreference.getKey()).thenReturn(prefKey);
        when(mPreference.isVisible()).thenReturn(true);
    }

    @Test
    public void displayPreference_multiSimGsm_shouldAddSecondPreference() {
        ReflectionHelpers.setField(mController, "mIsMultiSim", true);
        when(mTelephonyManager.getPhoneCount()).thenReturn(2);
        when(mTelephonyManager.getPhoneType()).thenReturn(PHONE_TYPE_GSM);

        mController.displayPreference(mScreen);

        verify(mScreen).addPreference(mSecondSimPreference);
    }

    @Test
    public void displayPreference_singleSimCdmaPhone_shouldSetSingleSimCdmaTitleAndMeid() {
        ReflectionHelpers.setField(mController, "mIsMultiSim", false);
        final String meid = "125132215123";
        when(mTelephonyManager.getPhoneType()).thenReturn(PHONE_TYPE_CDMA);
        doReturn(meid).when(mController).getMeid(anyInt());

        mController.displayPreference(mScreen);

        verify(mPreference).setTitle(mContext.getString(R.string.status_meid_number));
        verify(mPreference).setSummary(meid);
    }

    @Test
    public void displayPreference_multiSimCdmaPhone_shouldSetMultiSimCdmaTitleAndMeid() {
        ReflectionHelpers.setField(mController, "mIsMultiSim", true);
        final String meid = "125132215123";
        when(mTelephonyManager.getPhoneCount()).thenReturn(2);
        when(mTelephonyManager.getPhoneType()).thenReturn(PHONE_TYPE_CDMA);
        doReturn(meid).when(mController).getMeid(anyInt());

        mController.displayPreference(mScreen);

        verify(mPreference).setTitle(mContext.getString(R.string.meid_multi_sim, 1 /* sim slot */));
        verify(mSecondSimPreference).setTitle(
                mContext.getString(R.string.meid_multi_sim, 2 /* sim slot */));
        verify(mPreference).setSummary(meid);
        verify(mSecondSimPreference).setSummary(meid);
    }

    @Test
    public void displayPreference_singleSimGsmPhone_shouldSetSingleSimGsmTitleAndImei() {
        ReflectionHelpers.setField(mController, "mIsMultiSim", false);
        final String imei = "125132215123";
        when(mTelephonyManager.getPhoneType()).thenReturn(PHONE_TYPE_GSM);
        when(mTelephonyManager.getImei(anyInt())).thenReturn(imei);

        mController.displayPreference(mScreen);

        verify(mPreference).setTitle(mContext.getString(R.string.status_imei));
        verify(mPreference).setSummary(imei);
    }

    @Test
    public void displayPreference_multiSimGsmPhone_shouldSetMultiSimGsmTitleAndImei() {
        ReflectionHelpers.setField(mController, "mIsMultiSim", true);
        final String imei = "125132215123";
        when(mTelephonyManager.getPhoneCount()).thenReturn(2);
        when(mTelephonyManager.getPhoneType()).thenReturn(PHONE_TYPE_GSM);
        when(mTelephonyManager.getImei(anyInt())).thenReturn(imei);

        mController.displayPreference(mScreen);

        verify(mPreference).setTitle(mContext.getString(R.string.imei_multi_sim, 1 /* sim slot */));
        verify(mSecondSimPreference).setTitle(
                mContext.getString(R.string.imei_multi_sim, 2 /* sim slot */));
        verify(mPreference).setSummary(imei);
        verify(mSecondSimPreference).setSummary(imei);
    }

    @Test
    public void handlePreferenceTreeClick_shouldStartDialogFragment() {
        when(mFragment.getChildFragmentManager())
            .thenReturn(mock(FragmentManager.class, Answers.RETURNS_DEEP_STUBS));
        when(mPreference.getTitle()).thenReturn("SomeTitle");
        mController.displayPreference(mScreen);

        mController.handlePreferenceTreeClick(mPreference);

        verify(mFragment).getChildFragmentManager();
    }
}
