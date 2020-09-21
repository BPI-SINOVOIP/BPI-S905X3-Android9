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
package com.android.settings.dashboard;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.ArgumentMatchers.nullable;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.Icon;
import android.os.Bundle;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceManager;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.core.PreferenceControllerMixin;
import com.android.settings.testutils.FakeFeatureFactory;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settingslib.core.AbstractPreferenceController;
import com.android.settingslib.core.instrumentation.VisibilityLoggerMixin;
import com.android.settingslib.drawer.DashboardCategory;
import com.android.settingslib.drawer.Tile;
import com.android.settingslib.drawer.TileUtils;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.util.ReflectionHelpers;

import java.util.ArrayList;
import java.util.List;

@RunWith(SettingsRobolectricTestRunner.class)
public class DashboardFragmentTest {

    @Mock
    private FakeFeatureFactory mFakeFeatureFactory;
    private DashboardCategory mDashboardCategory;
    private Context mContext;
    private TestFragment mTestFragment;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = spy(RuntimeEnvironment.application);
        mFakeFeatureFactory = FakeFeatureFactory.setupForTest();
        mDashboardCategory = new DashboardCategory();
        mDashboardCategory.addTile(new Tile());
        mTestFragment = new TestFragment(RuntimeEnvironment.application);
        when(mFakeFeatureFactory.dashboardFeatureProvider
                .getTilesForCategory(nullable(String.class)))
                .thenReturn(mDashboardCategory);
        mTestFragment.onAttach(RuntimeEnvironment.application);
        when(mContext.getPackageName()).thenReturn("TestPackage");
    }

    @Test
    public void testPreferenceControllerGetterSetter_shouldAddAndGetProperly() {
        final TestPreferenceController controller = new TestPreferenceController(mContext);
        mTestFragment.addPreferenceController(controller);

        final TestPreferenceController retrievedController = mTestFragment.use
                (TestPreferenceController.class);

        assertThat(controller).isSameAs(retrievedController);
    }

    @Test
    public void testPreferenceControllerSetter_shouldAddAndNotReplace() {
        final TestPreferenceController controller1 = new TestPreferenceController(mContext);
        mTestFragment.addPreferenceController(controller1);
        final TestPreferenceController controller2 = new TestPreferenceController(mContext);
        mTestFragment.addPreferenceController(controller2);

        final TestPreferenceController retrievedController = mTestFragment.use
                (TestPreferenceController.class);

        assertThat(controller1).isSameAs(retrievedController);
    }

    @Test
    public void displayTilesAsPreference_shouldAddTilesWithIntent() {
        when(mFakeFeatureFactory.dashboardFeatureProvider
                .getTilesForCategory(nullable(String.class)))
                .thenReturn(mDashboardCategory);
        when(mFakeFeatureFactory.dashboardFeatureProvider
                .getDashboardKeyForTile(nullable(Tile.class)))
                .thenReturn("test_key");
        mTestFragment.onCreatePreferences(new Bundle(), "rootKey");

        verify(mTestFragment.mScreen).addPreference(nullable(Preference.class));
    }

    @Test
    public void displayTilesAsPreference_shouldNotAddTilesWithoutIntent() {
        mTestFragment.onCreatePreferences(new Bundle(), "rootKey");

        verify(mTestFragment.mScreen, never()).addPreference(nullable(Preference.class));
    }

    @Test
    public void displayTilesAsPreference_withEmptyCategory_shouldNotAddTiles() {
        mDashboardCategory.removeTile(0);
        mTestFragment.onCreatePreferences(new Bundle(), "rootKey");

        verify(mTestFragment.mScreen, never()).addPreference(nullable(Preference.class));
    }

    @Test
    public void onAttach_shouldCreatePlaceholderPreferenceController() {
        final AbstractPreferenceController controller = mTestFragment.use(
                DashboardTilePlaceholderPreferenceController.class);

        assertThat(controller).isNotNull();
    }

    @Test
    public void updateState_skipUnavailablePrefs() {
        final List<AbstractPreferenceController> preferenceControllers = mTestFragment.mControllers;
        final AbstractPreferenceController mockController1 =
                mock(AbstractPreferenceController.class);
        final AbstractPreferenceController mockController2 =
                mock(AbstractPreferenceController.class);
        when(mockController1.getPreferenceKey()).thenReturn("key1");
        when(mockController2.getPreferenceKey()).thenReturn("key2");
        preferenceControllers.add(mockController1);
        preferenceControllers.add(mockController2);
        when(mockController1.isAvailable()).thenReturn(false);
        when(mockController2.isAvailable()).thenReturn(true);
        mTestFragment.onAttach(RuntimeEnvironment.application);
        mTestFragment.onResume();

        verify(mockController1).getPreferenceKey();
        verify(mockController2, times(2)).getPreferenceKey();
    }

    @Test
    public void updateState_doesNotSkipControllersOfSameClass() {
        final AbstractPreferenceController mockController1 =
                mock(AbstractPreferenceController.class);
        final AbstractPreferenceController mockController2 =
                mock(AbstractPreferenceController.class);
        mTestFragment.addPreferenceController(mockController1);
        mTestFragment.addPreferenceController(mockController2);
        when(mockController1.isAvailable()).thenReturn(true);
        when(mockController2.isAvailable()).thenReturn(true);

        mTestFragment.updatePreferenceStates();

        verify(mockController1).getPreferenceKey();
        verify(mockController2).getPreferenceKey();
    }

    @Test
    public void tintTileIcon_hasMetadata_shouldReturnIconTintableMetadata() {
        final Tile tile = new Tile();
        tile.icon = mock(Icon.class);
        final Bundle metaData = new Bundle();
        tile.metaData = metaData;

        metaData.putBoolean(TileUtils.META_DATA_PREFERENCE_ICON_TINTABLE, false);
        assertThat(mTestFragment.tintTileIcon(tile)).isFalse();

        metaData.putBoolean(TileUtils.META_DATA_PREFERENCE_ICON_TINTABLE, true);
        assertThat(mTestFragment.tintTileIcon(tile)).isTrue();
    }

    @Test
    public void tintTileIcon_noIcon_shouldReturnFalse() {
        final Tile tile = new Tile();
        tile.metaData = new Bundle();

        assertThat(mTestFragment.tintTileIcon(tile)).isFalse();
    }

    @Test
    public void tintTileIcon_noMetadata_shouldReturnPackageNameCheck() {
        final Tile tile = new Tile();
        tile.icon = mock(Icon.class);
        final Intent intent = new Intent();
        tile.intent = intent;
        intent.setComponent(
            new ComponentName(RuntimeEnvironment.application.getPackageName(), "TestClass"));
        assertThat(mTestFragment.tintTileIcon(tile)).isFalse();

        intent.setComponent(new ComponentName("OtherPackage", "TestClass"));
        assertThat(mTestFragment.tintTileIcon(tile)).isTrue();
    }

    public static class TestPreferenceController extends AbstractPreferenceController
            implements PreferenceControllerMixin {

        private TestPreferenceController(Context context) {
            super(context);
        }

        @Override
        public boolean handlePreferenceTreeClick(Preference preference) {
            return false;
        }

        @Override
        public boolean isAvailable() {
            return false;
        }

        @Override
        public String getPreferenceKey() {
            return null;
        }

        @Override
        public void updateNonIndexableKeys(List<String> keys) {
        }
    }

    public static class TestFragment extends DashboardFragment {

        private final PreferenceManager mPreferenceManager;
        private final Context mContext;
        private final List<AbstractPreferenceController> mControllers;

        public final PreferenceScreen mScreen;

        public TestFragment(Context context) {
            mContext = context;
            mPreferenceManager = mock(PreferenceManager.class);
            mScreen = mock(PreferenceScreen.class);
            mControllers = new ArrayList<>();

            when(mPreferenceManager.getContext()).thenReturn(mContext);
            ReflectionHelpers.setField(
                    this, "mVisibilityLoggerMixin", mock(VisibilityLoggerMixin.class));
        }

        @Override
        public Context getContext() {
            return mContext;
        }

        @Override
        public int getMetricsCategory() {
            return 0;
        }

        @Override
        public PreferenceScreen getPreferenceScreen() {
            return mScreen;
        }

        @Override
        protected String getLogTag() {
            return "TEST_FRAG";
        }

        @Override
        protected int getPreferenceScreenResId() {
            return 0;
        }

        @Override
        protected List<AbstractPreferenceController> createPreferenceControllers(Context context) {
            return mControllers;
        }

        @Override
        public PreferenceManager getPreferenceManager() {
            return mPreferenceManager;
        }
    }
}
