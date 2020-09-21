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
 * limitations under the License
 */

package com.android.tv.settings.device.apps;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.os.Bundle;
import android.support.v7.preference.Preference;
import android.text.TextUtils;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settingslib.core.AbstractPreferenceController;
import com.android.tv.settings.PreferenceControllerFragment;
import com.android.tv.settings.R;

import java.util.ArrayList;
import java.util.List;

/**
 * Fragment for managing recent apps, and apps permissions.
 */
public class AppsFragment extends PreferenceControllerFragment {

    private static final String KEY_PERMISSIONS = "Permissions";

    public static void prepareArgs(Bundle b, String volumeUuid, String volumeName) {
        b.putString(AppsActivity.EXTRA_VOLUME_UUID, volumeUuid);
        b.putString(AppsActivity.EXTRA_VOLUME_NAME, volumeName);
    }

    public static AppsFragment newInstance(String volumeUuid, String volumeName) {
        final Bundle b = new Bundle(2);
        prepareArgs(b, volumeUuid, volumeName);
        final AppsFragment f = new AppsFragment();
        f.setArguments(b);
        return f;
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        final Preference permissionPreference = findPreference(KEY_PERMISSIONS);
        final String volumeUuid = getArguments().getString(AppsActivity.EXTRA_VOLUME_UUID);
        permissionPreference.setVisible(TextUtils.isEmpty(volumeUuid));
    }

    @Override
    protected int getPreferenceScreenResId() {
        return R.xml.apps;
    }

    @Override
    protected List<AbstractPreferenceController> onCreatePreferenceControllers(Context context) {
        final Activity activity = getActivity();
        final Application app = activity != null ? activity.getApplication() : null;
        List<AbstractPreferenceController> controllers = new ArrayList<>();
        controllers.add(new RecentAppsPreferenceController(getContext(), app));
        return controllers;
    }

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.SETTINGS_APP_NOTIF_CATEGORY;
    }
}
