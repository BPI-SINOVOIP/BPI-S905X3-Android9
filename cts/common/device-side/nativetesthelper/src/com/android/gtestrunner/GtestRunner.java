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

package com.android.gtestrunner;

import org.junit.runner.Description;
import org.junit.runner.Runner;
import org.junit.runner.notification.RunNotifier;

public class GtestRunner extends Runner {
    private static boolean sOnceFlag = false;

    private Class mTargetClass;
    private Description mDescription;

    public GtestRunner(Class testClass) {
        synchronized (GtestRunner.class) {
            if (sOnceFlag) {
                throw new IllegalStateException("Error multiple GtestRunners defined");
            }
            sOnceFlag = true;
        }

        mTargetClass = testClass;
        TargetLibrary library = (TargetLibrary) testClass.getAnnotation(TargetLibrary.class);
        if (library == null) {
            throw new IllegalStateException("Missing required @TargetLibrary annotation");
        }
        System.loadLibrary(library.value());
        mDescription = Description.createSuiteDescription(testClass);
        nInitialize(mDescription);
    }

    @Override
    public Description getDescription() {
        return mDescription;
    }

    @Override
    public void run(RunNotifier notifier) {
        nRun(notifier);
    }

    private static native void nInitialize(Description description);
    private static native void nRun(RunNotifier notifier);
}
