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

package vogar.target.junit;

import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.Set;
import javax.annotation.Nullable;
import org.junit.runner.Description;

/**
 * Utilities for bridging between JUnit and Vogar.
 */
public class JUnitUtils {
    private JUnitUtils() {}

    /**
     * Get the name of the test in {@code <class>(#<method>)?} format.
     */
    public static String getTestName(Description description) {
        String className = description.getClassName();
        String methodName = description.getMethodName();
        return getTestName(className, methodName);
    }

    /**
     * Get the name of the test in {@code <class>(#<method>)?} format.
     */
    public static String getTestName(String className, String methodName) {
        if (methodName == null) {
            return className;
        } else {
            return className + "#" + methodName;
        }
    }

    /**
     * Merge the qualification and list of method names into a single set.
     * @param qualification the qualification applied to a class name, i.e. the part after the #
     * @param args the list of method names
     * @return The set of method names, possibly empty.
     */
    public static Set<String> mergeQualificationAndArgs(@Nullable String qualification, String[] args) {
        Set<String> methodNames = new LinkedHashSet<>();
        if (qualification != null) {
            methodNames.add(qualification);
        }
        Collections.addAll(methodNames, args);
        return methodNames;
    }
}
