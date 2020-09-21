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
package com.android.tradefed.testtype.suite;

/**
 * Helper class for operation related to merging {@link ITestSuite} and {@link ModuleDefinition}
 * after a split.
 */
public class ModuleMerger {

    private static void mergeModules(ModuleDefinition module1, ModuleDefinition module2) {
        if (!module1.getId().equals(module2.getId())) {
            throw new IllegalArgumentException(
                    String.format(
                            "Modules must have the same id to be mergeable: received %s and "
                                    + "%s",
                            module1.getId(), module2.getId()));
        }
        module1.addTests(module2.getTests());
    }

    /**
     * Merge the modules from one suite to another.
     *
     * @param suite1 the suite that will receive the module from the other.
     * @param suite2 the suite that will give the module.
     */
    public static void mergeSplittedITestSuite(ITestSuite suite1, ITestSuite suite2) {
        if (suite1.getDirectModule() == null) {
            throw new IllegalArgumentException("suite was not a splitted suite.");
        }
        if (suite2.getDirectModule() == null) {
            throw new IllegalArgumentException("suite was not a splitted suite.");
        }
        mergeModules(suite1.getDirectModule(), suite2.getDirectModule());
    }

    /** Returns true if the two suites are part of the same original split. False otherwise. */
    public static boolean arePartOfSameSuite(ITestSuite suite1, ITestSuite suite2) {
        if (suite1.getDirectModule() == null) {
            return false;
        }
        if (suite2.getDirectModule() == null) {
            return false;
        }
        if (!suite1.getDirectModule().getId().equals(suite2.getDirectModule().getId())) {
            return false;
        }
        return true;
    }
}
