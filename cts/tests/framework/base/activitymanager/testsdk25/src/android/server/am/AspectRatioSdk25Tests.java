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
 * limitations under the License
 */

package android.server.am;

import static org.junit.Assert.fail;

import android.app.Activity;
import android.platform.test.annotations.Presubmit;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Build: mmma -j32 cts/tests/framework/base/activitymanager
 * Run: cts/tests/framework/base/activitymanager/util/run-test CtsActivityManagerDeviceSdk25TestCases android.server.am.AspectRatioSdk25Tests
 */
@RunWith(AndroidJUnit4.class)
@Presubmit
public class AspectRatioSdk25Tests extends AspectRatioTestsBase {

    // Max supported aspect ratio for pre-O apps.
    private static final float MAX_PRE_O_ASPECT_RATIO = 1.86f;

    // Test target activity that has targetSdk="25" and resizeableActivity="false".
    public static class Sdk25MaxAspectRatioActivity extends Activity {
    }

    @Rule
    public ActivityTestRule<Sdk25MaxAspectRatioActivity> mSdk25MaxAspectRatioActivity =
            new ActivityTestRule<>(Sdk25MaxAspectRatioActivity.class,
                    false /* initialTouchMode */, false /* launchActivity */);

    @Test
    public void testMaxAspectRatioPreOActivity() throws Exception {
        runAspectRatioTest(mSdk25MaxAspectRatioActivity, actual -> {
            if (aspectRatioLessThanEqual(actual, MAX_PRE_O_ASPECT_RATIO)) return;
            fail("actual=" + actual + " is greater than expected=" + MAX_PRE_O_ASPECT_RATIO);
        });
    }
}
