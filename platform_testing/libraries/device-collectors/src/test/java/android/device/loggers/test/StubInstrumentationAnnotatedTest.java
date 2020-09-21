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
package android.device.loggers.test;

import android.device.loggers.TestLogData;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;

/**
 * Android Instrumentation annotated to test for logged files from host-side. This is meant to run
 * from the host-side to exercise the loggers.
 */
@RunWith(AndroidJUnit4.class)
public class StubInstrumentationAnnotatedTest {

    @Rule public TestLogData logs = new TestLogData();

    @Test
    public void testOne() {
        logs.addTestLog("fake_file", new File("/fake/path"));
    }
}
