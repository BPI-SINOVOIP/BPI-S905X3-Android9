/*
 * Copyright (C) 2008 The Android Open Source Project
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

package dot.junit;

public class DxAbstractMain {

    /* NOTE: Because of how tests are generated, this is replicated between
     *       this class and DxTestCase.
     */

    private static void checkError(Class<?> expectedErrorClass, Throwable thrown,
                                   boolean in_invocation_exc) {
        if (expectedErrorClass != null && thrown == null) {
            fail("Expected error of type " + expectedErrorClass, null);
        } else if (expectedErrorClass == null && thrown != null) {
            fail("Unexpected error " + thrown, thrown);
        } else if (expectedErrorClass != null && thrown != null) {
            if (in_invocation_exc) {
                if (!(thrown instanceof java.lang.reflect.InvocationTargetException)) {
                    fail("Expected invocation target exception, but got " + thrown, thrown);
                }
                thrown = thrown.getCause();
            }
            if (!expectedErrorClass.equals(thrown.getClass())) {
                fail("Expected error of type " + expectedErrorClass + ", but got " +
                         thrown.getClass(), thrown);
            }
        }
    }

    /**
     * Try to load the class with the given name, and check for the expected error.
     */
    public static Class<?> load(String className, Class<?> expectedErrorClass) {
        Class<?> c;
        try {
            c = Class.forName(className);
        } catch (Throwable t) {
            if (expectedErrorClass != null) {
                checkError(expectedErrorClass, t, false);
            } else {
                fail("Could not load class " + className, t);
            }
            return null;
        }
        checkError(expectedErrorClass, null, false);
        return c;
    }

    /**
     * Try to load the class with the given name, find the "run" method and run it.
     * If expectedErrorClass is not null, check for an exception of that class.
     */
    public static void loadAndRun(String className, boolean isStatic, boolean wrapped,
                                  Class<?> expectedErrorClass, Object... args) {
        Class<?> c = load(className, null);

        java.lang.reflect.Method method = null;
        // We expect only ever one declared method named run, but don't know the arguments. So
        // search for one.
        for (java.lang.reflect.Method m : c.getDeclaredMethods()) {
            if (m.getName().equals("run")) {
                method = m;
                break;
            }
        }
        if (method == null) {
            fail("Could not find method 'run'", null);
        }

        Object receiver = null;
        if (!isStatic) {
            try {
                receiver = c.newInstance();
            } catch (Exception exc) {
                fail("Could not instantiate " + className, exc);
            }
        }

        try {
            method.invoke(receiver, args);
        } catch (Throwable t) {
            checkError(expectedErrorClass, t, wrapped);
            return;
        }
        checkError(expectedErrorClass, null, false);
    }

    public static void loadAndRun(String className, Class<?> expectedErrorClass) {
        loadAndRun(className, false, true, expectedErrorClass);
    }

    public static void loadAndRun(String className, Class<?> expectedErrorClass, Object... args) {
        loadAndRun(className, false, true, expectedErrorClass, args);
    }

    static public void assertEquals(int expected, int actual) {
        if (expected != actual)
            throw new AssertionFailedException(
                    "not equals. Expected " + expected + " actual " + actual);
    }

    static public void assertEquals(String message, int expected, int actual) {
        if (expected != actual)
            throw new AssertionFailedException(
                    "not equals: " + message + " Expected " + expected + " actual " + actual);
    }

    static public void assertEquals(long expected, long actual) {
        if (expected != actual)
            throw new AssertionFailedException(
                    "not equals. Expected " + expected + " actual " + actual);
    }

    static public void assertEquals(double expected, double actual, double delta) {
        if (!(Math.abs(expected - actual) <= delta))
            throw new AssertionFailedException("not within delta");
    }

    static public void assertEquals(Object expected, Object actual) {
        if (expected == null && actual == null)
            return;
        if (expected != null && expected.equals(actual))
            return;
        throw new AssertionFailedException("not the same: " + expected + " vs " + actual);
    }

    static public void assertTrue(boolean condition) {
        if (!condition)
            throw new AssertionFailedException("condition was false");
    }

    static public void assertFalse(boolean condition) {
        if (condition)
            throw new AssertionFailedException("condition was true");
    }

    static public void assertNotNull(Object object) {
        if (object == null)
            throw new AssertionFailedException("object was null");
    }

    static public void assertNull(Object object) {
        if (object != null)
            throw new AssertionFailedException("object was not null");
    }

    static public void fail(String message) {
        fail(message, null);
    }

    static public void fail(String message, Throwable cause) {
        throw new AssertionFailedException(message, cause);
    }
}
