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

package com.android.car.settings.common;

import static com.android.settingslib.drawer.TileUtils.EXTRA_SETTINGS_ACTION;
import static com.android.settingslib.drawer.TileUtils.META_DATA_PREFERENCE_ICON;
import static com.android.settingslib.drawer.TileUtils.META_DATA_PREFERENCE_SUMMARY;
import static com.android.settingslib.drawer.TileUtils.META_DATA_PREFERENCE_TITLE;

import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.graphics.drawable.Icon;
import android.os.Bundle;
import android.text.TextUtils;

import androidx.car.widget.ListItem;
import androidx.car.widget.TextListItem;

import com.android.car.settings.R;

import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

/**
 * Loads Activity with TileUtils.EXTRA_SETTINGS_ACTION.
 */
public class ExtraSettingsLoader {
    private static final Logger LOG = new Logger(ExtraSettingsLoader.class);
    private static final String META_DATA_PREFERENCE_CATEGORY = "com.android.settings.category";
    public static final String WIRELESS_CATEGORY = "com.android.settings.category.wireless";
    public static final String DEVICE_CATEGORY = "com.android.settings.category.device";
    public static final String SYSTEM_CATEGORY = "com.android.settings.category.system";
    public static final String PERSONAL_CATEGORY = "com.android.settings.category.personal";
    private final Context mContext;

    public ExtraSettingsLoader(Context context) {
        mContext = context;
    }

    /**
     * Returns a map of category and setting items pair loaded from 3rd party.
     */
    public Map<String, Collection<ListItem>> load() {
        PackageManager pm = mContext.getPackageManager();
        Intent intent = new Intent(EXTRA_SETTINGS_ACTION);
        Map<String, Collection<ListItem>> extraSettings = new HashMap<>();
        // initialize the categories
        extraSettings.put(WIRELESS_CATEGORY, new LinkedList());
        extraSettings.put(DEVICE_CATEGORY, new LinkedList());
        extraSettings.put(SYSTEM_CATEGORY, new LinkedList());
        extraSettings.put(PERSONAL_CATEGORY, new LinkedList());

        List<ResolveInfo> results = pm.queryIntentActivitiesAsUser(intent,
                PackageManager.GET_META_DATA, ActivityManager.getCurrentUser());

        for (ResolveInfo resolved : results) {
            if (!resolved.system) {
                // Do not allow any app to be added to settings, only system ones.
                continue;
            }
            String title = null;
            String summary = null;
            String category = null;
            ActivityInfo activityInfo = resolved.activityInfo;
            Bundle metaData = activityInfo.metaData;
            try {
                Resources res = pm.getResourcesForApplication(activityInfo.packageName);
                if (metaData.containsKey(META_DATA_PREFERENCE_TITLE)) {
                    if (metaData.get(META_DATA_PREFERENCE_TITLE) instanceof Integer) {
                        title = res.getString(metaData.getInt(META_DATA_PREFERENCE_TITLE));
                    } else {
                        title = metaData.getString(META_DATA_PREFERENCE_TITLE);
                    }
                }
                if (TextUtils.isEmpty(title)) {
                    LOG.d("no title.");
                    title = activityInfo.loadLabel(pm).toString();
                }
                if (metaData.containsKey(META_DATA_PREFERENCE_SUMMARY)) {
                    if (metaData.get(META_DATA_PREFERENCE_SUMMARY) instanceof Integer) {
                        summary = res.getString(metaData.getInt(META_DATA_PREFERENCE_SUMMARY));
                    } else {
                        summary = metaData.getString(META_DATA_PREFERENCE_SUMMARY);
                    }
                } else {
                    LOG.d("no description.");
                }
                if (metaData.containsKey(META_DATA_PREFERENCE_CATEGORY)) {
                    if (metaData.get(META_DATA_PREFERENCE_CATEGORY) instanceof Integer) {
                        category = res.getString(metaData.getInt(META_DATA_PREFERENCE_CATEGORY));
                    } else {
                        category = metaData.getString(META_DATA_PREFERENCE_CATEGORY);
                    }
                } else {
                    LOG.d("no category.");
                }
            } catch (PackageManager.NameNotFoundException | Resources.NotFoundException e) {
                LOG.d("Couldn't find info", e);
            }
            Icon icon = null;
            if (metaData.containsKey(META_DATA_PREFERENCE_ICON)) {
                int iconRes = metaData.getInt(META_DATA_PREFERENCE_ICON);
                icon = Icon.createWithResource(activityInfo.packageName, iconRes);
            } else {
                icon = Icon.createWithResource(mContext, R.drawable.ic_settings_gear);
                LOG.d("use default icon.");
            }
            Intent extraSettingIntent =
                    new Intent().setClassName(activityInfo.packageName, activityInfo.name);
            if (category == null || !extraSettings.containsKey(category)) {
                // If category is not specified or not supported, default to device.
                category = DEVICE_CATEGORY;
            }
            TextListItem item = new TextListItem(mContext);
            item.setTitle(title);
            item.setBody(summary);
            item.setPrimaryActionIcon(icon.loadDrawable(mContext), /* useLargeIcon= */ false);
            item.setSupplementalIcon(R.drawable.ic_chevron_right, /* showDivider= */ false);
            item.setOnClickListener(v -> mContext.startActivity(extraSettingIntent));
            extraSettings.get(category).add(item);
        }
        return extraSettings;
    }
}
