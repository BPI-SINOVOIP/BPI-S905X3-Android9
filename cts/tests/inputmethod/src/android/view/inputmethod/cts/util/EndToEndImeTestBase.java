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

package android.view.inputmethod.cts.util;

import static org.junit.Assume.assumeTrue;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.support.test.InstrumentationRegistry;

import org.junit.Before;

public class EndToEndImeTestBase {
    @Before
    public void showStateInitializeActivity() {
        // TODO(b/37502066): Move this back to @BeforeClass once b/37502066 is fixed.
        assumeTrue("MockIme cannot be used for devices that do not support installable IMEs",
                InstrumentationRegistry.getContext().getPackageManager().hasSystemFeature(
                        PackageManager.FEATURE_INPUT_METHODS));

        final Intent intent = new Intent()
                .setAction(Intent.ACTION_MAIN)
                .setClass(InstrumentationRegistry.getTargetContext(), StateInitializeActivity.class)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                .addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION)
                .addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
        InstrumentationRegistry.getInstrumentation().startActivitySync(intent);
    }
}
