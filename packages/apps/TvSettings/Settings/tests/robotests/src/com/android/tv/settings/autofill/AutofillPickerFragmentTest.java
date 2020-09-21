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

package com.android.tv.settings.autofill;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.robolectric.shadow.api.Shadow.extract;

import android.os.UserManager;
import android.provider.Settings;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;

import com.android.tv.settings.RadioPreference;
import com.android.tv.settings.TvSettingsRobolectricTestRunner;
import com.android.tv.settings.testutils.ShadowInputMethodManager;
import com.android.tv.settings.testutils.ShadowUserManager;
import com.android.tv.settings.testutils.TvShadowActivityThread;
import com.android.tv.settings.testutils.Utils;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowSettings;

@RunWith(TvSettingsRobolectricTestRunner.class)
@Config(shadows = {
        ShadowUserManager.class,
        TvShadowActivityThread.class,
        ShadowInputMethodManager.class})
public class AutofillPickerFragmentTest {
    @Spy
    private AutofillPickerFragment mFragment;

    private ShadowUserManager mUserManager;

    @Mock
    private PreferenceScreen mPreferenceScreen;

    private RadioPreference mNonePreference;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mUserManager = extract(RuntimeEnvironment.application.getSystemService(UserManager.class));
        mUserManager.setIsAdminUser(true);
        doReturn(RuntimeEnvironment.application).when(mFragment).getContext();
        mFragment.onAttach(RuntimeEnvironment.application);

        doReturn(mPreferenceScreen).when(mFragment).getPreferenceScreen();
        mNonePreference = new RadioPreference(mFragment.getContext());
        doReturn(mNonePreference).when(mPreferenceScreen).findPreference(
                AutofillPickerFragment.KEY_FOR_NONE);
    }

    @Test
    public void selectedService() {

        Utils.addAutofill("com.test.AutofillPackage", "com.test.AutofillPackage.MyService");

        ShadowSettings.ShadowGlobal.putString(mFragment.getContext().getContentResolver(),
                Settings.Secure.AUTOFILL_SERVICE,
                "com.test.AutofillPackage/com.test.AutofillPackage.MyService");

        mFragment.bind(mPreferenceScreen, false);

        ArgumentCaptor<Preference> captor = ArgumentCaptor.forClass(Preference.class);
        verify(mPreferenceScreen, times(1)).addPreference(captor.capture());
        RadioPreference pref = (RadioPreference) captor.getValue();
        assertThat("com.test.AutofillPackage.MyService").isEqualTo(pref.getTitle());
        assertThat(pref.isChecked()).isTrue();

        assertThat(mNonePreference.isChecked()).isFalse();
    }

    @Test
    public void selectedNone() {

        Utils.addAutofill("com.test.AutofillPackage", "com.test.AutofillPackage.MyService");

        mFragment.bind(mPreferenceScreen, false);

        ArgumentCaptor<Preference> captor = ArgumentCaptor.forClass(Preference.class);
        verify(mPreferenceScreen, times(1)).addPreference(captor.capture());
        RadioPreference pref = (RadioPreference) captor.getValue();
        assertThat("com.test.AutofillPackage.MyService").isEqualTo(pref.getTitle());
        assertThat(pref.isChecked()).isFalse();

        assertThat(mNonePreference.isChecked()).isTrue();
    }
}
