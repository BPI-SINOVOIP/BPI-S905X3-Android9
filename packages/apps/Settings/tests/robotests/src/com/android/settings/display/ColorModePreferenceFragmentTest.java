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
package com.android.settings.display;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.os.Bundle;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;

import com.android.internal.app.ColorDisplayController;
import com.android.internal.logging.nano.MetricsProto;
import com.android.settings.R;
import com.android.settings.applications.LayoutPreference;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.SettingsShadowResources;
import com.android.settingslib.widget.CandidateInfo;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.util.ReflectionHelpers;

import java.util.List;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = SettingsShadowResources.class)
public class ColorModePreferenceFragmentTest {

    private ColorModePreferenceFragment mFragment;

    @Mock
    private ColorDisplayController mController;

    @Mock
    private Activity mActivity;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);

        mFragment = spy(new ColorModePreferenceFragment());
        ReflectionHelpers.setField(mFragment, "mController", mController);
    }

    @Test
    public void verifyMetricsConstant() {
        assertThat(mFragment.getMetricsCategory())
                .isEqualTo(MetricsProto.MetricsEvent.COLOR_MODE_SETTINGS);
    }

    @Test
    public void getCandidates_all() {
        when(mFragment.getContext()).thenReturn(RuntimeEnvironment.application);
        SettingsShadowResources.overrideResource(
                com.android.internal.R.array.config_availableColorModes, new int[]{
                    ColorDisplayController.COLOR_MODE_NATURAL,
                    ColorDisplayController.COLOR_MODE_BOOSTED,
                    ColorDisplayController.COLOR_MODE_SATURATED,
                    ColorDisplayController.COLOR_MODE_AUTOMATIC
                });
        List<? extends CandidateInfo> candidates = mFragment.getCandidates();

        assertThat(candidates.size()).isEqualTo(4);
        assertThat(candidates.get(0).getKey())
                .isEqualTo(ColorModePreferenceFragment.KEY_COLOR_MODE_NATURAL);
        assertThat(candidates.get(1).getKey())
                .isEqualTo(ColorModePreferenceFragment.KEY_COLOR_MODE_BOOSTED);
        assertThat(candidates.get(2).getKey())
                .isEqualTo(ColorModePreferenceFragment.KEY_COLOR_MODE_SATURATED);
        assertThat(candidates.get(3).getKey())
                .isEqualTo(ColorModePreferenceFragment.KEY_COLOR_MODE_AUTOMATIC);
    }

    @Test
    public void getCandidates_none() {
        when(mFragment.getContext()).thenReturn(RuntimeEnvironment.application);
        SettingsShadowResources.overrideResource(
                com.android.internal.R.array.config_availableColorModes, null);
        List<? extends CandidateInfo> candidates = mFragment.getCandidates();

        assertThat(candidates.size()).isEqualTo(0);
    }

    @Test
    public void getCandidates_withAutomatic() {
        when(mFragment.getContext()).thenReturn(RuntimeEnvironment.application);
        SettingsShadowResources.overrideResource(
                com.android.internal.R.array.config_availableColorModes, new int[]{
                    ColorDisplayController.COLOR_MODE_NATURAL,
                    ColorDisplayController.COLOR_MODE_AUTOMATIC
                });
        List<? extends CandidateInfo> candidates = mFragment.getCandidates();

        assertThat(candidates.size()).isEqualTo(2);
        assertThat(candidates.get(0).getKey())
                .isEqualTo(ColorModePreferenceFragment.KEY_COLOR_MODE_NATURAL);
        assertThat(candidates.get(1).getKey())
                .isEqualTo(ColorModePreferenceFragment.KEY_COLOR_MODE_AUTOMATIC);
    }

    @Test
    public void getCandidates_withoutAutomatic() {
        when(mFragment.getContext()).thenReturn(RuntimeEnvironment.application);
        SettingsShadowResources.overrideResource(
                com.android.internal.R.array.config_availableColorModes, new int[]{
                    ColorDisplayController.COLOR_MODE_NATURAL,
                    ColorDisplayController.COLOR_MODE_BOOSTED,
                    ColorDisplayController.COLOR_MODE_SATURATED,
                });
        List<? extends CandidateInfo> candidates = mFragment.getCandidates();

        assertThat(candidates.size()).isEqualTo(3);
        assertThat(candidates.get(0).getKey())
                .isEqualTo(ColorModePreferenceFragment.KEY_COLOR_MODE_NATURAL);
        assertThat(candidates.get(1).getKey())
                .isEqualTo(ColorModePreferenceFragment.KEY_COLOR_MODE_BOOSTED);
        assertThat(candidates.get(2).getKey())
                .isEqualTo(ColorModePreferenceFragment.KEY_COLOR_MODE_SATURATED);
    }

    @Test
    public void getKey_natural() {
        when(mController.getColorMode())
            .thenReturn(ColorDisplayController.COLOR_MODE_NATURAL);

        assertThat(mFragment.getDefaultKey())
                .isEqualTo(ColorModePreferenceFragment.KEY_COLOR_MODE_NATURAL);
    }

    @Test
    public void getKey_boosted() {
        when(mController.getColorMode())
            .thenReturn(ColorDisplayController.COLOR_MODE_BOOSTED);

        assertThat(mFragment.getDefaultKey())
                .isEqualTo(ColorModePreferenceFragment.KEY_COLOR_MODE_BOOSTED);
    }

    @Test
    public void getKey_saturated() {
        when(mController.getColorMode())
            .thenReturn(ColorDisplayController.COLOR_MODE_SATURATED);

        assertThat(mFragment.getDefaultKey())
            .isEqualTo(ColorModePreferenceFragment.KEY_COLOR_MODE_SATURATED);
    }

    @Test
    public void getKey_automatic() {
        when(mController.getColorMode())
            .thenReturn(ColorDisplayController.COLOR_MODE_AUTOMATIC);

        assertThat(mFragment.getDefaultKey())
            .isEqualTo(ColorModePreferenceFragment.KEY_COLOR_MODE_AUTOMATIC);
    }

    @Test
    public void setKey_natural() {
        mFragment.setDefaultKey(ColorModePreferenceFragment.KEY_COLOR_MODE_NATURAL);
        verify(mController).setColorMode(ColorDisplayController.COLOR_MODE_NATURAL);
    }

    @Test
    public void setKey_boosted() {
        mFragment.setDefaultKey(ColorModePreferenceFragment.KEY_COLOR_MODE_BOOSTED);
        verify(mController).setColorMode(ColorDisplayController.COLOR_MODE_BOOSTED);
    }

    @Test
    public void setKey_saturated() {
        mFragment.setDefaultKey(ColorModePreferenceFragment.KEY_COLOR_MODE_SATURATED);
        verify(mController).setColorMode(ColorDisplayController.COLOR_MODE_SATURATED);
    }

    @Test
    public void setKey_automatic() {
        mFragment.setDefaultKey(ColorModePreferenceFragment.KEY_COLOR_MODE_AUTOMATIC);
        verify(mController).setColorMode(ColorDisplayController.COLOR_MODE_AUTOMATIC);
    }

    @Test
    public void onCreatePreferences_useNewTitle_shouldAddColorModePreferences() {
        doNothing().when(mFragment).addPreferencesFromResource(anyInt());
        doNothing().when(mFragment).updateCandidates();

        mFragment.onCreatePreferences(Bundle.EMPTY, null /* rootKey */);

        verify(mFragment).addPreferencesFromResource(R.xml.color_mode_settings);
    }

    @Test
    public void addStaticPreferences_shouldAddPreviewImage() {
        PreferenceScreen mockPreferenceScreen = mock(PreferenceScreen.class);
        LayoutPreference mockPreview = mock(LayoutPreference.class);

        ArgumentCaptor<Preference> preferenceCaptor = ArgumentCaptor.forClass(Preference.class);

        mFragment.configureAndInstallPreview(mockPreview, mockPreferenceScreen);
        verify(mockPreview, times(1)).setSelectable(false);
        verify(mockPreferenceScreen, times(1)).addPreference(preferenceCaptor.capture());

        assertThat(preferenceCaptor.getValue()).isEqualTo(mockPreview);
    }

    @Test
    public void onAccessibilityTransformChanged_toggles() {
        when(mFragment.getActivity()).thenReturn(mActivity);
        mFragment.onAccessibilityTransformChanged(true /* state */);
        verify(mActivity).onBackPressed();
    }
}
