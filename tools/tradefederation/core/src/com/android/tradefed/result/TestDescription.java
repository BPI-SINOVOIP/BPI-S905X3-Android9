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
package com.android.tradefed.result;

import com.android.ddmlib.testrunner.TestIdentifier;

import java.io.Serializable;
import java.lang.annotation.Annotation;
import java.util.Arrays;
import java.util.Collection;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Class representing information about a test case. */
public final class TestDescription implements Serializable {

    /** Regex for method parameterized. For example: testName[0] */
    public static final Pattern PARAMETERIZED_TEST_REGEX = Pattern.compile("(.*)?\\[(.*)\\]$");

    private final String mClassName;
    private final String mTestName;
    private final String mTestNameNoParams;
    private Annotation[] mAnnotations;

    /**
     * Constructor
     *
     * @param className The name of the class holding the test.
     * @param testName The test (method) name.
     */
    public TestDescription(String className, String testName) {
        if (className == null || testName == null) {
            throw new IllegalArgumentException("className and testName must be non-null");
        }
        mClassName = className;
        mTestName = testName;
        mAnnotations = new Annotation[0];

        // If the method looks parameterized, track the base non-parameterized name.
        Matcher m = PARAMETERIZED_TEST_REGEX.matcher(testName);
        if (m.find()) {
            mTestNameNoParams = m.group(1);
        } else {
            mTestNameNoParams = testName;
        }
    }

    /**
     * Constructor
     *
     * @param className The name of the class holding the test.
     * @param testName The test (method) name.
     * @param annotations List of {@link Annotation} associated with the test case.
     */
    public TestDescription(String className, String testName, Annotation... annotations) {
        this(className, testName);
        mAnnotations = annotations;
    }

    /**
     * Constructor
     *
     * @param className The name of the class holding the test.
     * @param testName The test (method) name.
     * @param annotations Collection of {@link Annotation} associated with the test case.
     */
    public TestDescription(String className, String testName, Collection<Annotation> annotations) {
        this(className, testName, annotations.toArray(new Annotation[annotations.size()]));
    }

    /**
     * @return the annotation of type annotationType that is attached to this description node, or
     *     null if none exists
     */
    public <T extends Annotation> T getAnnotation(Class<T> annotationType) {
        for (Annotation each : mAnnotations) {
            if (each.annotationType().equals(annotationType)) {
                return annotationType.cast(each);
            }
        }
        return null;
    }

    /** @return all of the annotations attached to this description node */
    public Collection<Annotation> getAnnotations() {
        return Arrays.asList(mAnnotations);
    }

    /** Returns the fully qualified class name of the test. */
    public String getClassName() {
        return mClassName;
    }

    /**
     * Returns the name of the test with the parameters, if it's parameterized test. Returns the
     * regular test name if not a parameterized test.
     */
    public String getTestName() {
        return mTestName;
    }

    /** Returns the name of the test without any parameters (if it's a parameterized method). */
    public String getTestNameWithoutParams() {
        return mTestNameNoParams;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((mClassName == null) ? 0 : mClassName.hashCode());
        result = prime * result + ((mTestName == null) ? 0 : mTestName.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) return true;
        if (obj == null) return false;
        if (getClass() != obj.getClass()) return false;
        TestDescription other = (TestDescription) obj;

        if (!mClassName.equals(other.mClassName) || !mTestName.equals(other.mTestName)) {
            return false;
        }
        return true;
    }

    @Override
    public String toString() {
        return String.format("%s#%s", getClassName(), getTestName());
    }

    /**
     * Create a {@link TestDescription} from a {@link TestIdentifier}. Used for ease of conversion
     * from one to another.
     *
     * @param testId The {@link TestIdentifier} to convert.
     * @return the created {@link TestDescription} with the TestIdentifier values.
     */
    public static TestDescription createFromTestIdentifier(TestIdentifier testId) {
        return new TestDescription(testId.getClassName(), testId.getTestName());
    }

    /**
     * Create a {@link TestIdentifier} from a {@link TestDescription}. Useful for converting a
     * description during testing.
     *
     * @param desc The {@link TestDescription} to convert.
     * @return The created {@link TestIdentifier} with the TestDescription values.
     */
    public static TestIdentifier convertToIdentifier(TestDescription desc) {
        return new TestIdentifier(desc.getClassName(), desc.getTestName());
    }
}
