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
package com.android.compatibility.common.tradefed.testtype.retry;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.compatibility.common.tradefed.testtype.suite.CompatibilityTestSuite;
import com.android.compatibility.common.tradefed.util.VtsRetryFilterHelper;
import com.android.tradefed.config.OptionClass;

/**
 * Runner that creates a {@link CompatibilityTestSuite} to re-run VTS results.
 */
@OptionClass(alias = "compatibility")
public class VtsRetryFactoryTest extends RetryFactoryTest {
    /**
     * {@inheritDoc}
     */
    @Override
    protected VtsRetryFilterHelper createFilterHelper(CompatibilityBuildHelper buildHelper) {
        return new VtsRetryFilterHelper(buildHelper, getRetrySessionId(), mSubPlan, mIncludeFilters,
                mExcludeFilters, mAbiName, mModuleName, mTestName, mRetryType);
    }
}
