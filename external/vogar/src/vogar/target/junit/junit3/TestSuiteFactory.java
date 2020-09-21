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

package vogar.target.junit.junit3;

import java.util.List;
import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;
import org.junit.runner.Description;

/**
 * A factory for creating components of a test suite.
 *
 * @param <T> the base type of the 'test case', 'suite' and 'custom test' components
 */
public interface TestSuiteFactory<T> {

    /**
     * Create a representation of a {@link TestSuite}.
     *
     * @param name     the name of the suite.
     * @param children the transformed children of the test.
     * @return the representation of the {@link TestSuite}.
     */
    T createSuite(String name, List<T> children);

    /**
     * Create a representation of the {@link TestCase}.
     *
     * @param testCase    the test case.
     * @param description the description of the test case.
     * @return the representation of the {@link TestCase}.
     */
    T createTestCase(TestCase testCase, Description description);

    /**
     * Create a representation of a custom {@link Test}.
     *
     * @param test        the test.
     * @param description the description of the test case.
     * @return the representation of the {@link Test}.
     */
    T createCustomTest(Test test, Description description);
}
