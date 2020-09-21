/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.continuous;

import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.result.BugreportCollector;
import com.android.tradefed.result.DeviceFileReporter;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.NameMangleListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.InstrumentationTest;

import java.util.LinkedHashMap;
import java.util.Map;

/**
 * A test that runs the smoke tests
 * <p />
 * Simply {@link InstrumentationTest} with extra reporting.  Should be re-integrated with
 * {@link InstrumentationTest} after it's clear that it works as expected.
 */
@OptionClass(alias = "smoke")
public class SmokeTest extends InstrumentationTest {
    @Option(name = "capture-file-pattern", description = "File glob of on-device files to log " +
            "if found.  Takes two arguments: the glob, and the file type " +
            "(text/xml/zip/gzip/png/unknown).  May be repeated.", importance = Importance.IF_UNSET)
    private Map<String, LogDataType> mUploadFilePatterns = new LinkedHashMap<String, LogDataType>();

    @Option(name = "bugreport-device-wait-time", description = "How many seconds to wait for the " +
            "device to become available so we can capture a bugreport.  Useful in case the smoke " +
            "tests fail due to a device reboot.")
    private int mDeviceWaitTime = 60;

    /**
     * Simple constructor that disables an incompatible runmode from the superclass
     */
    public SmokeTest() {
        super();
        // Re-run mode doesn't work properly with Smoke
        setRerunMode(false);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(final ITestInvocationListener listener) throws DeviceNotAvailableException {
        // trimListener should be the first thing to receive any results.  It will pass the results
        // through to the bugListener, which will forward them (after collecting any necessary
        // bugreports) to the real Listener(s).
        final BugreportCollector bugListener = new BugreportCollector(listener, getDevice());
        bugListener.setDeviceWaitTime(mDeviceWaitTime);
        bugListener.addPredicate(BugreportCollector.AFTER_FAILED_TESTCASES);
        final ITestInvocationListener trimListener = new TrimListener(bugListener);

        super.run(trimListener);

        final DeviceFileReporter dfr = new DeviceFileReporter(getDevice(), trimListener);
        dfr.addPatterns(mUploadFilePatterns);
        dfr.run();
    }

    /**
     * A class to adjust the test identifiers from something like this:
     * com.android.smoketest.SmokeTestRunner$3#com.android.voicedialer.VoiceDialerActivity
     * To this:
     * SmokeFAST#com.android.voicedialer.VoiceDialerActivity
     */
    static class TrimListener extends NameMangleListener {
        public TrimListener(ITestInvocationListener listener) {
            super(listener);
        }

        /** {@inheritDoc} */
        @Override
        protected TestDescription mangleTestId(TestDescription test) {
            final String method = test.getTestName();
            final String klass = test.getClassName().replaceFirst(
                    "com.android.smoketest.SmokeTestRunner(?:\\$\\d+)?", "SmokeFAST");
            return new TestDescription(klass, method);
        }
    }
}

