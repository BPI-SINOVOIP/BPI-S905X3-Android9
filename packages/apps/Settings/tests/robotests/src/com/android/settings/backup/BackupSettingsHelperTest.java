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
 * limitations under the License
 */

package com.android.settings.backup;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.backup.BackupManager;
import android.app.backup.IBackupManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.os.IBinder;
import android.os.RemoteException;

import com.android.settings.R;
import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = BackupSettingsHelperTest.ShadowBackupManagerStub.class)
public class BackupSettingsHelperTest {

    private static final String DEFAULT_SETTINGS_CLASSNAME =
            "com.android.settings.Settings$PrivacySettingsActivity";

    private static final int DEFAULT_SUMMARY_RESOURCE =
            R.string.backup_configure_account_default_summary;

    private static final int DEFAULT_LABEL_RESOURCE =
            R.string.privacy_settings_title;

    private static final int MANUFACTURER_INTENT_RESOURCE = R.string.config_backup_settings_intent;

    private static final int MANUFACTURER_LABEL_RESOURCE = R.string.config_backup_settings_label;

    private Context mContext;

    private BackupSettingsHelper mBackupSettingsHelper;

    @Mock
    private static IBackupManager mBackupManager;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mContext = spy(RuntimeEnvironment.application.getApplicationContext());
        when(mBackupManager.getCurrentTransport()).thenReturn("test_transport");
        mBackupSettingsHelper = new BackupSettingsHelper(mContext);
    }

    @Test
    public void testGetIntentFromBackupTransport() throws Exception {
        Intent intent = new Intent();

        when(mBackupManager.getDataManagementIntent(anyString())).thenReturn(intent);

        Intent backupIntent = mBackupSettingsHelper.getIntentForBackupSettingsFromTransport();

        assertThat(backupIntent).isEqualTo(intent);

        verify(mBackupManager).getDataManagementIntent(anyString());
    }

    @Test
    public void testGetIntentFromBackupTransport_WithIntent() throws Exception {
        Intent intent = mock(Intent.class);

        when(mBackupManager.getDataManagementIntent(anyString())).thenReturn(intent);

        Intent backupIntent = mBackupSettingsHelper.getIntentForBackupSettingsFromTransport();

        assertThat(backupIntent).isEqualTo(intent);
    }

    @Test
    public void testGetIntentFromBackupTransport_WithNullIntent() throws Exception {
        when(mBackupManager.getDataManagementIntent(anyString())).thenReturn(null);

        Intent backupIntent = mBackupSettingsHelper.getIntentForBackupSettingsFromTransport();

        assertThat(backupIntent).isNull();
    }

    @Test
    public void testGetIntentFromBackupTransport_RemoteException() throws Exception {
        when(mBackupManager.getDataManagementIntent(anyString())).thenThrow(new RemoteException());

        Intent backupIntent = mBackupSettingsHelper.getIntentForBackupSettingsFromTransport();

        assertThat(backupIntent).isNull();
    }

    @Test
    public void testGetIntentFromBackupTransport_BackupEnabled() throws Exception {
        Intent intent = new Intent("test_intent");

        when(mBackupManager.getDataManagementIntent(anyString())).thenReturn(intent);
        when(mBackupManager.isBackupServiceActive(anyInt())).thenReturn(true);

        Intent backupIntent = mBackupSettingsHelper.getIntentForBackupSettingsFromTransport();

        assertThat(backupIntent.getExtras().get(BackupManager.EXTRA_BACKUP_SERVICES_AVAILABLE))
                .isEqualTo(true);
    }

    @Test
    public void testGetIntentFromBackupTransport_BackupDisabled() throws Exception {
        Intent intent = new Intent("test_intent");

        when(mBackupManager.getDataManagementIntent(anyString())).thenReturn(intent);
        when(mBackupManager.isBackupServiceActive(anyInt())).thenReturn(false);

        Intent backupIntent = mBackupSettingsHelper.getIntentForBackupSettingsFromTransport();

        assertThat(backupIntent.getExtras().get(BackupManager.EXTRA_BACKUP_SERVICES_AVAILABLE))
                .isEqualTo(false);
    }

    @Test
    public void testGetIntentFromBackupTransport_BackupStatusException() throws Exception {
        Intent intent = new Intent("test_intent");

        when(mBackupManager.getDataManagementIntent(anyString())).thenReturn(intent);
        when(mBackupManager.isBackupServiceActive(anyInt())).thenThrow(new RemoteException());

        Intent backupIntent = mBackupSettingsHelper.getIntentForBackupSettingsFromTransport();

        assertThat(backupIntent.getExtras().get(BackupManager.EXTRA_BACKUP_SERVICES_AVAILABLE))
                .isEqualTo(false);
    }

    @Test
    public void testIsIntentProvidedByTransport_WithNullIntent() throws Exception {
        when(mBackupManager.getDataManagementIntent(anyString())).thenReturn(null);

        boolean isIntentProvided = mBackupSettingsHelper.isIntentProvidedByTransport();

        assertThat(isIntentProvided).isFalse();
    }

    @Test
    public void testIsIntentProvidedByTransport_WithInvalidIntent() throws Exception {
        Intent intent = mock(Intent.class);

        when(mBackupManager.getDataManagementIntent(anyString())).thenReturn(intent);

        PackageManager packageManager = mock(PackageManager.class);
        when(mContext.getPackageManager()).thenReturn(packageManager);
        when(intent.resolveActivity(packageManager)).thenReturn(null);

        boolean isIntentProvided = mBackupSettingsHelper.isIntentProvidedByTransport();

        assertThat(isIntentProvided).isFalse();
    }

    @Test
    public void testIsIntentProvidedByTransport_WithIntent() throws Exception {
        Intent intent = mock(Intent.class);

        when(mBackupManager.getDataManagementIntent(anyString())).thenReturn(intent);

        PackageManager packageManager = mock(PackageManager.class);
        when(mContext.getPackageManager()).thenReturn(packageManager);
        when(intent.resolveActivity(packageManager)).thenReturn(mock(ComponentName.class));

        boolean isIntentProvided = mBackupSettingsHelper.isIntentProvidedByTransport();

        assertThat(isIntentProvided).isTrue();
    }

    @Test
    public void testGetSummaryFromBackupTransport() throws Exception {
        String summary = "test_summary";

        when(mBackupManager.getDestinationString(anyString())).thenReturn(summary);

        String backupSummary = mBackupSettingsHelper.getSummaryFromBackupTransport();

        assertThat(backupSummary).isEqualTo(summary);
    }

    @Test
    public void testGetSummaryFromBackupTransport_RemoteException() throws Exception {
        when(mBackupManager.getDestinationString(anyString())).thenThrow(new RemoteException());

        String backupSummary = mBackupSettingsHelper.getSummaryFromBackupTransport();

        assertThat(backupSummary).isNull();
    }

    @Test
    public void testGetLabelBackupTransport() throws Exception {
        String label = "test_label";

        when(mBackupManager.getDataManagementLabel(anyString())).thenReturn(label);

        String backupLabel = mBackupSettingsHelper.getLabelFromBackupTransport();

        assertThat(backupLabel).isEqualTo(label);
    }

    @Test
    public void testGetLabelBackupTransport_RemoteException() throws Exception {
        when(mBackupManager.getDataManagementLabel(anyString())).thenThrow(new RemoteException());

        String backupLabel = mBackupSettingsHelper.getLabelFromBackupTransport();

        assertThat(backupLabel).isNull();
    }

    @Test
    public void testGetIntentForBackupSettings_WithIntentFromTransport() throws Exception {
        Intent intent = mock(Intent.class);

        when(mBackupManager.getDataManagementIntent(anyString())).thenReturn(intent);

        PackageManager packageManager = mock(PackageManager.class);
        when(mContext.getPackageManager()).thenReturn(packageManager);
        when(intent.resolveActivity(packageManager)).thenReturn(mock(ComponentName.class));

        Intent backupIntent = mBackupSettingsHelper.getIntentForBackupSettings();

        assertThat(backupIntent).isEqualTo(intent);
    }

    @Test
    public void testGetIntentForBackupSettings_WithoutIntentFromTransport() throws Exception {
        when(mBackupManager.getDataManagementIntent(anyString())).thenReturn(null);

        Intent backupIntent = mBackupSettingsHelper.getIntentForBackupSettings();

        assertThat(backupIntent.getComponent().getClassName())
            .isEqualTo(DEFAULT_SETTINGS_CLASSNAME);
    }

    @Test
    public void testGetLabelForBackupSettings_WithLabelFromTransport() throws Exception {
        String label = "test_label";

        when(mBackupManager.getDataManagementLabel(anyString())).thenReturn(label);

        String backupLabel = mBackupSettingsHelper.getLabelForBackupSettings();

        assertThat(backupLabel).isEqualTo(label);
    }

    @Test
    public void testGetLabelForBackupSettings_WithEmptyLabelFromTransport() throws Exception {
        String label = "";

        when(mBackupManager.getDataManagementLabel(anyString())).thenReturn(label);

        String backupLabel = mBackupSettingsHelper.getLabelForBackupSettings();

        assertThat(backupLabel).isEqualTo(mContext.getString(DEFAULT_LABEL_RESOURCE));
    }

    @Test
    public void testGetLabelForBackupSettings_WithoutLabelFromTransport() throws Exception {
        when(mBackupManager.getDataManagementLabel(anyString())).thenReturn(null);

        String backupLabel = mBackupSettingsHelper.getLabelForBackupSettings();

        assertThat(backupLabel).isEqualTo(mContext.getString(DEFAULT_LABEL_RESOURCE));
    }

    @Test
    public void testGetSummaryForBackupSettings_WithSummaryFromTransport() throws Exception {
        String summary = "test_summary";

        when(mBackupManager.getDestinationString(anyString())).thenReturn(summary);

        String backupSummary = mBackupSettingsHelper.getSummaryForBackupSettings();

        assertThat(backupSummary).isEqualTo(summary);
    }

    @Test
    public void testGetSummaryForBackupSettings_WithoutSummaryFromTransport() throws Exception {
        when(mBackupManager.getDestinationString(anyString())).thenReturn(null);

        String backupSummary = mBackupSettingsHelper.getSummaryForBackupSettings();

        assertThat(backupSummary).isEqualTo(mContext.getString(DEFAULT_SUMMARY_RESOURCE));
    }

    @Test
    public void testIsBackupProvidedByManufacturer_WithIntent() throws Exception {
        String intent = "test_intent";

        when(mContext.getApplicationContext()).thenReturn(mContext);
        Resources spiedResources = spy(mContext.getResources());
        when(mContext.getResources()).thenReturn(spiedResources);
        when(spiedResources.getString(MANUFACTURER_INTENT_RESOURCE)).thenReturn(intent);
        mBackupSettingsHelper = new BackupSettingsHelper(mContext);

        boolean hasManufacturerIntent = mBackupSettingsHelper.isBackupProvidedByManufacturer();

        assertThat(hasManufacturerIntent).isTrue();
    }

    @Test
    public void testIsBackupProvidedByManufacturer_WithoutIntent() throws Exception {
        String intent = "";

        when(mContext.getApplicationContext()).thenReturn(mContext);
        Resources spiedResources = spy(mContext.getResources());
        when(mContext.getResources()).thenReturn(spiedResources);
        when(spiedResources.getString(MANUFACTURER_INTENT_RESOURCE)).thenReturn(intent);
        mBackupSettingsHelper = new BackupSettingsHelper(mContext);

        boolean hasManufacturerIntent = mBackupSettingsHelper.isBackupProvidedByManufacturer();

        assertThat(hasManufacturerIntent).isFalse();
    }

    @Test
    public void testGetLabelProvidedByManufacturer() throws Exception {
        String label = "test_label";

        when(mContext.getApplicationContext()).thenReturn(mContext);
        Resources spiedResources = spy(mContext.getResources());
        when(mContext.getResources()).thenReturn(spiedResources);
        when(spiedResources.getString(MANUFACTURER_LABEL_RESOURCE)).thenReturn(label);
        mBackupSettingsHelper = new BackupSettingsHelper(mContext);

        String manufacturerLabel = mBackupSettingsHelper.getLabelProvidedByManufacturer();

        assertThat(manufacturerLabel).isEqualTo(label);
    }

    @Test
    public void testGetIntentProvidedByManufacturer() throws Exception {
        String intent = "test_intent";

        when(mContext.getApplicationContext()).thenReturn(mContext);
        Resources spiedResources = spy(mContext.getResources());
        when(mContext.getResources()).thenReturn(spiedResources);
        when(spiedResources.getString(MANUFACTURER_INTENT_RESOURCE)).thenReturn(intent);
        mBackupSettingsHelper = new BackupSettingsHelper(mContext);

        Intent manufacturerIntent = mBackupSettingsHelper.getIntentProvidedByManufacturer();

        assertThat(manufacturerIntent).isNotNull();
    }

    @Implements(IBackupManager.Stub.class)
    public static class ShadowBackupManagerStub {
        @Implementation
        public static IBackupManager asInterface(IBinder iBinder) {
            return mBackupManager;
        }
    }
}
