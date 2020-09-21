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

package vogar;

/**
 * The type of runner to use for the test classes.
 */
public enum RunnerType {
    /**
     * Runs both JUnit classes and classes with a main(String[] args) method.
     */
    DEFAULT(false, true, true),

    /**
     * Runs only Caliper benchmarks.
     */
    CALIPER(true, false, false),

    /**
     * Runs only JUnit classes.
     */
    JUNIT(false, true, false),

    /**
     * Runs only classes with a main(String[] args) method.
     */
    MAIN(false, false, true);

    private final boolean supportsCaliper;
    private final boolean supportsJUnit;
    private final boolean supportsMain;

    RunnerType(boolean supportsCaliper, boolean supportsJUnit, boolean supportsMain) {
        this.supportsCaliper = supportsCaliper;
        this.supportsJUnit = supportsJUnit;
        this.supportsMain = supportsMain;
    }

    public boolean supportsCaliper() {
        return supportsCaliper;
    }

    public boolean supportsJUnit() {
        return supportsJUnit;
    }

    public boolean supportsMain() {
        return supportsMain;
    }
}
