/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.systemintents.cts;

import static org.junit.Assert.assertTrue;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Configuration;
import android.net.Uri;
import android.provider.Settings;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.support.test.InstrumentationRegistry;
import android.util.Log;

import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class TestSystemIntents {

    /*
     * List of activity intents defined by the system.  Activities to handle each of these
     * intents must all exist.
     *
     * They are Intents here rather than simply action strings so that the test can
     * easily accommodate data URIs or similar for correct resolution.
     *
     * The flags associated with each intent indicate kinds of device on which the given
     * UI intent is *not* applicable.
     */

    private static final int EXCLUDE_TV = 1 << 0;
    private static final int EXCLUDE_WATCH = 1 << 1;
    private static final int EXCLUDE_NON_TELEPHONY = 1 << 2;
    private static final int EXCLUDE_NON_INSTALLABLE_IME = 1 << 3;

    class IntentEntry {
        public int flags;
        public Intent intent;

        public IntentEntry(int f, Intent i) {
            flags = f;
            intent = i;
        }
    }

    private final IntentEntry[] mTestIntents = {
            /* Settings-namespace intent actions */
            new IntentEntry(0, new Intent(Settings.ACTION_SETTINGS)),
            new IntentEntry(0, new Intent(Settings.ACTION_WEBVIEW_SETTINGS)),
            new IntentEntry(0, new Intent(Settings.ACTION_APPLICATION_DEVELOPMENT_SETTINGS)),
            new IntentEntry(0, new Intent(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS)),
            new IntentEntry(0, new Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS)
                    .setData(Uri.parse("package:android.systemintents.cts"))),
            new IntentEntry(0, new Intent(Settings.ACTION_IGNORE_BACKGROUND_DATA_RESTRICTIONS_SETTINGS)
                    .setData(Uri.parse("package:android.systemintents.cts"))),
            new IntentEntry(0, new Intent(Settings.ACTION_HOME_SETTINGS)),
            new IntentEntry(EXCLUDE_NON_TELEPHONY,
                    new Intent(Settings.ACTION_APN_SETTINGS)),
            new IntentEntry(EXCLUDE_TV|EXCLUDE_WATCH,
                    new Intent(Settings.ACTION_NOTIFICATION_POLICY_ACCESS_SETTINGS)),
            new IntentEntry(EXCLUDE_NON_INSTALLABLE_IME,
                    new Intent(Settings.ACTION_INPUT_METHOD_SETTINGS))
    };

    @Test
    public void testSystemIntents() {
        final PackageManager pm = InstrumentationRegistry.getContext().getPackageManager();
        int productFlags = 0;

        if (pm.hasSystemFeature(PackageManager.FEATURE_LEANBACK)) {
            productFlags |= EXCLUDE_TV;
        }

        if (!pm.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            productFlags |= EXCLUDE_NON_TELEPHONY;
        }

        if (!pm.hasSystemFeature(PackageManager.FEATURE_INPUT_METHODS)) {
            productFlags |= EXCLUDE_NON_INSTALLABLE_IME;
        }

        final Configuration config = InstrumentationRegistry.getContext().getResources().getConfiguration();
        if ((config.uiMode & Configuration.UI_MODE_TYPE_WATCH) != 0) {
            productFlags |= EXCLUDE_WATCH;
        }

        for (IntentEntry e : mTestIntents) {
            if ((productFlags & e.flags) == 0) {
                final ResolveInfo ri = pm.resolveActivity(e.intent, PackageManager.MATCH_DEFAULT_ONLY);
                assertTrue("API intent " + e.intent + " not implemented by any activity", ri != null);
            }
        }
    }
}
