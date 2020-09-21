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

import java.lang.annotation.Annotation;
import java.util.List;
import junit.framework.Test;
import junit.framework.TestCase;

/**
 * A factory for creating components of a {@link TestCase} derived test suite.
 *
 * @param <S> the type of the 'suite' component
 * @param <T> the type of the 'test' component
 */
public interface TestCaseFactory<S,T> {

    /**
     * Determine whether class validation is done eagerly up front and so will result in one class
     * level error or not.
     *
     * <p>If this returns true then it takes responsibility for validating the test class, using
     * {@link TestCaseTransformer#validateTestClass(Class)}.
     *
     * @return true if the class validation should be eagerly, false otherwise.
     */
    boolean eagerClassValidation();

    /**
     * Create the 'test' for the method.
     */
    T createTest(Class<? extends TestCase> testClass, String methodName, Annotation[] annotations);

    /**
     * Create a 'test' that when run will throw the supplied throwable.
     */
    T createFailingTest(Class<? extends Test> testClass, String name, Throwable throwable);

    /**
     * Constructs a test suite from the given class and list of tests.
     */
    S createSuite(Class<? extends TestCase> testClass, List<T> tests);
}
