/*
 * Copyright (c) 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.systemui.qs.tiles;

import static com.android.internal.logging.nano.MetricsProto.MetricsEvent.FIELD_QS_MODE;

import android.annotation.Nullable;
import android.app.ActivityManager;
import android.content.Intent;
import android.metrics.LogMaker;
import android.provider.Settings;
import android.service.quicksettings.Tile;
import android.support.annotation.StringRes;
import android.util.Log;
import android.widget.Switch;

import com.android.internal.app.ColorDisplayController;
import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.systemui.R;
import com.android.systemui.qs.QSHost;
import com.android.systemui.plugins.qs.QSTile.BooleanState;
import com.android.systemui.qs.tileimpl.QSTileImpl;

import java.time.LocalTime;
import java.time.format.DateTimeFormatter;

public class NightDisplayTile extends QSTileImpl<BooleanState>
        implements ColorDisplayController.Callback {

    /**
     * Pattern for {@link java.time.format.DateTimeFormatter} used to approximate the time to the
     * nearest hour and add on the AM/PM indicator.
     */
    private static final String PATTERN_HOUR = "h a";
    private static final String PATTERN_HOUR_MINUTE = "h:mm a";


    private ColorDisplayController mController;
    private boolean mIsListening;

    public NightDisplayTile(QSHost host) {
        super(host);
        mController = new ColorDisplayController(mContext, ActivityManager.getCurrentUser());
    }

    @Override
    public boolean isAvailable() {
        return ColorDisplayController.isAvailable(mContext);
    }

    @Override
    public BooleanState newTileState() {
        return new BooleanState();
    }

    @Override
    protected void handleClick() {
        // Enroll in forced auto mode if eligible.
        if ("1".equals(Settings.Global.getString(mContext.getContentResolver(),
                Settings.Global.NIGHT_DISPLAY_FORCED_AUTO_MODE_AVAILABLE))
                && mController.getAutoModeRaw() == -1) {
            mController.setAutoMode(ColorDisplayController.AUTO_MODE_CUSTOM);
            Log.i("NightDisplayTile", "Enrolled in forced night display auto mode");
        }

        // Change current activation state.
        final boolean activated = !mState.value;
        mController.setActivated(activated);
    }

    @Override
    protected void handleUserSwitch(int newUserId) {
        // Stop listening to the old controller.
        if (mIsListening) {
            mController.setListener(null);
        }

        // Make a new controller for the new user.
        mController = new ColorDisplayController(mContext, newUserId);
        if (mIsListening) {
            mController.setListener(this);
        }

        super.handleUserSwitch(newUserId);
    }

    @Override
    protected void handleUpdateState(BooleanState state, Object arg) {
        state.value = mController.isActivated();
        state.label = state.contentDescription =
                mContext.getString(R.string.quick_settings_night_display_label);
        state.icon = ResourceIcon.get(R.drawable.ic_qs_night_display_on);
        state.expandedAccessibilityClassName = Switch.class.getName();
        state.state = state.value ? Tile.STATE_ACTIVE : Tile.STATE_INACTIVE;
        state.secondaryLabel = getSecondaryLabel(state.value);
    }

    /**
     * Returns a {@link String} for the secondary label that reflects when the light will be turned
     * on or off based on the current auto mode and night light activated status.
     */
    @Nullable
    private String getSecondaryLabel(boolean isNightLightActivated) {
        switch(mController.getAutoMode()) {
            case ColorDisplayController.AUTO_MODE_TWILIGHT:
                // Auto mode related to sunrise & sunset. If the light is on, it's guaranteed to be
                // turned off at sunrise. If it's off, it's guaranteed to be turned on at sunset.
                return isNightLightActivated
                        ? mContext.getString(
                                R.string.quick_settings_night_secondary_label_until_sunrise)
                        : mContext.getString(
                                R.string.quick_settings_night_secondary_label_on_at_sunset);

            case ColorDisplayController.AUTO_MODE_CUSTOM:
                // User-specified time, approximated to the nearest hour.
                final @StringRes int toggleTimeStringRes;
                final LocalTime toggleTime;
                final DateTimeFormatter toggleTimeFormat;

                if (isNightLightActivated) {
                    toggleTime = mController.getCustomEndTime();
                    toggleTimeStringRes = R.string.quick_settings_secondary_label_until;
                } else {
                    toggleTime = mController.getCustomStartTime();
                    toggleTimeStringRes = R.string.quick_settings_night_secondary_label_on_at;
                }

                // Choose between just showing the hour or also showing the minutes (based on the
                // user-selected toggle time). This helps reduce how much space the label takes.
                toggleTimeFormat = DateTimeFormatter.ofPattern(
                        toggleTime.getMinute() == 0 ? PATTERN_HOUR : PATTERN_HOUR_MINUTE);

                return mContext.getString(toggleTimeStringRes, toggleTime.format(toggleTimeFormat));

            default:
                // No secondary label when auto mode is disabled.
                return null;
        }
    }

    @Override
    public int getMetricsCategory() {
        return MetricsEvent.QS_NIGHT_DISPLAY;
    }

    @Override
    public LogMaker populate(LogMaker logMaker) {
        return super.populate(logMaker).addTaggedData(FIELD_QS_MODE, mController.getAutoModeRaw());
    }

    @Override
    public Intent getLongClickIntent() {
        return new Intent(Settings.ACTION_NIGHT_DISPLAY_SETTINGS);
    }

    @Override
    protected void handleSetListening(boolean listening) {
        mIsListening = listening;
        if (listening) {
            mController.setListener(this);
            refreshState();
        } else {
            mController.setListener(null);
        }
    }

    @Override
    public CharSequence getTileLabel() {
        return mContext.getString(R.string.quick_settings_night_display_label);
    }

    @Override
    public void onActivated(boolean activated) {
        refreshState();
    }
}
