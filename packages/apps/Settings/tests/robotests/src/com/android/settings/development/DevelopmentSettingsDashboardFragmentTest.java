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

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.os.UserManager;
import android.provider.SearchIndexableResource;
import android.provider.Settings;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settings.R;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.SettingsShadowResources;
import com.android.settings.widget.SwitchBar;
import com.android.settings.widget.ToggleSwitch;
import com.android.settingslib.development.AbstractEnableAdbPreferenceController;
import com.android.settingslib.development.DevelopmentSettingsEnabler;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadows.ShadowUserManager;
import org.robolectric.util.ReflectionHelpers;

import java.util.List;

@RunWith(SettingsRobolectricTestRunner.class)
public class DevelopmentSettingsDashboardFragmentTest {

    private ToggleSwitch mSwitch;
    private Context mContext;
    private ShadowUserManager mShadowUserManager;
    private DevelopmentSettingsDashboardFragment mDashboard;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = RuntimeEnvironment.application;
        SwitchBar switchBar = new SwitchBar(mContext);
        mSwitch = switchBar.getSwitch();
        mDashboard = spy(new DevelopmentSettingsDashboardFragment());
        ReflectionHelpers.setField(mDashboard, "mSwitchBar", switchBar);
        UserManager userManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        mShadowUserManager = Shadows.shadowOf(userManager);
        mShadowUserManager.setIsAdminUser(true);
    }

    @After
    public void tearDown() {
        ShadowEnableDevelopmentSettingWarningDialog.reset();
    }

    @Test
    public void shouldNotHaveHelpResource() {
        assertThat(mDashboard.getHelpResource()).isEqualTo(0);
    }

    @Test
    public void shouldLogAsFeatureFlagPage() {
        assertThat(mDashboard.getMetricsCategory())
            .isEqualTo(MetricsProto.MetricsEvent.DEVELOPMENT);
    }

    @Test
    public void searchIndex_shouldIndexFromPrefXml() {
        final List<SearchIndexableResource> index =
                DevelopmentSettingsDashboardFragment.SEARCH_INDEX_DATA_PROVIDER
                        .getXmlResourcesToIndex(RuntimeEnvironment.application, true);

        assertThat(index.size()).isEqualTo(1);
        assertThat(index.get(0).xmlResId).isEqualTo(R.xml.development_settings);
    }

    @Test
    @Config(shadows = SettingsShadowResources.SettingsShadowTheme.class)
    public void searchIndex_pageDisabledBySetting_shouldAddAllKeysToNonIndexable() {
        final Context appContext = RuntimeEnvironment.application;
        DevelopmentSettingsEnabler.setDevelopmentSettingsEnabled(appContext, false);

        final List<String> nonIndexableKeys =
                DevelopmentSettingsDashboardFragment.SEARCH_INDEX_DATA_PROVIDER
                        .getNonIndexableKeys(appContext);

        assertThat(nonIndexableKeys).contains("enable_adb");
    }

    @Test
    @Config(shadows = SettingsShadowResources.SettingsShadowTheme.class)
    public void searchIndex_pageDisabledForNonAdmin_shouldAddAllKeysToNonIndexable() {
        final Context appContext = RuntimeEnvironment.application;
        DevelopmentSettingsEnabler.setDevelopmentSettingsEnabled(appContext, true);
        mShadowUserManager.setIsAdminUser(false);
        mShadowUserManager.setIsDemoUser(false);

        final List<String> nonIndexableKeys =
                DevelopmentSettingsDashboardFragment.SEARCH_INDEX_DATA_PROVIDER
                        .getNonIndexableKeys(appContext);

        assertThat(nonIndexableKeys).contains("enable_adb");
    }

    @Test
    @Config(shadows = {
            ShadowPictureColorModePreferenceController.class,
            ShadowAdbPreferenceController.class,
            ShadowClearAdbKeysPreferenceController.class
    })
    public void searchIndex_pageEnabled_shouldNotAddKeysToNonIndexable() {
        final Context appContext = RuntimeEnvironment.application;
        DevelopmentSettingsEnabler.setDevelopmentSettingsEnabled(appContext, true);

        final List<String> nonIndexableKeys =
                DevelopmentSettingsDashboardFragment.SEARCH_INDEX_DATA_PROVIDER
                        .getNonIndexableKeys(appContext);

        assertThat(nonIndexableKeys).doesNotContain("development_prefs_screen");
    }

    @Test
    @Config(shadows = ShadowEnableDevelopmentSettingWarningDialog.class)
    public void onSwitchChanged_sameState_shouldDoNothing() {
        when(mDashboard.getContext()).thenReturn(mContext);
        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.DEVELOPMENT_SETTINGS_ENABLED, 0);

        mDashboard.onSwitchChanged(mSwitch, false /* isChecked */);
        assertThat(ShadowEnableDevelopmentSettingWarningDialog.mShown).isFalse();
    }

    @Test
    @Config(shadows = ShadowEnableDevelopmentSettingWarningDialog.class)
    public void onSwitchChanged_turnOn_shouldShowWarningDialog() {
        when(mDashboard.getContext()).thenReturn(mContext);
        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.DEVELOPMENT_SETTINGS_ENABLED, 0);

        mDashboard.onSwitchChanged(mSwitch, true /* isChecked */);
        assertThat(ShadowEnableDevelopmentSettingWarningDialog.mShown).isTrue();
    }

    @Test
    @Config(shadows = ShadowEnableDevelopmentSettingWarningDialog.class)
    public void onSwitchChanged_turnOff_shouldTurnOff() {
        when(mDashboard.getContext()).thenReturn(mContext);
        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.DEVELOPMENT_SETTINGS_ENABLED, 1);

        mDashboard.onSwitchChanged(mSwitch, false /* isChecked */);

        assertThat(ShadowEnableDevelopmentSettingWarningDialog.mShown).isFalse();
        assertThat(DevelopmentSettingsEnabler.isDevelopmentSettingsEnabled(mContext)).isFalse();
    }

    @Test
    public void onOemUnlockDialogConfirmed_shouldCallControllerOemConfirmed() {
        final OemUnlockPreferenceController controller = mock(OemUnlockPreferenceController.class);
        doReturn(controller).when(mDashboard)
            .getDevelopmentOptionsController(OemUnlockPreferenceController.class);
        mDashboard.onOemUnlockDialogConfirmed();
        verify(controller).onOemUnlockConfirmed();
    }

    @Test
    public void onOemUnlockDialogConfirmed_shouldCallControllerOemDismissed() {
        final OemUnlockPreferenceController controller = mock(OemUnlockPreferenceController.class);
        doReturn(controller).when(mDashboard)
            .getDevelopmentOptionsController(OemUnlockPreferenceController.class);
        mDashboard.onOemUnlockDialogDismissed();
        verify(controller).onOemUnlockDismissed();
    }

    @Test
    public void onAdbDialogConfirmed_shouldCallControllerDialogConfirmed() {
        final AdbPreferenceController controller = mock(AdbPreferenceController.class);
        doReturn(controller).when(mDashboard)
            .getDevelopmentOptionsController(AdbPreferenceController.class);
        mDashboard.onEnableAdbDialogConfirmed();

        verify(controller).onAdbDialogConfirmed();
    }

    @Test
    public void onAdbDialogDismissed_shouldCallControllerOemDismissed() {
        final AdbPreferenceController controller = mock(AdbPreferenceController.class);
        doReturn(controller).when(mDashboard)
            .getDevelopmentOptionsController(AdbPreferenceController.class);
        mDashboard.onEnableAdbDialogDismissed();

        verify(controller).onAdbDialogDismissed();
    }

    @Test
    public void onAdbClearKeysDialogConfirmed_shouldCallControllerDialogConfirmed() {
        final ClearAdbKeysPreferenceController controller =
            mock(ClearAdbKeysPreferenceController.class);
        doReturn(controller).when(mDashboard)
            .getDevelopmentOptionsController(ClearAdbKeysPreferenceController.class);
        mDashboard.onAdbClearKeysDialogConfirmed();

        verify(controller).onClearAdbKeysConfirmed();
    }

    @Test
    public void onDisableLogPersistDialogConfirmed_shouldCallControllerDialogConfirmed() {
        final LogPersistPreferenceController controller =
            mock(LogPersistPreferenceController.class);
        doReturn(controller).when(mDashboard)
            .getDevelopmentOptionsController(LogPersistPreferenceController.class);
        mDashboard.onDisableLogPersistDialogConfirmed();

        verify(controller).onDisableLogPersistDialogConfirmed();
    }

    @Test
    public void onDisableLogPersistDialogRejected_shouldCallControllerDialogRejected() {
        final LogPersistPreferenceController controller =
            mock(LogPersistPreferenceController.class);
        doReturn(controller).when(mDashboard)
            .getDevelopmentOptionsController(LogPersistPreferenceController.class);
        mDashboard.onDisableLogPersistDialogRejected();

        verify(controller).onDisableLogPersistDialogRejected();
    }

    @Implements(EnableDevelopmentSettingWarningDialog.class)
    public static class ShadowEnableDevelopmentSettingWarningDialog {

        static boolean mShown;

        public static void reset() {
            mShown = false;
        }

        @Implementation
        public static void show(DevelopmentSettingsDashboardFragment host) {
            mShown = true;
        }
    }

    @Implements(PictureColorModePreferenceController.class)
    public static class ShadowPictureColorModePreferenceController {
        @Implementation
        public boolean isAvailable() {
            return true;
        }
    }

    @Implements(AbstractEnableAdbPreferenceController.class)
    public static class ShadowAdbPreferenceController {
        @Implementation
        public boolean isAvailable() {
            return true;
        }
    }

    @Implements(ClearAdbKeysPreferenceController.class)
    public static class ShadowClearAdbKeysPreferenceController {

        @Implementation
        public boolean isAvailable() {
            return true;
        }
    }
}
