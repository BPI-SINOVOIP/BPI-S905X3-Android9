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

import android.content.Context;
import android.support.annotation.Nullable;
import android.support.v7.preference.Preference;

import com.android.settings.core.PreferenceControllerMixin;
import com.android.settingslib.core.lifecycle.Lifecycle;
import com.android.settingslib.development.AbstractLogpersistPreferenceController;

public class LogPersistPreferenceController extends AbstractLogpersistPreferenceController
        implements PreferenceControllerMixin {

    private final DevelopmentSettingsDashboardFragment mFragment;

    public LogPersistPreferenceController(Context context,
            DevelopmentSettingsDashboardFragment fragment, Lifecycle lifecycle) {
        super(context, lifecycle);

        mFragment = fragment;
    }

    @Override
    public void showConfirmationDialog(@Nullable Preference preference) {
        DisableLogPersistWarningDialog.show(mFragment);
    }

    @Override
    public void dismissConfirmationDialog() {
        // intentional no-op
    }

    @Override
    public boolean isConfirmationDialogShowing() {
        return false;
    }

    @Override
    public void updateState(Preference preference) {
        updateLogpersistValues();
    }

    @Override
    protected void onDeveloperOptionsSwitchDisabled() {
        super.onDeveloperOptionsSwitchDisabled();
        writeLogpersistOption(null /* new value */, true);
    }

    public void onDisableLogPersistDialogConfirmed() {
        setLogpersistOff(true);
        updateLogpersistValues();
    }

    public void onDisableLogPersistDialogRejected() {
        updateLogpersistValues();
    }
}
