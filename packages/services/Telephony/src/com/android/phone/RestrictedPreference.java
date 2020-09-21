/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.phone;

import static com.android.settingslib.RestrictedLockUtils.EnforcedAdmin;

import android.content.Context;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.util.AttributeSet;
import android.view.View;
import android.widget.TextView;

import com.android.settingslib.RestrictedLockUtils;

/**
 * Preference class that supports being disabled by a device admin.
 *
 * <p>This class is a mimic of ../../../frameworks/base/packages/SettingsLib/src/com/android
 * /settingslib/RestrictedPreference.java,
 * but support framework {@link Preference}.
 */
public class RestrictedPreference extends Preference {
    private final Context mContext;

    private boolean mDisabledByAdmin;
    private EnforcedAdmin mEnforcedAdmin;

    public RestrictedPreference(Context context, AttributeSet attrs,
            int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        mContext = context;

        setLayoutResource(com.android.settingslib.R.layout.preference_two_target);
        setWidgetLayoutResource(R.layout.restricted_icon);
    }

    public RestrictedPreference(Context context, AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, 0);
    }

    public RestrictedPreference(Context context, AttributeSet attrs) {
        this(context, attrs, android.R.attr.preferenceStyle);
    }

    public RestrictedPreference(Context context) {
        this(context, null);
    }

    @Override
    public void performClick(PreferenceScreen preferenceScreen) {
        if (mDisabledByAdmin) {
            RestrictedLockUtils.sendShowAdminSupportDetailsIntent(mContext, mEnforcedAdmin);
        } else {
            super.performClick(preferenceScreen);
        }
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);

        final View divider = view.findViewById(com.android.settingslib.R.id.two_target_divider);
        final View widgetFrame = view.findViewById(android.R.id.widget_frame);
        final View restrictedIcon = view.findViewById(R.id.restricted_icon);
        final TextView summaryView = view.findViewById(android.R.id.summary);
        if (divider != null) {
            divider.setVisibility(mDisabledByAdmin ? View.VISIBLE : View.GONE);
        }
        if (widgetFrame != null) {
            widgetFrame.setVisibility(mDisabledByAdmin ? View.VISIBLE : View.GONE);
        }
        if (restrictedIcon != null) {
            restrictedIcon.setVisibility(mDisabledByAdmin ? View.VISIBLE : View.GONE);
        }
        if (summaryView != null && mDisabledByAdmin) {
            summaryView.setText(com.android.settingslib.R.string.disabled_by_admin_summary_text);
            summaryView.setVisibility(View.VISIBLE);
        }

        if (mDisabledByAdmin) {
            view.setEnabled(true);
        }
    }

    @Override
    public void setEnabled(boolean enabled) {
        if (enabled && mDisabledByAdmin) {
            setDisabledByAdmin(null);
            return;
        }
        super.setEnabled(enabled);
    }

    /**
     * Disable this preference based on the enforce admin.
     *
     * @param admin Details of the admin who enforced the restriction. If it is {@code null}, then
     * this preference will be enabled. Otherwise, it will be disabled.
     */
    public void setDisabledByAdmin(EnforcedAdmin admin) {
        final boolean disabled = admin != null;
        mEnforcedAdmin = admin;
        boolean changed = false;
        if (mDisabledByAdmin != disabled) {
            mDisabledByAdmin = disabled;
            changed = true;
        }
        setEnabled(!disabled);
        if (changed) {
            notifyChanged();
        }
    }
}
