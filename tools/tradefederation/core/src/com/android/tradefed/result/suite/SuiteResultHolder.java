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
package com.android.tradefed.result.suite;

import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.testtype.IAbi;

import java.util.Collection;
import java.util.Map;

/** Helper object to ease up serializing and deserializing the invocation results. */
public class SuiteResultHolder {
    /** The context of the invocation that generated these results. */
    public IInvocationContext context;

    /** The collection of all results from the invocation. */
    public Collection<TestRunResult> runResults;
    /** A map of each module's abi. */
    public Map<String, IAbi> modulesAbi;

    public int completeModules;
    public int totalModules;
    public long passedTests;
    public long failedTests;
    public long startTime;
    public long endTime;
}
