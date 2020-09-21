/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.settings.dashboard.conditional;

import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.net.NetworkPolicyManager;
import android.util.FeatureFlagUtils;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;
import com.android.settings.Settings;
import com.android.settings.core.FeatureFlags;

public class BackgroundDataCondition extends Condition {

    public BackgroundDataCondition(ConditionManager manager) {
        super(manager);
    }

    @Override
    public void refreshState() {
        setActive(NetworkPolicyManager.from(mManager.getContext()).getRestrictBackground());
    }

    @Override
    public Drawable getIcon() {
        return mManager.getContext().getDrawable(R.drawable.ic_data_saver);
    }

    @Override
    public CharSequence getTitle() {
        return mManager.getContext().getString(R.string.condition_bg_data_title);
    }

    @Override
    public CharSequence getSummary() {
        return mManager.getContext().getString(R.string.condition_bg_data_summary);
    }

    @Override
    public CharSequence[] getActions() {
        return new CharSequence[] {mManager.getContext().getString(R.string.condition_turn_off)};
    }

    @Override
    public void onPrimaryClick() {
        final Class activityClass = FeatureFlagUtils.isEnabled(mManager.getContext(),
                FeatureFlags.DATA_USAGE_SETTINGS_V2)
                ? Settings.DataUsageSummaryActivity.class
                : Settings.DataUsageSummaryLegacyActivity.class;
        mManager.getContext().startActivity(new Intent(mManager.getContext(), activityClass)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
    }

    @Override
    public int getMetricsConstant() {
        return MetricsEvent.SETTINGS_CONDITION_BACKGROUND_DATA;
    }

    @Override
    public void onActionClick(int index) {
        if (index == 0) {
            NetworkPolicyManager.from(mManager.getContext()).setRestrictBackground(false);
            setActive(false);
        } else {
            throw new IllegalArgumentException("Unexpected index " + index);
        }
    }
}
