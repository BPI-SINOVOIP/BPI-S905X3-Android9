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

import static com.android.settings.widget.EntityHeaderController.PREF_KEY_APP_HEADER;

import android.app.Activity;
import android.arch.lifecycle.LifecycleObserver;
import android.arch.lifecycle.OnLifecycleEvent;
import android.content.Context;
import android.support.v14.preference.PreferenceFragment;
import android.support.v7.preference.Preference;
import android.text.BidiFormatter;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.util.Slog;
import android.view.View;

import com.android.settings.R;
import com.android.settings.applications.LayoutPreference;
import com.android.settings.core.PreferenceControllerMixin;
import com.android.settings.widget.EntityHeaderController;
import com.android.settingslib.core.lifecycle.Lifecycle;

public class HeaderPreferenceController extends NotificationPreferenceController
        implements PreferenceControllerMixin, LifecycleObserver {

    private final PreferenceFragment mFragment;
    private EntityHeaderController mHeaderController;
    private boolean mStarted = false;

    public HeaderPreferenceController(Context context, PreferenceFragment fragment) {
        super(context, null);
        mFragment = fragment;
    }

    @Override
    public String getPreferenceKey() {
        return PREF_KEY_APP_HEADER;
    }

    @Override
    public boolean isAvailable() {
        return mAppRow != null;
    }

    @Override
    public void updateState(Preference preference) {
        if (mAppRow != null && mFragment != null) {

            Activity activity = null;
            if (mStarted) {
                // don't call done on an activity if it hasn't started yet
                activity = mFragment.getActivity();
            }

            LayoutPreference pref = (LayoutPreference) preference;
            mHeaderController = EntityHeaderController.newInstance(
                    mFragment.getActivity(), mFragment, pref.findViewById(R.id.entity_header));
            pref = mHeaderController.setIcon(mAppRow.icon)
                    .setLabel(getLabel())
                    .setSummary(getSummary())
                    .setPackageName(mAppRow.pkg)
                    .setUid(mAppRow.uid)
                    .setButtonActions(EntityHeaderController.ActionType.ACTION_NOTIF_PREFERENCE,
                            EntityHeaderController.ActionType.ACTION_NONE)
                    .setHasAppInfoLink(true)
                    .done(activity, mContext);
            pref.findViewById(R.id.entity_header).setVisibility(View.VISIBLE);
        }
    }

    CharSequence getLabel() {
        return (mChannel != null && !isDefaultChannel()) ? mChannel.getName()
                : mChannelGroup != null
                        ? mChannelGroup.getName()
                        : mAppRow.label;
    }

    @Override
    public CharSequence getSummary() {
        if (mChannel != null && !isDefaultChannel()) {
            if (mChannelGroup != null
                    && !TextUtils.isEmpty(mChannelGroup.getName())) {
                final SpannableStringBuilder summary = new SpannableStringBuilder();
                BidiFormatter bidi = BidiFormatter.getInstance();
                summary.append(bidi.unicodeWrap(mAppRow.label.toString()));
                summary.append(bidi.unicodeWrap(mContext.getText(
                        R.string.notification_header_divider_symbol_with_spaces)));
                summary.append(bidi.unicodeWrap(mChannelGroup.getName().toString()));
                return summary.toString();
            } else {
                return mAppRow.label.toString();
            }
        } else if (mChannelGroup != null) {
            return mAppRow.label.toString();
        } else {
            return "";
        }
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_START)
    public void onStart() {
        mStarted = true;
        if (mHeaderController != null) {
            mHeaderController.styleActionBar(mFragment.getActivity());
        }
    }
}
