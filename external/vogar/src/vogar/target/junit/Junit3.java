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

import java.lang.reflect.Modifier;
import junit.framework.Test;
import junit.framework.TestCase;
import vogar.ClassAnalyzer;

/**
 * Utilities for manipulating JUnit tests.
 */
public final class Junit3 {
    private Junit3() {}

    public static boolean isJunit3Test(Class<?> klass) {
        // public class FooTest extends TestCase {...}
        //   or
        // public class FooSuite {
        //    public static Test suite() {...}
        // }
        return (TestCase.class.isAssignableFrom(klass) && !Modifier.isAbstract(klass.getModifiers()))
                || new ClassAnalyzer(klass).hasMethod(true, Test.class, "suite");
    }
}
