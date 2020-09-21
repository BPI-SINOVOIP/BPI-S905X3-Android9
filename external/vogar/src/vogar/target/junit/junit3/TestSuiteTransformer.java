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
import java.util.ArrayList;
import java.util.List;
import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;
import org.junit.runner.Describable;
import org.junit.runner.Description;

/**
 * Transforms a hierarchy of {@link Test} objects into a hierarchy of other objects.
 *
 * <p>This delegates the work of constructing tests and a suite (collection of tests) to a
 * {@link TestSuiteFactory}.
 *
 * <p>Separates the traversing of the {@link TestSuite} hierarchy from the construction of the
 * objects representing the 'suite' and 'test' allowing it to be more easily reused.
 *
 * @param <T> the base type of the 'test case', 'suite' and 'custom test' components
 */
public class TestSuiteTransformer<T> {

    private static final Annotation[] NO_ANNOTATIONS = new Annotation[0];
    private final TestSuiteFactory<T> factory;

    public TestSuiteTransformer(TestSuiteFactory<T> factory) {
        this.factory = factory;
    }

    public T transform(Class<?> suiteClass, Test test) {
        Description description = Description.createTestDescription(
                suiteClass, "suite", suiteClass.getAnnotations());
        return transformTest(test, suiteClass.getName(), description);
    }

    private T transformTest(Test test, String defaultName, Description suiteDescription) {
        if (test instanceof TestSuite) {
            return transformSuite((TestSuite) test, defaultName, suiteDescription);
        } else if (test instanceof TestCase) {
            final TestCase testCase = (TestCase) test;
            Description description = makeDescription(test);
            return factory.createTestCase(testCase, description);
        } else {
            return factory.createCustomTest(test, suiteDescription);
        }
    }

    private T transformSuite(
            TestSuite testSuite, String defaultName, Description suiteDescription) {
        List<T> children = new ArrayList<>();
        int count = testSuite.testCount();
        String name = testSuite.getName();
        if (name == null) {
            name = defaultName;
        }
        for (int i = 0; i < count; i++) {
            Test test = testSuite.testAt(i);
            children.add(transformTest(test, name + "[" + i + "]", suiteDescription));
        }
        return factory.createSuite(name, children);
    }

    private static Description makeDescription(Test test) {
        if (test instanceof Describable) {
            return ((Describable) test).getDescription();
        } else if (test instanceof TestCase) {
            TestCase testCase = (TestCase) test;
            Class<? extends TestCase> testCaseClass = testCase.getClass();
            String methodName = testCase.getName();
            Annotation[] annotations = NO_ANNOTATIONS;
            if (methodName != null) {
                try {
                    Method method = testCaseClass.getMethod(methodName);
                    annotations = method.getAnnotations();
                } catch (NoSuchMethodException ignored) {
                    // Do nothing, an error will be thrown when the test itself is run.
                }
            }
            return Description.createTestDescription(testCaseClass, methodName, annotations);
        } else if (test instanceof TestSuite) {
            TestSuite testSuite = (TestSuite) test;
            Description description = Description.createSuiteDescription(testSuite.getName());
            int count = testSuite.testCount();
            for (int i = 0; i < count; i++) {
                Test childTest = testSuite.testAt(i);
                description.addChild(makeDescription(childTest));
            }
            return description;
        } else {
            throw new UnsupportedOperationException("Cannot get description of: " + test);
        }
    }
}
