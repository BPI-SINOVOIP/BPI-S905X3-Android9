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
package com.android.car.settings.display;

import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE;
import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE_AUTOMATIC;
import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL;

import android.content.Context;
import android.os.Bundle;
import android.provider.Settings;

import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.ListItemProvider.ListProvider;
import androidx.car.widget.TextListItem;

import com.android.car.settings.R;
import com.android.car.settings.common.ListItemSettingsFragment;

import java.util.ArrayList;
import java.util.List;

/**
 * Activity to host Display related preferences.
 */
public class DisplaySettingsFragment extends ListItemSettingsFragment {

    /**
     * Gets a new instance of this class.
     */
    public static DisplaySettingsFragment newInstance() {
        DisplaySettingsFragment displaySettingsFragment = new DisplaySettingsFragment();
        Bundle bundle = ListItemSettingsFragment.getBundle();
        bundle.putInt(EXTRA_TITLE_ID, R.string.display_settings);
        bundle.putInt(EXTRA_ACTION_BAR_LAYOUT, R.layout.action_bar);
        displaySettingsFragment.setArguments(bundle);
        return displaySettingsFragment;
    }

    @Override
    public ListItemProvider getItemProvider() {
        return new ListProvider(getLineItems());
    }

    private List<ListItem> getLineItems() {
        List<ListItem> lineItems = new ArrayList<>();
        Context context = getContext();
        if (supportsAdaptiveBrightness()) {
            TextListItem adaptiveBrightnessItem = new TextListItem(context);
            adaptiveBrightnessItem.setTitle(context.getString(R.string.auto_brightness_title));
            adaptiveBrightnessItem.setBody(
                    context.getString(R.string.auto_brightness_summary));
            adaptiveBrightnessItem.setSwitch(
                    isAdaptiveBrightnessChecked(),
                    /* showDivider= */false,
                    (button, isChecked) ->
                            Settings.System.putInt(context.getContentResolver(),
                                    SCREEN_BRIGHTNESS_MODE,
                                    isChecked ? SCREEN_BRIGHTNESS_MODE_AUTOMATIC
                                            : SCREEN_BRIGHTNESS_MODE_MANUAL));
            lineItems.add(adaptiveBrightnessItem);
        }
        lineItems.add(new BrightnessLineItem(context));
        return lineItems;
    }

    private boolean isAdaptiveBrightnessChecked() {
        int brightnessMode = Settings.System.getInt(getContext().getContentResolver(),
                SCREEN_BRIGHTNESS_MODE, SCREEN_BRIGHTNESS_MODE_MANUAL);
        return brightnessMode != SCREEN_BRIGHTNESS_MODE_MANUAL;
    }

    private boolean supportsAdaptiveBrightness() {
        return getContext().getResources().getBoolean(
                com.android.internal.R.bool.config_automatic_brightness_available);
    }
}
