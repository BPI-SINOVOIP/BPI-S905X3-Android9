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

package android.fragment.cts.sdk26;

import static org.junit.Assert.assertTrue;

import android.os.Debug;
import android.support.test.filters.MediumTest;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class FragmentManagerNonConfigTest {

    @Rule
    public ActivityTestRule<NonConfigOnStopActivity> mActivityRule =
            new ActivityTestRule<>(NonConfigOnStopActivity.class);

    /**
     * When a fragment is added during onStop(), it shouldn't show up in non-config
     * state when restored for apps targeting SDKs before P.
     */
    @Test
    public void nonConfigStop() throws Throwable {
        // Trigger activity relaunch.
        final NonConfigOnStopActivity activity = mActivityRule.getActivity();
        activity.sDestroyed = new CountDownLatch(1);
        activity.sResumed = new CountDownLatch(1);
        mActivityRule.runOnUiThread(() -> activity.recreate());

        // Wait for activity to be relaunched.
        assertTrue(activity.sDestroyed.await(1, TimeUnit.SECONDS));
        assertTrue(activity.sResumed.await(1, TimeUnit.SECONDS));

        // A fragment was added in onStop(), but we shouldn't see it here...
        final NonConfigOnStopActivity recreatedActivity = NonConfigOnStopActivity.sActivity;
        assertTrue(recreatedActivity.getFragmentManager().getFragments().isEmpty());
    }
}