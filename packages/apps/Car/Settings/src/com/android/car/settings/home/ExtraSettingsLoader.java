package com.android.car.settings.home;

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

import com.android.car.list.LaunchAppLineItem;
import com.android.car.list.TypedPagedListAdapter;
import com.android.car.settings.R;
import com.android.car.settings.common.Logger;

import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

/**
 * Loads Activity with TileUtils.EXTRA_SETTINGS_ACTION.
 * TODO: remove after all list new switched to androidx.car.widget
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

    public Map<String, Collection<TypedPagedListAdapter.LineItem>> load() {
        PackageManager pm = mContext.getPackageManager();
        Intent intent = new Intent(EXTRA_SETTINGS_ACTION);
        Map<String, Collection<TypedPagedListAdapter.LineItem>> extraSettings = new HashMap<>();
        // initialize the categories
        extraSettings.put(WIRELESS_CATEGORY, new LinkedList());
        extraSettings.put(DEVICE_CATEGORY, new LinkedList());
        extraSettings.put(SYSTEM_CATEGORY, new LinkedList());
        extraSettings.put(PERSONAL_CATEGORY, new LinkedList());

        List<ResolveInfo> results = pm.queryIntentActivitiesAsUser(intent,
                PackageManager.GET_META_DATA, ActivityManager.getCurrentUser());

        for (ResolveInfo resolved : results) {
            if (!resolved.system) {
                // Do not allow any app to add to settings, only system ones.
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
            if (TextUtils.isEmpty(title)) {
                LOG.d("no title.");
                title = activityInfo.loadLabel(pm).toString();
            }
            Integer iconRes = null;
            Icon icon = null;
            if (metaData.containsKey(META_DATA_PREFERENCE_ICON)) {
                iconRes = metaData.getInt(META_DATA_PREFERENCE_ICON);
                icon = Icon.createWithResource(activityInfo.packageName, iconRes);
            } else {
                icon = Icon.createWithResource(mContext, R.drawable.ic_settings_gear);
                LOG.d("no icon.");
            }
            Intent extraSettingIntent =
                    new Intent().setClassName(activityInfo.packageName, activityInfo.name);
            if (category == null || !extraSettings.containsKey(category)) {
                // If category is not specified or not supported, default to device.
                category = DEVICE_CATEGORY;
            }
            extraSettings.get(category).add(new LaunchAppLineItem(
                    title, icon, mContext, summary, extraSettingIntent));
        }
        return extraSettings;
    }
}
