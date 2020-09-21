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

package android.systemui.cts;

import static android.support.test.InstrumentationRegistry.getInstrumentation;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeFalse;

import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.support.test.uiautomator.UiDevice;
import android.view.View;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests for light system bars that set the flag via theme.
 *
 * atest CtsSystemUiTestCases:LightBarThemeTest
 */
@RunWith(AndroidJUnit4.class)
public class LightBarThemeTest extends LightBarTestBase {

    private UiDevice mDevice;

    @Rule
    public ActivityTestRule<LightBarThemeActivity> mActivityRule = new ActivityTestRule<>(
            LightBarThemeActivity.class);

    @Before
    public void setUp() {
        mDevice = UiDevice.getInstance(getInstrumentation());
    }

    @Test
    public void testThemeSetsFlags() throws Exception {
        final int visibility = mActivityRule.getActivity().getSystemUiVisibility();
        assertTrue((visibility & View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR) != 0);
        assertTrue((visibility & View.SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR) != 0);
    }

    @Test
    public void testGetNavigationBarDividerColor() throws Exception {
        assumeHasColorNavigationBar();

        assertEquals(getInstrumentation().getContext().getColor(R.color.navigationBarDividerColor),
                mActivityRule.getActivity().getWindow().getNavigationBarDividerColor());
    }

    @Test
    public void testNavigationBarDividerColor() throws Exception {
        assumeHasColorNavigationBar();

        // Wait until the activity is fully visible
        mDevice.waitForIdle();

        checkNavigationBarDivider(mActivityRule.getActivity(),
                getInstrumentation().getContext().getColor(R.color.navigationBarDividerColor));
    }
}
