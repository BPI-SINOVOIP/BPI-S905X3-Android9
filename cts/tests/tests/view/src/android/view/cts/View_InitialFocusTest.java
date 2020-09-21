/*
 * Copyright 2017 The Android Open Source Project
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

package android.view.cts;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.pm.PackageManager;
import android.support.test.InstrumentationRegistry;
import android.support.test.annotation.UiThreadTest;
import android.support.test.filters.MediumTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;

import org.junit.Assume;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class View_InitialFocusTest {
    @Rule
    public ActivityTestRule<FocusHandlingCtsActivity> mActivityRule =
            new ActivityTestRule<>(FocusHandlingCtsActivity.class, true);

    @Before
    public void setup() throws Exception {
        Activity activity = mActivityRule.getActivity();
        Assume.assumeTrue(
                activity.getPackageManager().hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN));
        Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        instrumentation.setInTouchMode(true);
    }

    private View[] getInitialAndFirstFocus(int res) throws Throwable {
        Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        final Activity activity = mActivityRule.getActivity();
        mActivityRule.runOnUiThread(() -> activity.getLayoutInflater().inflate(res,
                (ViewGroup) activity.findViewById(R.id.auto_test_area)));
        instrumentation.waitForIdleSync();
        View root = activity.findViewById(R.id.main_view);
        View initial = root.findFocus();
        instrumentation.sendKeyDownUpSync(KeyEvent.KEYCODE_TAB); // leaves touch-mode
        View first = root.findFocus();
        return new View[]{initial, first};
    }

    @Test
    public void testNoInitialFocus() throws Throwable {
        Activity activity = mActivityRule.getActivity();
        View[] result = getInitialAndFirstFocus(R.layout.focus_handling_focusables);
        assertNull(result[0]);
        assertSame(result[1], activity.findViewById(R.id.focusable1));
    }

    @Test
    public void testDefaultFocus() throws Throwable {
        Activity activity = mActivityRule.getActivity();
        View[] result = getInitialAndFirstFocus(R.layout.focus_handling_default_focus);
        assertNull(result[0]);
        assertSame(result[1], activity.findViewById(R.id.focusable2));
    }

    @Test
    public void testInitialFocus() throws Throwable {
        Activity activity = mActivityRule.getActivity();
        View[] result = getInitialAndFirstFocus(R.layout.focus_handling_initial_focus);
        assertSame(result[0], activity.findViewById(R.id.focusable3));
    }

    @UiThreadTest
    @Test
    public void testClearFocus() throws Throwable {
        Activity activity = mActivityRule.getActivity();
        View v1 = activity.findViewById(R.id.view1);
        v1.setFocusableInTouchMode(true);
        assertFalse(v1.isFocused());
        v1.requestFocus();
        assertTrue(v1.isFocused());
        v1.clearFocus();
        assertNull(activity.getWindow().getDecorView().findFocus());
    }
}
