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
package com.android.compatibility.common.util;

import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

/**
 * Custom JUnit4 rule that provides "before" / "after" callbacks, which is useful to use with
 * {@link org.junit.rules.RuleChain}.
 */
public class BeforeAfterRule implements TestRule {
    @Override
    public Statement apply(Statement base, Description description) {
        return new Statement() {

            @Override
            public void evaluate() throws Throwable {
                onBefore(base, description);
                try {
                    base.evaluate();
                } finally {
                    onAfter(base, description);
                }
            }
        };
    }

    protected void onBefore(Statement base, Description description) throws Throwable {
    }

    protected void onAfter(Statement base, Description description) throws Throwable {
    }
}
