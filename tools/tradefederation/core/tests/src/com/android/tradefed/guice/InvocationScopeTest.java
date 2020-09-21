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
package com.android.tradefed.guice;

import static org.junit.Assert.*;

import com.android.tradefed.config.Configuration;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.targetprep.multi.StubMultiTargetPreparer;
import com.android.tradefed.testtype.IRemoteTest;

import com.google.inject.Inject;
import com.google.inject.Injector;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Arrays;

/** Unit tests for {@link InvocationScope}. */
@RunWith(JUnit4.class)
public class InvocationScopeTest {

    /** Test class to check that object are properly seeded. */
    public static class InjectedClass implements IRemoteTest {

        public Injector mInjector;
        public IConfiguration mConfiguration;

        @Inject
        public void setInjector(Injector injector) {
            mInjector = injector;
        }

        @Inject
        public void setConfiguration(IConfiguration configuration) {
            mConfiguration = configuration;
        }

        @Override
        public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
            // Do nothing
        }
    }

    private IConfiguration mConfiguration;

    @Before
    public void setUp() {
        mConfiguration = new Configuration("test", "test");
    }

    /** Test that the injection and seed object are available in the scope. */
    @Test
    public void testInjection() {
        InjectedClass test = new InjectedClass();
        mConfiguration.setTest(test);
        mConfiguration.setMultiTargetPreparers(Arrays.asList(new StubMultiTargetPreparer()));
        InvocationScope scope = new InvocationScope();
        scope.enter();
        try {
            scope.seedConfiguration(mConfiguration);
            assertNotNull(test.mInjector);
            assertNotNull(test.mConfiguration);
            assertEquals(mConfiguration, test.mConfiguration);
        } finally {
            scope.exit();
        }
    }
}
