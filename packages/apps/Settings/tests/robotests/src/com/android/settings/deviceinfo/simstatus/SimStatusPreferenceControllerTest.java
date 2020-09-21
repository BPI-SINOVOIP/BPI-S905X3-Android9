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

package com.android.settings.deviceinfo.simstatus;

import static org.mockito.ArgumentMatchers.anyString;
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
public class SimStatusPreferenceControllerTest {

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
    private SimStatusPreferenceController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = spy(RuntimeEnvironment.application);
        doReturn(mUserManager).when(mContext).getSystemService(UserManager.class);
        mController = spy(new SimStatusPreferenceController(mContext, mFragment));
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
    public void displayPreference_multiSim_shouldAddSecondPreference() {
        when(mTelephonyManager.getPhoneCount()).thenReturn(2);

        mController.displayPreference(mScreen);

        verify(mScreen).addPreference(mSecondSimPreference);
    }

    @Test
    public void updateState_singleSim_shouldSetSingleSimTitleAndSummary() {
        when(mTelephonyManager.getPhoneCount()).thenReturn(1);
        mController.displayPreference(mScreen);

        mController.updateState(mPreference);

        verify(mPreference).setTitle(mContext.getString(R.string.sim_status_title));
        verify(mPreference).setSummary(anyString());
    }

    @Test
    public void updateState_multiSim_shouldSetMultiSimTitleAndSummary() {
        when(mTelephonyManager.getPhoneCount()).thenReturn(2);
        mController.displayPreference(mScreen);

        mController.updateState(mPreference);

        verify(mPreference).setTitle(
                mContext.getString(R.string.sim_status_title_sim_slot, 1 /* sim slot */));
        verify(mSecondSimPreference).setTitle(
                mContext.getString(R.string.sim_status_title_sim_slot, 2 /* sim slot */));
        verify(mPreference).setSummary(anyString());
        verify(mSecondSimPreference).setSummary(anyString());
    }

    @Test
    public void handlePreferenceTreeClick_shouldStartDialogFragment() {
        when(mFragment.getChildFragmentManager()).thenReturn(
                mock(FragmentManager.class, Answers.RETURNS_DEEP_STUBS));
        mController.displayPreference(mScreen);

        mController.handlePreferenceTreeClick(mPreference);

        verify(mFragment).getChildFragmentManager();
    }
}
