/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.tradefed.testtype.testdefs;

/**
 * Container object for a single test def's data
 */
class InstrumentationTestDef {
    private final String mName;
    private final String mPackage;
    private String mRunner = null;
    private String mClassName = null;
    private boolean mIsContinuous = false;
    private String mCoverageTarget = null;

    /**
     * Creates a {@link InstrumentationTestDef}.
     *
     * @param testName the unique test name
     * @param packageName the Android manifest package name of the test.
     */
    public InstrumentationTestDef(String testName, String packageName) {
        mName = testName;
        mPackage = packageName;
    }

    void setRunner(String runnerName) {
        mRunner = runnerName;
    }

    void setClassName(String className) {
        mClassName = className;
    }

    void setContinuous(boolean isContinuous) {
        mIsContinuous = isContinuous;
    }

    void setCoverageTarget(String coverageTarget) {
        mCoverageTarget = coverageTarget;
    }

    /**
     * Returns the unique name of the test definition.
     */
    String getName() {
        return mName;
    }

    /**
     * Returns the Android Manifest package name of the test application.
     */
    String getPackage() {
        return mPackage;
    }

    /**
     * Returns the fully specified  name of the instrumentation runner to use. <code>null</code>
     * if not specified.
     */
    String getRunner() {
        return mRunner;
    }

    /**
     * Returns the fully specified  name of the test class to run. <code>null</code> if not
     * specified.
     */
    String getClassName() {
        return mClassName;
    }

    /**
     * Returns <code>true</code> if test is part of the continuous test.
     */
    boolean isContinuous() {
        return mIsContinuous;
    }

    /**
     * Returns the coverage target for the test.
     */
    String getCoverageTarget() {
        return mCoverageTarget;
    }
}
