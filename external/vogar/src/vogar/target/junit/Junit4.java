/*
 * Copyright (C) 2011 The Android Open Source Project
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

import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Suite;

/**
 * Utilities for manipulating JUnit4 tests.
 */
public final class Junit4 {
    private Junit4() {}

    public static boolean isJunit4Test(Class<?> klass) {
        boolean isTestSuite = false;
        boolean hasSuiteClasses = false;

        boolean concrete = !Modifier.isAbstract(klass.getModifiers());

        // @RunWith(Suite.class)
        // @SuiteClasses( ... )
        // public class MyTest { ... }
        //   or
        // @RunWith(Parameterized.class)
        // public class MyTest { ... }
        for (Annotation a : klass.getAnnotations()) {
            Class<?> annotationClass = a.annotationType();

            if (RunWith.class.isAssignableFrom(annotationClass)) {
                Class<?> runnerClass = ((RunWith) a).value();
                if (Suite.class.isAssignableFrom(runnerClass)) {
                    isTestSuite = true;
                } else if (Parameterized.class.isAssignableFrom(runnerClass)) {
                    // @Parameterized test classes are instantiated so must be concrete.
                    return concrete;
                }
            } else if (Suite.SuiteClasses.class.isAssignableFrom(annotationClass)) {
                hasSuiteClasses = true;
            }

            if (isTestSuite && hasSuiteClasses) {
                // This doesn't instantiate the class so doesn't care if it's abstract.
                return true;
            }
        }

        // Test classes that have methods annotated with @Test are instantiated so must be concrete.
        if (!concrete) {
            return false;
        }

        // public class MyTest {
        //     @Test
        //     public void example() { ... }
        // }
        for (Method m : klass.getMethods()) {
            for (Annotation a : m.getAnnotations()) {
                if (org.junit.Test.class.isAssignableFrom(a.annotationType())) {
                    return true;
                }
            }
        }

        return false;
    }
}
