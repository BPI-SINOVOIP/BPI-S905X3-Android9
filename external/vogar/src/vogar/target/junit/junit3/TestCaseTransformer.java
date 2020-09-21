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
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import junit.framework.AssertionFailedError;
import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

/**
 * Transforms a {@link TestCase} based test into another representation.
 *
 * <p>This delegates the work of constructing tests and a suite (collection of tests) to a
 * {@link TestCaseFactory}.
 *
 * <p>Separates the scanning of the {@link TestCase} derived class from the construction of the
 * objects representing the 'suite' and 'test' allowing it to be more easily reused.
 *
 * @param <S> the type of the 'suite' component
 * @param <T> the type of the 'test' component
 */
public class TestCaseTransformer<S,T> {

    private final TestCaseFactory<S, T> factory;

    private final Comparator<? super T> comparator;

    public TestCaseTransformer(TestCaseFactory<S, T> factory, Comparator<? super T> comparator) {
        this.comparator = comparator;
        this.factory = factory;
    }

    public S createSuite(Class<? extends TestCase> testClass) {
        List<T> tests = testsFromTestCase(testClass);
        return factory.createSuite(testClass, tests);
    }

    private T createTest(
            Class<? extends TestCase> testClass, String methodName, Annotation[] annotations) {
        return factory.createTest(testClass, methodName, annotations);
    }

    private T createWarning(Class<? extends Test> testClass, String name, Throwable throwable) {
        return factory.createFailingTest(testClass, name, throwable);
    }

    private List<T> testsFromTestCase(final Class<? extends TestCase> testClass) {
        List<T> tests = new ArrayList<>();

        // Check as much as possible in advance to avoid generating multiple error messages for the
        // same failure.
        if (factory.eagerClassValidation()) {
            try {
                validateTestClass(testClass);
            } catch (AssertionFailedError e) {
                tests.add(createWarning(testClass, "warning", e));
                return tests;
            }
        }

        for (Method method : testClass.getMethods()) {
            if (!isTestMethod(method)) {
                continue;
            }

            tests.add(createTest(testClass, method.getName(), method.getAnnotations()));
        }

        Collections.sort(tests, comparator);

        return tests;
    }

    private static boolean isTestMethod(Method m) {
        return m.getParameterTypes().length == 0
                && m.getName().startsWith("test")
                && m.getReturnType().equals(Void.TYPE);
    }

    public static void validateTestClass(Class<?> testClass) {
        try {
            TestSuite.getTestConstructor(testClass);
        } catch (NoSuchMethodException e) {
            throw new AssertionFailedError(
                    "Class " + testClass.getName()
                            + " has no public constructor TestCase(String name) or TestCase()");
        }

        if (!Modifier.isPublic(testClass.getModifiers())) {
            throw new AssertionFailedError("Class " + testClass.getName() + " is not public");
        }
    }
}
