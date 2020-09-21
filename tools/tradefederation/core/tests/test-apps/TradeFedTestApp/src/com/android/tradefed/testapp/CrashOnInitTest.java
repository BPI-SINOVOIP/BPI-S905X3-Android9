/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.tradefed.testapp;

import junit.framework.TestCase;

/**
 * A {@link TestCase} that fails initialization.
 * <p/>
 * Used to test exception scenarios in tradefed.InstrumentationTest collect tests step
 */
public class CrashOnInitTest extends TestCase {

    public CrashOnInitTest(String name) {
        throw new RuntimeException();
    }

    public CrashOnInitTest() {
        throw new RuntimeException();
    }

    public void testNeverRun() {

    }
}
