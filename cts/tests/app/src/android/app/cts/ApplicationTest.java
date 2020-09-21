/*
 * Copyright (C) 2008 The Android Open Source Project
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

package android.app.cts;

import android.app.Activity;
import android.app.Application;
import android.app.Instrumentation;
import android.app.stubs.MockApplication;
import android.app.stubs.MockApplicationActivity;
import android.content.Context;
import android.content.Intent;
import android.test.InstrumentationTestCase;

import com.android.compatibility.common.util.SystemUtil;

/**
 * Test {@link Application}.
 */
public class ApplicationTest extends InstrumentationTestCase {
    private static final String ERASE_FONT_SCALE_CMD = "settings delete system font_scale";
    // 2 is an arbitrary value.
    private static final String PUT_FONT_SCALE_CMD = "settings put system font_scale 2";

    @Override
    public void tearDown() throws Exception {
        SystemUtil.runShellCommand(getInstrumentation(), ERASE_FONT_SCALE_CMD);
    }


    public void testApplication() throws Throwable {
        final Instrumentation instrumentation = getInstrumentation();
        final Context targetContext = instrumentation.getTargetContext();

        final Intent intent = new Intent(targetContext, MockApplicationActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        final Activity activity = instrumentation.startActivitySync(intent);
        final MockApplication mockApp = (MockApplication) activity.getApplication();
        assertTrue(mockApp.isConstructorCalled);
        assertTrue(mockApp.isOnCreateCalled);
        toggleFontScale();
        assertTrue(waitForOnConfigurationChange(mockApp));
    }

    // Font scale is a global configuration.
    // This function will delete any previous font scale changes, apply one, and remove it.
    private void toggleFontScale() throws Throwable {
        SystemUtil.runShellCommand(getInstrumentation(), ERASE_FONT_SCALE_CMD);
        getInstrumentation().waitForIdleSync();
        SystemUtil.runShellCommand(getInstrumentation(), PUT_FONT_SCALE_CMD);
        getInstrumentation().waitForIdleSync();
        SystemUtil.runShellCommand(getInstrumentation(), ERASE_FONT_SCALE_CMD);
    }

    // Wait for a maximum of 5 seconds for global config change to occur.
    private boolean waitForOnConfigurationChange(MockApplication mockApp) throws Throwable {
        int retriesLeft = 5;
        while(retriesLeft-- > 0) {
            if (mockApp.isOnConfigurationChangedCalled) {
                return true;
            }
            Thread.sleep(1000);
        }
        return false;
    }

}
