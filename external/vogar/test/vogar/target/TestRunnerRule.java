/*
 * Copyright (C) 2015 The Android Open Source Project
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
package vogar.target;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Properties;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;
import vogar.TestProperties;

/**
 * Creates {@link TestRunner} for tests.
 *
 * <p>Obtains test specific arguments from the {@link TestRunnerProperties} annotation on the test
 * and makes them available to the test itself.
 *
 * @see TestRunnerProperties
 */
public class TestRunnerRule implements TestRule {

    private Properties properties;
    private TestRunnerProperties testRunnerProperties;

    @Override
    public Statement apply(Statement base, Description description) {
        testRunnerProperties = description.getAnnotation(TestRunnerProperties.class);
        if (testRunnerProperties != null) {
            properties = new Properties();
            setProperty(TestProperties.MONITOR_PORT, testRunnerProperties.monitorPort());
            String testClassOrPackage = treatEmptyAsNull(testRunnerProperties.testClassOrPackage());
            if (testClassOrPackage == null) {
                Class<?> testClass = testRunnerProperties.testClass();
                if (testClass != TestRunnerProperties.Default.class) {
                    testClassOrPackage = testClass.getName();
                }
            }
            setProperty(TestProperties.TEST_CLASS_OR_PACKAGE, testClassOrPackage);
            setProperty(TestProperties.RUNNER_TYPE, testRunnerProperties.runnerType().toString());
            setProperty(TestProperties.TIMEOUT, testRunnerProperties.timeout());
        }
        return base;
    }

    public Class<?> testClass() {
        return testRunnerProperties.testClass();
    }

    private void setProperty(String key, String value) {
        value = treatEmptyAsNull(value);
        if (value != null) {
            properties.setProperty(key, value);
        }
    }

    private void setProperty(String key, boolean value) {
        properties.setProperty(key, String.valueOf(value));
    }

    private void setProperty(String key, int value) {
        properties.setProperty(key, String.valueOf(value));
    }

    private String treatEmptyAsNull(String s) {
        return s.equals("") ? null : s;
    }

    /**
     * Create the {@link TestRunner} using properties provided by {@link TestRunnerProperties} if
     * available.
     *
     * @param args the command line arguments for the {@link TargetRunner} instance.
     */
    public TestRunner createTestRunner(String... args) {
        if (properties == null) {
            throw new IllegalStateException("Cannot create TestRunner as test does not have an "
                    + "associated @" + TestRunnerProperties.class.getName() + " annotation");
        }

        return new TestRunner(properties, new ArrayList<>(Arrays.asList(args)));
    }
}
