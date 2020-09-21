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
package com.android.emergency.preferences;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.nullable;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.SharedPreferences;
import android.support.v7.preference.PreferenceGroup;
import android.support.v7.preference.PreferenceManager;
import android.support.v7.preference.PreferenceScreen;
import android.text.Editable;
import android.view.View;
import android.widget.AutoCompleteTextView;

import com.android.emergency.PreferenceKeys;
import com.android.emergency.TestConfig;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.util.ReflectionHelpers;

/** Unit tests for {@link NameAutoCompletePreference}. */
@RunWith(RobolectricTestRunner.class)
@Config(manifest = TestConfig.MANIFEST_PATH, sdk = TestConfig.SDK_VERSION)
public class NameAutoCompletePreferenceTest {
    @Mock private PreferenceManager mPreferenceManager;
    @Mock private SharedPreferences mSharedPreferences;
    @Mock private NameAutoCompletePreference.SuggestionProvider mAutoCompleteSuggestionProvider;
    private NameAutoCompletePreference mPreference;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        when(mPreferenceManager.getSharedPreferences()).thenReturn(mSharedPreferences);

        Context context = RuntimeEnvironment.application;

        mPreference =
            spy(new NameAutoCompletePreference(context, null, mAutoCompleteSuggestionProvider));

        PreferenceGroup prefRoot = spy(new PreferenceScreen(context, null));
        when(prefRoot.getPreferenceManager()).thenReturn(mPreferenceManager);
        prefRoot.addPreference(mPreference);
    }

    @Test
    public void testProperties() {
        assertThat(mPreference).isNotNull();
        assertThat(mPreference.isEnabled()).isTrue();
        assertThat(mPreference.isPersistent()).isTrue();
        assertThat(mPreference.isSelectable()).isTrue();
        assertThat(mPreference.isNotSet()).isTrue();
    }

    @Test
    public void testReloadFromPreference() throws Throwable {
        mPreference.setKey(PreferenceKeys.KEY_NAME);

        String name = "John";
        when(mSharedPreferences.getString(eq(mPreference.getKey()), nullable(String.class)))
                .thenReturn(name);

        mPreference.reloadFromPreference();
        assertThat(mPreference.getText()).isEqualTo(name);
        assertThat(mPreference.isNotSet()).isFalse();
    }

    @Test
    public void testSetText() throws Throwable {
        final String name = "John";
        mPreference.setText(name);
        assertThat(mPreference.getText()).isEqualTo(name);
        assertThat(mPreference.getSummary()).isEqualTo(name);
    }

    @Test
    public void testGetAutoCompleteTextView() {
        AutoCompleteTextView autoCompleteTextView = mPreference.getAutoCompleteTextView();
        assertThat(autoCompleteTextView).isNotNull();
    }

    @Test
    public void testCreateAutocompleteSuggestions_noNameToSuggest() {
        when(mAutoCompleteSuggestionProvider.hasNameToSuggest()).thenReturn(false);
        assertThat(mPreference.createAutocompleteSuggestions()).isEqualTo(new String[] {});
    }

    @Test
    public void testCreateAutocompleteSuggestions_nameToSuggest() {
        final String name = "Jane";
        when(mAutoCompleteSuggestionProvider.hasNameToSuggest()).thenReturn(true);
        when(mAutoCompleteSuggestionProvider.getNameSuggestion()).thenReturn(name);
        assertThat(mPreference.createAutocompleteSuggestions()).isEqualTo(new String[] {name});
    }

    @Test
    public void testBindDialog_shouldFocusOnEditText() {
        final AutoCompleteTextView editText = mock(AutoCompleteTextView.class);
        final Editable text = mock(Editable.class);
        when(editText.getText()).thenReturn(text);
        when(text.length()).thenReturn(0);
        ReflectionHelpers.setField(mPreference, "mAutoCompleteTextView", editText);

        mPreference.onBindDialogView(mock(View.class));

        verify(editText).requestFocus();
    }
}
