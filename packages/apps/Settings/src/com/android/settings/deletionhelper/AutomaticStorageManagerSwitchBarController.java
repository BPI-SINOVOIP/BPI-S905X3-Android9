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

package com.android.settings.deletionhelper;

import android.app.FragmentManager;
import android.content.Context;
import android.os.SystemProperties;
import android.provider.Settings;
import android.support.v7.preference.Preference;
import android.widget.Switch;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.internal.util.Preconditions;
import com.android.settings.widget.SwitchBar;
import com.android.settingslib.core.instrumentation.MetricsFeatureProvider;
import com.android.settingslib.Utils;

/** Handles the logic for flipping the storage management toggle on a {@link SwitchBar}. */
public class AutomaticStorageManagerSwitchBarController
        implements SwitchBar.OnSwitchChangeListener {
    private static final String STORAGE_MANAGER_ENABLED_BY_DEFAULT_PROPERTY =
            "ro.storage_manager.enabled";

    private Context mContext;
    private SwitchBar mSwitchBar;
    private MetricsFeatureProvider mMetrics;
    private Preference mDaysToRetainPreference;
    private FragmentManager mFragmentManager;

    public AutomaticStorageManagerSwitchBarController(
            Context context,
            SwitchBar switchBar,
            MetricsFeatureProvider metrics,
            Preference daysToRetainPreference,
            FragmentManager fragmentManager) {
        mContext = Preconditions.checkNotNull(context);
        mSwitchBar = Preconditions.checkNotNull(switchBar);
        mMetrics = Preconditions.checkNotNull(metrics);
        mDaysToRetainPreference = Preconditions.checkNotNull(daysToRetainPreference);
        mFragmentManager = Preconditions.checkNotNull(fragmentManager);

        initializeCheckedStatus();
    }

    private void initializeCheckedStatus() {
        mSwitchBar.setChecked(Utils.isStorageManagerEnabled(mContext));
        mSwitchBar.addOnSwitchChangeListener(this);
    }

    @Override
    public void onSwitchChanged(Switch switchView, boolean isChecked) {
        mMetrics.action(mContext, MetricsEvent.ACTION_TOGGLE_STORAGE_MANAGER, isChecked);
        mDaysToRetainPreference.setEnabled(isChecked);
        Settings.Secure.putInt(
                mContext.getContentResolver(),
                Settings.Secure.AUTOMATIC_STORAGE_MANAGER_ENABLED,
                isChecked ? 1 : 0);
        // Only show a warning if enabling.
        if (isChecked) {
            maybeShowWarning();
        }
    }

    /** Unregisters the controller from listening to further events. */
    public void tearDown() {
        mSwitchBar.removeOnSwitchChangeListener(this);
    }

    private void maybeShowWarning() {
        // If the storage manager is on by default, we don't need to show the additional dialog.
        if (SystemProperties.getBoolean(STORAGE_MANAGER_ENABLED_BY_DEFAULT_PROPERTY, false)) {
            return;
        }
        ActivationWarningFragment fragment = ActivationWarningFragment.newInstance();
        fragment.show(mFragmentManager, ActivationWarningFragment.TAG);
    }
}
