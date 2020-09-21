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
package com.android.tradefed.result.ddmlib;

import com.android.ddmlib.IShellEnabledDevice;
import com.android.ddmlib.testrunner.ITestRunListener;
import com.android.ddmlib.testrunner.InstrumentationResultParser;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;

import java.util.Collection;

/**
 * Extension of the ddmlib {@link RemoteAndroidTestRunner} to set some default for Tradefed use
 * cases.
 */
public class DefaultRemoteAndroidTestRunner extends RemoteAndroidTestRunner {

    public DefaultRemoteAndroidTestRunner(
            String packageName, String runnerName, IShellEnabledDevice remoteDevice) {
        super(packageName, runnerName, remoteDevice);
    }

    /** {@inheritDoc} */
    @Override
    public InstrumentationResultParser createParser(
            String runName, Collection<ITestRunListener> listeners) {
        InstrumentationResultParser parser = super.createParser(runName, listeners);
        // Keep the formatting, in order to preserve the stack format.
        parser.setTrimLine(false);
        return parser;
    }
}
