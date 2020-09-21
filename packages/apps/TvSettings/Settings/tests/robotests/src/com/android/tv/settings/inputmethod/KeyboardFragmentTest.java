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

package com.android.tv.settings.inputmethod;

import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.robolectric.shadow.api.Shadow.extract;

import android.os.UserManager;
import android.provider.Settings;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceCategory;
import android.support.v7.preference.PreferenceScreen;

import com.android.tv.settings.R;
import com.android.tv.settings.TvSettingsRobolectricTestRunner;
import com.android.tv.settings.testutils.ShadowInputMethodManager;
import com.android.tv.settings.testutils.ShadowUserManager;
import com.android.tv.settings.testutils.TvShadowActivityThread;
import com.android.tv.settings.testutils.Utils;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
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
public class KeyboardFragmentTest {
    @Spy
    private KeyboardFragment mKeyboardFragment;

    private ShadowUserManager mUserManager;

    @Mock
    private PreferenceScreen mPreferenceScreen;

    @Mock
    private PreferenceCategory mAutofillCategory;

    @Mock
    private Preference mCurrentAutofill;

    @Mock
    private PreferenceCategory mKeyboardCategory;

    @Mock
    private ListPreference mCurrentKeyboard;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mUserManager = extract(RuntimeEnvironment.application.getSystemService(UserManager.class));
        mUserManager.setIsAdminUser(true);
        doReturn(RuntimeEnvironment.application).when(mKeyboardFragment).getContext();
        mKeyboardFragment.onAttach(RuntimeEnvironment.application);
        doReturn(RuntimeEnvironment.application).when(mKeyboardFragment).getPreferenceContext();

        doReturn(mPreferenceScreen).when(mKeyboardFragment).getPreferenceScreen();

        doReturn(mKeyboardCategory).when(mKeyboardFragment).findPreference(
                KeyboardFragment.KEY_KEYBOARD_CATEGORY);

        doReturn(mCurrentKeyboard).when(mKeyboardFragment).findPreference(
                KeyboardFragment.KEY_CURRENT_KEYBOARD);
        doReturn(1).when(mKeyboardCategory).getPreferenceCount();
        doReturn(mCurrentKeyboard).when(mKeyboardCategory).getPreference(0);

        doReturn(mAutofillCategory).when(mKeyboardFragment).findPreference(
                KeyboardFragment.KEY_AUTOFILL_CATEGORY);

        doReturn(mCurrentAutofill).when(mKeyboardFragment).findPreference(
                KeyboardFragment.KEY_CURRENT_AUTOFILL);
        doReturn(1).when(mAutofillCategory).getPreferenceCount();
        doReturn(mCurrentAutofill).when(mAutofillCategory).getPreference(0);
    }

    @Test
    public void testUpdateAutofillSettings_noCandiate() {

        mKeyboardFragment.updateUi();

        verify(mPreferenceScreen, atLeastOnce()).setTitle(R.string.system_keyboard);
        verify(mPreferenceScreen, never()).setTitle(R.string.system_keyboard_autofill);

        verify(mKeyboardCategory, atLeastOnce()).setVisible(false);
        verify(mKeyboardCategory, never()).setVisible(true);

        verify(mAutofillCategory, atLeastOnce()).setVisible(false);
        verify(mAutofillCategory, never()).setVisible(true);
    }

    @Test
    public void testUpdateAutofillSettings_selected() {

        Utils.addAutofill("com.test.AutofillPackage", "com.test.AutofillPackage.MyService");

        ShadowSettings.ShadowGlobal.putString(mKeyboardFragment.getContext().getContentResolver(),
                Settings.Secure.AUTOFILL_SERVICE,
                "com.test.AutofillPackage/com.test.AutofillPackage.MyService");

        mKeyboardFragment.updateUi();

        verify(mPreferenceScreen, atLeastOnce()).setTitle(R.string.system_keyboard_autofill);
        verify(mPreferenceScreen, never()).setTitle(R.string.system_keyboard);

        verify(mKeyboardCategory, atLeastOnce()).setVisible(true);
        verify(mKeyboardCategory, never()).setVisible(false);

        verify(mAutofillCategory, atLeastOnce()).setVisible(true);
        verify(mAutofillCategory, never()).setVisible(false);

        verify(mCurrentAutofill, atLeastOnce()).setSummary(
                "com.test.AutofillPackage.MyService");
    }

    @Test
    public void testUpdateAutofillSettings_selectedNone() {

        Utils.addAutofill("com.test.AutofillPackage", "com.test.AutofillPackage.MyService");

        ShadowSettings.ShadowGlobal.putString(mKeyboardFragment.getContext().getContentResolver(),
                Settings.Secure.AUTOFILL_SERVICE, null);

        mKeyboardFragment.updateUi();

        verify(mPreferenceScreen, atLeastOnce()).setTitle(R.string.system_keyboard_autofill);
        verify(mPreferenceScreen, never()).setTitle(R.string.system_keyboard);

        verify(mKeyboardCategory, atLeastOnce()).setVisible(true);
        verify(mKeyboardCategory, never()).setVisible(false);

        verify(mAutofillCategory, atLeastOnce()).setVisible(true);
        verify(mAutofillCategory, never()).setVisible(false);

        verify(mCurrentAutofill, atLeastOnce()).setSummary(
                mKeyboardFragment.getContext().getString(R.string.autofill_none));
    }

}
