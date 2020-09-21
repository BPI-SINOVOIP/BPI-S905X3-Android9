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

package com.android.settings.notification;

import static android.app.NotificationChannel.DEFAULT_CHANNEL_ID;
import static android.app.NotificationManager.IMPORTANCE_DEFAULT;
import static android.app.NotificationManager.IMPORTANCE_NONE;
import static android.app.NotificationManager.IMPORTANCE_UNSPECIFIED;

import android.app.NotificationManager;
import android.content.Context;
import android.support.v7.preference.Preference;
import android.widget.Switch;

import com.android.settings.R;
import com.android.settings.applications.LayoutPreference;
import com.android.settings.core.PreferenceControllerMixin;
import com.android.settings.widget.SwitchBar;

public class BlockPreferenceController extends NotificationPreferenceController
        implements PreferenceControllerMixin, SwitchBar.OnSwitchChangeListener {

    private static final String KEY_BLOCK = "block";
    private NotificationSettingsBase.ImportanceListener mImportanceListener;

    public BlockPreferenceController(Context context,
            NotificationSettingsBase.ImportanceListener importanceListener,
            NotificationBackend backend) {
        super(context, backend);
        mImportanceListener = importanceListener;
    }

    @Override
    public String getPreferenceKey() {
        return KEY_BLOCK;
    }

    @Override
    public boolean isAvailable() {
        if (mAppRow == null) {
            return false;
        }
        if (mChannel != null) {
            return isChannelBlockable();
        } else if (mChannelGroup != null) {
            return isChannelGroupBlockable();
        } else {
            return !mAppRow.systemApp || (mAppRow.systemApp && mAppRow.banned);
        }
    }

    public void updateState(Preference preference) {
        LayoutPreference pref = (LayoutPreference) preference;
        SwitchBar bar = pref.findViewById(R.id.switch_bar);
        if (bar != null) {
            bar.setSwitchBarText(R.string.notification_switch_label,
                    R.string.notification_switch_label);
            bar.show();
            try {
                bar.addOnSwitchChangeListener(this);
            } catch (IllegalStateException e) {
                // an exception is thrown if you try to add the listener twice
            }
            bar.setDisabledByAdmin(mAdmin);

            if (mChannel != null) {
                bar.setChecked(!mAppRow.banned
                        && mChannel.getImportance() != NotificationManager.IMPORTANCE_NONE);
            } else if (mChannelGroup != null) {
                bar.setChecked(!mAppRow.banned && !mChannelGroup.isBlocked());
            } else {
                bar.setChecked(!mAppRow.banned);
            }
        }
    }

    @Override
    public void onSwitchChanged(Switch switchView, boolean isChecked) {
        boolean blocked = !isChecked;
        if (mChannel != null) {
            final int originalImportance = mChannel.getImportance();
            // setting the initial state of the switch in updateState() triggers this callback.
            // It's always safe to override the importance if it's meant to be blocked or if
            // it was blocked and we are unblocking it.
            if (blocked || originalImportance == IMPORTANCE_NONE) {
                final int importance = blocked ? IMPORTANCE_NONE
                        : isDefaultChannel() ? IMPORTANCE_UNSPECIFIED : IMPORTANCE_DEFAULT;
                mChannel.setImportance(importance);
                saveChannel();
            }
            if (mBackend.onlyHasDefaultChannel(mAppRow.pkg, mAppRow.uid)) {
                if (mAppRow.banned != blocked) {
                    mAppRow.banned = blocked;
                    mBackend.setNotificationsEnabledForPackage(mAppRow.pkg, mAppRow.uid, !blocked);
                }
            }
        } else if (mChannelGroup != null) {
            mChannelGroup.setBlocked(blocked);
            mBackend.updateChannelGroup(mAppRow.pkg, mAppRow.uid, mChannelGroup);
        } else if (mAppRow != null) {
            mAppRow.banned = blocked;
            mBackend.setNotificationsEnabledForPackage(mAppRow.pkg, mAppRow.uid, !blocked);
        }
        mImportanceListener.onImportanceChanged();
    }
}
