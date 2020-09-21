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

package com.android.settings.development;

import static android.arch.lifecycle.Lifecycle.Event.ON_PAUSE;
import static android.arch.lifecycle.Lifecycle.Event.ON_RESUME;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.arch.lifecycle.LifecycleOwner;
import android.content.Context;
import android.content.res.Resources;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.R;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.core.lifecycle.Lifecycle;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(SettingsRobolectricTestRunner.class)
public class PictureColorModePreferenceControllerTest {

    @Mock
    private ColorModePreference mPreference;
    @Mock
    private Context mContext;
    @Mock
    private PreferenceScreen mPreferenceScreen;
    @Mock
    private Resources mResources;

    private LifecycleOwner mLifecycleOwner;
    private Lifecycle mLifecycle;
    private PictureColorModePreferenceController mController;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);
        mLifecycleOwner = () -> mLifecycle;
        mLifecycle = new Lifecycle(mLifecycleOwner);
        mController = new PictureColorModePreferenceController(mContext, mLifecycle);
        when(mPreferenceScreen.findPreference(mController.getPreferenceKey()))
            .thenReturn(mPreference);
        when(mContext.getResources()).thenReturn(mResources);
        when(mResources.getIntArray(R.array.color_mode_ids)).thenReturn(new int[0]);
        mController.displayPreference(mPreferenceScreen);
    }

    @Test
    public void isAvailable_shouldReturnFalseWhenWideColorGambit() {
        mController = spy(mController);
        doReturn(2).when(mController).getColorModeDescriptionsSize();
        doReturn(true).when(mController).isWideColorGamut();

        assertThat(mController.isAvailable()).isFalse();
    }

    @Test
    public void isAvailable_shouldReturnTrueWhenNotWideColorGambit() {
        mController = spy(mController);
        doReturn(2).when(mController).getColorModeDescriptionsSize();
        doReturn(false).when(mController).isWideColorGamut();

        assertThat(mController.isAvailable()).isTrue();
    }

    @Test
    public void isAvailable_shouldReturnFalseWhenColorCountIsOne() {
        mController = spy(mController);
        doReturn(1).when(mController).getColorModeDescriptionsSize();
        doReturn(true).when(mController).isWideColorGamut();

        assertThat(mController.isAvailable()).isFalse();
    }

    @Test
    public void isAvailable_shouldReturnTrueWhenColorCountIsTwo() {
        mController = spy(mController);
        doReturn(2).when(mController).getColorModeDescriptionsSize();
        doReturn(false).when(mController).isWideColorGamut();

        assertThat(mController.isAvailable()).isTrue();
    }

    @Test
    public void onDeveloperOptionEnabled_shouldEnablePreference() {
        mController = spy(mController);
        doReturn(true).when(mController).isAvailable();
        mController.onDeveloperOptionsEnabled();

        verify(mPreference).setEnabled(true);
    }

    @Test
    public void onDeveloperOptionDisabled_shouldDisablePreference() {
        mController = spy(mController);
        doReturn(true).when(mController).isAvailable();
        mController.onDeveloperOptionsDisabled();

        verify(mPreference).setEnabled(false);
    }

    @Test
    public void onResume_shouldStartListening() {
        mLifecycle.handleLifecycleEvent(ON_RESUME);

        verify(mPreference).startListening();
    }

    @Test
    public void onPause_shouldStopListening() {
        mLifecycle.handleLifecycleEvent(ON_RESUME);
        mLifecycle.handleLifecycleEvent(ON_PAUSE);

        verify(mPreference).stopListening();
    }
}
