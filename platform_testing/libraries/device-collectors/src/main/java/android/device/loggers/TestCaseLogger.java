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
package android.device.loggers;

import android.util.Log;

import org.junit.runner.Description;
import org.junit.runner.notification.RunListener;

/**
 * Logger for the device side that will mark the beginning and end of test cases in the logcat.
 */
public class TestCaseLogger extends RunListener {

    private static final String TAG = "TradefedDeviceEventsTag";

    @Override
    public void testStarted(Description description) throws Exception {
        String message =
                String.format(
                        "==================== %s#%s STARTED ====================",
                        description.getClassName(), description.getMethodName());
        Log.d(getTag(), message);
    }

    @Override
    public void testFinished(Description description) throws Exception {
        String message =
                String.format(
                        "==================== %s#%s ENDED ====================",
                        description.getClassName(), description.getMethodName());
        Log.d(getTag(), message);
    }

    /**
     * Returns the name of the current class to be used as a logging tag.
     */
    String getTag() {
        return TAG;
    }
}
