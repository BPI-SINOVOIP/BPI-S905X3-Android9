/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.tradefed.util;

import org.junit.runner.Description;
import org.junit.runner.manipulation.Filter;

/**
 * Helper Class that provides the filtering for JUnit4 runner by extending the {@link Filter}.
 */
public class JUnit4TestFilter extends Filter {

    private TestFilterHelper mFilterHelper;

    public JUnit4TestFilter(TestFilterHelper filterHelper) {
        mFilterHelper = filterHelper;
    }

    /**
     * Filter function deciding whether a test should run or not.
     */
    @Override
    public boolean shouldRun(Description description) {
        // If it's a suite, we check if at least one of the children can run, if yes then we run it
        if (description.isSuite()) {
            for (Description desc : description.getChildren()) {
                if (shouldRun(desc)) {
                    return true;
                }
            }
        }
        return mFilterHelper.shouldRun(description);
    }

    @Override
    public String describe() {
        return "This filter is based on annotations, regex filter, class and method name to decide "
                + "if a particular test should run.";
    }
}
