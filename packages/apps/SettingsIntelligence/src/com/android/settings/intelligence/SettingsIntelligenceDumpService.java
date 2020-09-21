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

package com.android.settings.intelligence;

import android.app.Service;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.IBinder;
import android.support.annotation.VisibleForTesting;

import com.android.settings.intelligence.suggestions.model.SuggestionCategory;
import com.android.settings.intelligence.suggestions.model.SuggestionCategoryRegistry;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.List;

/**
 * A service that reacts to adb shell dumpsys.
 * <p/>
 * adb shell am startservice com.android.settings.intelligence/\
 *      .SettingsIntelligenceDumpService
 * adb shell dumpsys activity service com.android.settings.intelligence/\
 *      .SettingsIntelligenceDumpService
 */
public class SettingsIntelligenceDumpService extends Service {

    private static final String KEY_SUGGESTION_CATEGORY = "suggestion_category: ";

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    protected void dump(FileDescriptor fd, PrintWriter writer, String[] args) {
        final StringBuilder dump = new StringBuilder();
        dump.append(getString(R.string.app_name_settings_intelligence))
                .append('\n')
                .append(dumpSuggestions());
        writer.println(dump.toString());
    }

    @VisibleForTesting
    String dumpSuggestions() {
        final StringBuilder dump = new StringBuilder();
        final PackageManager pm = getPackageManager();
        dump.append(" suggestion dump\n");
        for (SuggestionCategory category : SuggestionCategoryRegistry.CATEGORIES) {
            dump.append(KEY_SUGGESTION_CATEGORY)
                    .append(category.getCategory())
                    .append('\n');

            final Intent probe = new Intent(Intent.ACTION_MAIN);
            probe.addCategory(category.getCategory());
            final List<ResolveInfo> results = pm
                    .queryIntentActivities(probe, PackageManager.GET_META_DATA);
            if (results == null || results.isEmpty()) {
                continue;
            }
            for (ResolveInfo info : results) {
                dump.append("\t\t")
                        .append(info.activityInfo.packageName)
                        .append('/')
                        .append(info.activityInfo.name)
                        .append('\n');
            }
        }
        return dump.toString();
    }
}
