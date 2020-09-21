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

/** Reporter that allows to generate reports in a particular format. TODO: fix logged file */
public abstract class FormattedGeneratorReporter extends SuiteResultReporter {

    /** {@inheritDoc} */
    @Override
    public final void invocationEnded(long elapsedTime) {
        // Let the parent create the results structures
        super.invocationEnded(elapsedTime);

        SuiteResultHolder holder = generateResultHolder();

        IFormatterGenerator generator = createFormatter();

        finalizeResults(generator, holder);
    }

    /**
     * Step that handles using the {@link IFormatterGenerator} and the {@link SuiteResultHolder} in
     * order to generate some formatted results.
     *
     * @param generator
     * @param resultHolder
     */
    public abstract void finalizeResults(
            IFormatterGenerator generator, SuiteResultHolder resultHolder);

    /** Returns a new instance of the {@link IFormatterGenerator} to use. */
    public abstract IFormatterGenerator createFormatter();

    private SuiteResultHolder generateResultHolder() {
        final SuiteResultHolder holder = new SuiteResultHolder();
        holder.context = getInvocationContext();

        holder.runResults = getRunResults();
        //holder.loggedFiles = mLoggedFiles;
        holder.modulesAbi = getModulesAbi();

        holder.completeModules = getCompleteModules();
        holder.totalModules = getTotalModules();
        holder.passedTests = getPassedTests();
        holder.failedTests = getFailedTests();

        holder.startTime = getStartTime();
        holder.endTime = getElapsedTime() + getStartTime();

        return holder;
    }
}
