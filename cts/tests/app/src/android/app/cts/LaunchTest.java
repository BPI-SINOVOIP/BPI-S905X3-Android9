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

import android.app.stubs.ActivityTestsBase;
import android.app.stubs.ClearTop;
import android.app.stubs.LaunchpadActivity;
import android.app.stubs.LocalActivity;
import android.app.stubs.LocalScreen;
import android.app.stubs.TestedActivity;
import android.app.stubs.TestedScreen;
import android.content.ComponentName;

public class LaunchTest extends ActivityTestsBase {

    public void testClearTopWhilResumed() {
        mIntent.putExtra("component", new ComponentName(getContext(), ClearTop.class));
        mIntent.putExtra(ClearTop.WAIT_CLEAR_TASK, true);
        runLaunchpad(LaunchpadActivity.LAUNCH);
    }

    public void testClearTopInCreate() throws Exception {
        mIntent.putExtra("component", new ComponentName(getContext(), ClearTop.class));
        runLaunchpad(LaunchpadActivity.LAUNCH);
    }

    public void testForwardResult() {
        runLaunchpad(LaunchpadActivity.FORWARD_RESULT);
    }

    public void testLocalScreen() {
        mIntent.putExtra("component", new ComponentName(getContext(), LocalScreen.class));
        runLaunchpad(LaunchpadActivity.LAUNCH);
    }

    public void testColdScreen() {
        mIntent.putExtra("component", new ComponentName(getContext(), TestedScreen.class));
        runLaunchpad(LaunchpadActivity.LAUNCH);
    }

    public void testLocalActivity() {
        mIntent.putExtra("component", new ComponentName(getContext(), LocalActivity.class));
        runLaunchpad(LaunchpadActivity.LAUNCH);
    }

    public void testColdActivity() {
        mIntent.putExtra("component", new ComponentName(getContext(), TestedActivity.class));
        runLaunchpad(LaunchpadActivity.LAUNCH);
    }
}
