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
package com.android.tradefed.prodtests;

import com.android.build.tests.ImageStatsTest;
import com.android.continuous.SmokeTestFailureReporterTest;
import com.android.continuous.SmokeTestTest;
import com.android.monkey.MonkeyBaseTest;

import org.junit.runner.RunWith;
import org.junit.runners.Suite;
import org.junit.runners.Suite.SuiteClasses;

/**
 * A test suite for all Trade Federation unit tests running under Junit4.
 *
 * <p>All tests listed here should be self-contained, and should not require any external
 * dependencies.
 */
@RunWith(Suite.class)
@SuiteClasses({
    // build.tests
    ImageStatsTest.class,

    // Continuous
    SmokeTestFailureReporterTest.class,
    SmokeTestTest.class,

    // monkey
    MonkeyBaseTest.class
})
public class UnitTests {
    // empty of purpose
}
