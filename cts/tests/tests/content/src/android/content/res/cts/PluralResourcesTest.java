/*
 * Copyright (C) 2009 The Android Open Source Project
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

package android.content.res.cts;

import java.util.Locale;

import junit.framework.Assert;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Log;

import android.content.cts.R;

public class PluralResourcesTest extends AndroidTestCase {
    public static boolean DEBUG = false;
    private static final String TAG = "PluralResourcesTest";

    private Resources mResources;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mResources = mContext.getResources();
    }

    private Resources resourcesForLanguage(final String lang) {
        final Configuration config = new Configuration();
        config.updateFrom(mResources.getConfiguration());
        config.setLocale(new Locale(lang));
        return new Resources(mResources.getAssets(), mResources.getDisplayMetrics(), config);
    }

    @SmallTest
    public void testPlurals() {
        CharSequence cs;
        final Resources res = resourcesForLanguage("en");

        cs = res.getQuantityText(R.plurals.plurals_test, 0);
        if (DEBUG) {
            Log.d(TAG, "english 0 cs=" + cs);
        }
        Assert.assertEquals(cs.toString(), "Some dogs");

        cs = res.getQuantityText(R.plurals.plurals_test, 1);
        if (DEBUG) {
            Log.d(TAG, "english 1 cs=" + cs);
        }
        Assert.assertEquals(cs.toString(), "A dog");

        cs = res.getQuantityText(R.plurals.plurals_test, 2);
        Assert.assertEquals(cs.toString(), "Some dogs");

        cs = res.getQuantityText(R.plurals.plurals_test, 5);
        Assert.assertEquals(cs.toString(), "Some dogs");

        cs = res.getQuantityText(R.plurals.plurals_test, 500);
        Assert.assertEquals(cs.toString(), "Some dogs");
    }

    @SmallTest
    public void testCzech() {
        CharSequence cs;
        final Resources res = resourcesForLanguage("cs");

        cs = res.getQuantityText(R.plurals.plurals_test, 0);
        if (DEBUG) {
            Log.d(TAG, "czech 0 cs=" + cs);
        }
        Assert.assertEquals(cs.toString(), "Some Czech dogs");

        cs = res.getQuantityText(R.plurals.plurals_test, 1);
        if (DEBUG) {
            Log.d(TAG, "czech 1 cs=" + cs);
        }
        Assert.assertEquals(cs.toString(), "A Czech dog");

        cs = res.getQuantityText(R.plurals.plurals_test, 2);
        if (DEBUG) {
            Log.d(TAG, "czech 2 cs=" + cs);
        }
        Assert.assertEquals(cs.toString(), "Few Czech dogs");

        cs = res.getQuantityText(R.plurals.plurals_test, 5);
        Assert.assertEquals(cs.toString(), "Some Czech dogs");

        cs = res.getQuantityText(R.plurals.plurals_test, 500);
        Assert.assertEquals(cs.toString(), "Some Czech dogs");
    }
}
