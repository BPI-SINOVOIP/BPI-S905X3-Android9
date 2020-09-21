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

package android.server.am;

import static android.server.am.UiDeviceUtils.pressBackButton;
import static android.server.am.deprecatedsdk.Components.MAIN_ACTIVITY;

import android.support.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Ensure that compatibility dialog is shown when launching an application
 * targeting a deprecated version of SDK.
 * <p>Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:DeprecatedTargetSdkTest
 */
@RunWith(AndroidJUnit4.class)
public class DeprecatedTargetSdkTest extends ActivityManagerTestBase {

    /** @see com.android.server.am.DeprecatedTargetSdkVersionDialog */
    private static final String DEPRECATED_TARGET_SDK_VERSION_DIALOG =
            "DeprecatedTargetSdkVersionDialog";

    @After
    @Override
    public void tearDown() throws Exception {
        super.tearDown();

        // Ensure app process is stopped.
        stopTestPackage(MAIN_ACTIVITY);
    }

    @Test
    public void testCompatibilityDialog() throws Exception {
        // Launch target app.
        launchActivity(MAIN_ACTIVITY);
        mAmWmState.assertActivityDisplayed(MAIN_ACTIVITY);
        mAmWmState.assertWindowDisplayed(DEPRECATED_TARGET_SDK_VERSION_DIALOG);

        // Go back to dismiss the warning dialog.
        pressBackButton();

        // Go back again to formally stop the app. If we just kill the process, it'll attempt to
        // resume rather than starting from scratch (as far as ActivityStack is concerned) and it
        // won't invoke the warning dialog.
        pressBackButton();
    }
}
