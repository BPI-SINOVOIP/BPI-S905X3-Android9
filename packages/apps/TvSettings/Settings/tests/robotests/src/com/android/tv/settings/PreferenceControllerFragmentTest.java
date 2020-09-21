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

package com.android.tv.settings;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.nullable;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.content.Context;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;

import com.android.settingslib.core.AbstractPreferenceController;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;
import org.robolectric.RuntimeEnvironment;

import java.util.Collection;

@RunWith(TvSettingsRobolectricTestRunner.class)
public class PreferenceControllerFragmentTest {

    @Spy
    private PreferenceControllerFragment mPreferenceControllerFragment;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        doReturn(RuntimeEnvironment.application).when(mPreferenceControllerFragment).getContext();
    }

    @Test
    public void testPreferenceControllerGetterSetter_shouldAddAndGetProperly() {
        final AbstractPreferenceController mockController =
                mock(AbstractPreferenceController.class);
        mPreferenceControllerFragment.addPreferenceController(mockController);

        final AbstractPreferenceController retrievedController =
                mPreferenceControllerFragment.getOnePreferenceController(mockController.getClass());

        assertThat(mockController).isSameAs(retrievedController);
    }

    @Test
    public void testOnPreferenceTreeClick_shouldCallPreferenceController() {
        final AbstractPreferenceController mockController =
                mock(AbstractPreferenceController.class);
        mPreferenceControllerFragment.addPreferenceController(mockController);

        final Preference mockPref = mock(Preference.class);
        mPreferenceControllerFragment.onPreferenceTreeClick(mockPref);

        verify(mockController, times(1)).handlePreferenceTreeClick(mockPref);
    }

    @Test
    public void testUpdatePreferenceStates_shouldCallUpdateState() {
        final AbstractPreferenceController mockController =
                mock(AbstractPreferenceController.class);
        doReturn("TEST_KEY").when(mockController).getPreferenceKey();
        doReturn(true).when(mockController).isAvailable();
        mPreferenceControllerFragment.addPreferenceController(mockController);

        final Preference mockPreference = mock(Preference.class);
        final PreferenceScreen mockScreen = mock(PreferenceScreen.class);
        doReturn(mockPreference).when(mockScreen).findPreference("TEST_KEY");
        doReturn(mockScreen).when(mPreferenceControllerFragment).getPreferenceScreen();

        mPreferenceControllerFragment.updatePreferenceStates();

        verify(mockController, times(1)).updateState(mockPreference);
    }

    @Test
    public void testUpdatePreferenceStates_shouldNotCallUnavailable() {
        final AbstractPreferenceController mockController =
                mock(AbstractPreferenceController.class);
        doReturn("TEST_KEY").when(mockController).getPreferenceKey();
        doReturn(false).when(mockController).isAvailable();
        mPreferenceControllerFragment.addPreferenceController(mockController);

        final Preference mockPreference = mock(Preference.class);
        final PreferenceScreen mockScreen = mock(PreferenceScreen.class);
        doReturn(mockPreference).when(mockScreen).findPreference("TEST_KEY");
        doReturn(mockScreen).when(mPreferenceControllerFragment).getPreferenceScreen();

        mPreferenceControllerFragment.updatePreferenceStates();

        verify(mockController, never()).updateState(nullable(Preference.class));
    }

    @Test
    public void testUpdatePreferenceStates_shouldNotCallNoPref() {
        final AbstractPreferenceController mockController =
                mock(AbstractPreferenceController.class);
        doReturn("TEST_KEY").when(mockController).getPreferenceKey();
        doReturn(true).when(mockController).isAvailable();
        mPreferenceControllerFragment.addPreferenceController(mockController);

        final PreferenceScreen mockScreen = mock(PreferenceScreen.class);
        doReturn(null).when(mockScreen).findPreference("TEST_KEY");
        doReturn(mockScreen).when(mPreferenceControllerFragment).getPreferenceScreen();

        mPreferenceControllerFragment.updatePreferenceStates();

        verify(mockController, never()).updateState(nullable(Preference.class));
    }

    @Test
    public void testMultipleControllerClassInstances() {
        class MultiplePreferenceController extends AbstractPreferenceController {

            private final String mPreferenceKey;

            private MultiplePreferenceController(Context context, String preferenceKey) {
                super(context);
                mPreferenceKey = preferenceKey;
            }

            @Override
            public boolean isAvailable() {
                return true;
            }

            @Override
            public String getPreferenceKey() {
                return mPreferenceKey;
            }
        }

        final AbstractPreferenceController instance1 =
                new MultiplePreferenceController(RuntimeEnvironment.application, "One");
        final AbstractPreferenceController instance2 =
                new MultiplePreferenceController(RuntimeEnvironment.application, "Two");

        mPreferenceControllerFragment.addPreferenceController(instance1);
        mPreferenceControllerFragment.addPreferenceController(instance2);

        assertThat(mPreferenceControllerFragment
                .getOnePreferenceController(MultiplePreferenceController.class)).isNotNull();

        final Collection<MultiplePreferenceController> controllers =
                mPreferenceControllerFragment.getPreferenceControllers(
                        MultiplePreferenceController.class);

        assertThat(controllers).contains(instance1);
        assertThat(controllers).contains(instance2);
        assertThat(controllers).hasSize(2);
    }
}
