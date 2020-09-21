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

package android.app.cts;

import static android.content.Context.ACTIVITY_SERVICE;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.fail;

import android.app.ActivityManager;
import android.app.ActivityManager.RecentTaskInfo;
import android.app.ActivityManager.TaskDescription;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.platform.test.annotations.Presubmit;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import android.app.Activity;
import android.app.stubs.MockActivity;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;

/**
 * Build: mmma -j32 cts/tests/app
 * Run: cts/tests/framework/base/activitymanager/util/run-test CtsAppTestCases android.app.cts.TaskDescriptionTest
 */
@RunWith(AndroidJUnit4.class)
@Presubmit
public class TaskDescriptionTest {
    private static final String TEST_LABEL = "test-label";
    private static final int TEST_NO_DATA = 0;
    private static final int TEST_RES_DATA = 777;
    private static final int TEST_COLOR = Color.BLACK;

    @Rule
    public ActivityTestRule<MockActivity> mTaskDescriptionActivity =
        new ActivityTestRule<>(MockActivity.class,
            false /* initialTouchMode */, false /* launchActivity */);

    @Test
    public void testBitmapConstructor() throws Exception {
        final Activity activity = mTaskDescriptionActivity.launchActivity(null);
        final Bitmap bitmap = Bitmap.createBitmap(1, 1, Bitmap.Config.ARGB_8888);
        bitmap.eraseColor(0);
        activity.setTaskDescription(new TaskDescription(TEST_LABEL, bitmap, TEST_COLOR));
        assertTaskDescription(activity, TEST_LABEL, TEST_NO_DATA);
    }

    @Test
    public void testResourceConstructor() throws Exception {
        final Activity activity = mTaskDescriptionActivity.launchActivity(null);
        activity.setTaskDescription(new TaskDescription(TEST_LABEL, TEST_RES_DATA, TEST_COLOR));
        assertTaskDescription(activity, TEST_LABEL, TEST_RES_DATA);
    }

    private void assertTaskDescription(Activity activity, String label, int resId) {
        final ActivityManager am = (ActivityManager) activity.getSystemService(ACTIVITY_SERVICE);
        List<RecentTaskInfo> recentsTasks = am.getRecentTasks(1 /* maxNum */, 0 /* flags */);
        if (!recentsTasks.isEmpty()) {
            final RecentTaskInfo info = recentsTasks.get(0);
            if (activity.getTaskId() == info.id) {
                final TaskDescription td = info.taskDescription;
                assertNotNull(td);
                if (resId == TEST_NO_DATA) {
                    assertNotNull(td.getIcon());
                    assertNotNull(td.getIconFilename());
                } else {
                    assertNull(td.getIconFilename());
                    assertNull(td.getIcon());
                }
                assertEquals(resId, td.getIconResource());
                assertEquals(label, td.getLabel());
                return;
            }
        }
        fail("Did not find activity (id=" + activity.getTaskId() + ") in recent tasks list");
    }
}
