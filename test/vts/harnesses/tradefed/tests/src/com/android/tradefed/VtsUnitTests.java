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
package com.android.tradefed;

import com.android.tradefed.device.metric.VtsCoverageCollectorTest;
import com.android.tradefed.device.metric.VtsHalTraceCollectorTest;
import com.android.tradefed.presubmit.VtsConfigLoadingTest;
import com.android.tradefed.targetprep.VtsCoveragePreparerTest;
import com.android.tradefed.targetprep.VtsHalAdapterPreparerTest;
import com.android.tradefed.targetprep.VtsPythonVirtualenvPreparerTest;
import com.android.tradefed.targetprep.VtsTraceCollectPreparerTest;
import com.android.tradefed.testtype.VtsFuzzTestResultParserTest;
import com.android.tradefed.testtype.VtsFuzzTestTest;
import com.android.tradefed.testtype.VtsMultiDeviceTestResultParserTest;
import com.android.tradefed.testtype.VtsMultiDeviceTestTest;
import com.android.tradefed.util.CmdUtilTest;
import com.android.tradefed.util.ProcessHelperTest;
import com.android.tradefed.util.VtsPythonRunnerHelperTest;

import org.junit.runner.RunWith;
import org.junit.runners.Suite;
import org.junit.runners.Suite.SuiteClasses;

/**
 * A test suite for all VTS Tradefed unit tests.
 *
 * <p>All tests listed here should be self-contained, and do not require any external dependencies.
 */
@RunWith(Suite.class)
@SuiteClasses({
    // NOTE: please keep classes sorted lexicographically in each group
    //device
    VtsCoverageCollectorTest.class,
    VtsHalTraceCollectorTest.class,

    // presubmit
    VtsConfigLoadingTest.class,
    // targetprep
    VtsCoveragePreparerTest.class,
    VtsHalAdapterPreparerTest.class,
    VtsPythonVirtualenvPreparerTest.class,
    VtsTraceCollectPreparerTest.class,

    // testtype
    VtsFuzzTestResultParserTest.class,
    VtsFuzzTestTest.class,
    VtsMultiDeviceTestResultParserTest.class,
    VtsMultiDeviceTestTest.class,

    // util
    CmdUtilTest.class,
    ProcessHelperTest.class,
    VtsPythonRunnerHelperTest.class,
})
public class VtsUnitTests {
    // empty on purpose
}
