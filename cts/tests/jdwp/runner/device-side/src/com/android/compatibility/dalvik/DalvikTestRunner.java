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

package com.android.compatibility.dalvik;

import dalvik.system.DexFile;
import dalvik.system.PathClassLoader;

import junit.framework.AssertionFailedError;
import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestListener;
import junit.framework.TestResult;
import junit.framework.TestSuite;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.List;
import java.util.Scanner;
import java.util.Set;

/**
 * Runs tests against the Dalvik VM.
 */
public class DalvikTestRunner {

    private static final String ABI = "--abi=";
    private static final String INCLUDE = "--include-filter=";
    private static final String EXCLUDE = "--exclude-filter=";
    private static final String INCLUDE_FILE = "--include-filter-file=";
    private static final String EXCLUDE_FILE = "--exclude-filter-file=";
    private static final String COLLECT_TESTS_ONLY = "--collect-tests-only";
    private static final String JUNIT_IGNORE = "org.junit.Ignore";

    private static final String RUNNER_JAR = "cts-dalvik-device-test-runner.jar";

    public static void main(String[] args) {
        Config config;
        try {
            config = createConfig(args);
        } catch (Throwable t) {
            // Simulate one failed test.
            System.out.println("start-run:1");
            System.out.println("start-test:FailedConfigCreation");
            System.out.println("failure:" + DalvikTestListener.stringify(t));
            System.out.println("end-run:1");
            throw new RuntimeException(t);
        }
        run(config);
    }

    private static Config createConfig(String[] args) {
        String abiName = null;
        Config config = new Config();

        for (String arg : args) {
            if (arg.startsWith(ABI)) {
                abiName = arg.substring(ABI.length());
            } else if (arg.startsWith(INCLUDE)) {
                for (String include : arg.substring(INCLUDE.length()).split(",")) {
                    config.includes.add(include);
                }
            } else if (arg.startsWith(EXCLUDE)) {
                for (String exclude : arg.substring(EXCLUDE.length()).split(",")) {
                    config.excludes.add(exclude);
                }
            } else if (arg.startsWith(INCLUDE_FILE)) {
                loadFilters(arg.substring(INCLUDE_FILE.length()), config.includes);
            } else if (arg.startsWith(EXCLUDE_FILE)) {
                loadFilters(arg.substring(EXCLUDE_FILE.length()), config.excludes);
            } else if (COLLECT_TESTS_ONLY.equals(arg)) {
                config.collectTestsOnly = true;
            }
        }

        String[] classPathItems = System.getProperty("java.class.path").split(File.pathSeparator);
        List<Class<?>> classes = getClasses(classPathItems, abiName);
        config.suite = new FilterableTestSuite(classes, config.includes, config.excludes);

        return config;
    }

    private static void run(Config config) {
        TestListener listener = new DalvikTestListener();

        int count = config.suite.countTestCases();
        System.out.println(String.format("start-run:%d", count));
        long start = System.currentTimeMillis();

        if (config.collectTestsOnly) { // only simulate running/passing the tests with the listener
            collectTests(config.suite, listener, config.includes, config.excludes);
        } else { // run the tests
            TestResult result = new TestResult();
            result.addListener(listener);
            config.suite.run(result);
        }

        long end = System.currentTimeMillis();
        System.out.println(String.format("end-run:%d", end - start));
    }

    /* Recursively collect tests, since Test elements of the TestSuite may also be TestSuite
     * objects containing Tests. */
    private static void collectTests(TestSuite suite, TestListener listener,
            Set<String> includes, Set<String> excludes) {

        Enumeration<Test> tests = suite.tests();
        while (tests.hasMoreElements()) {
            Test test = tests.nextElement();
            if (test instanceof TestSuite) {
                collectTests((TestSuite) test, listener, includes, excludes);
            } else if (shouldCollect(test, includes, excludes)) {
                listener.startTest(test);
                listener.endTest(test);
            }
        }
    }

    /* Copied from FilterableTestSuite.shouldRun(), which is private */
    private static boolean shouldCollect(Test test, Set<String> includes, Set<String> excludes) {
        String fullName = test.toString();
        String[] parts = fullName.split("[\\(\\)]");
        String className = parts[1];
        String methodName = String.format("%s#%s", className, parts[0]);
        int index = className.lastIndexOf('.');
        String packageName = index < 0 ? "" : className.substring(0, index);

        if (excludes.contains(packageName)) {
            // Skip package because it was excluded
            return false;
        }
        if (excludes.contains(className)) {
            // Skip class because it was excluded
            return false;
        }
        if (excludes.contains(methodName)) {
            // Skip method because it was excluded
            return false;
        }
        return includes.isEmpty()
                || includes.contains(methodName)
                || includes.contains(className)
                || includes.contains(packageName);
    }

    private static void loadFilters(String filename, Set<String> filters) {
        try {
            Scanner in = new Scanner(new File(filename));
            while (in.hasNextLine()) {
                filters.add(in.nextLine());
            }
            in.close();
        } catch (FileNotFoundException e) {
            System.out.println(String.format("File %s not found when loading filters", filename));
        }
    }

    private static List<Class<?>> getClasses(String[] jars, String abiName) {
        List<Class<?>> classes = new ArrayList<>();
        for (String jar : jars) {
            if (jar.contains(RUNNER_JAR)) {
                // The runner jar must be added to the class path to invoke DalvikTestRunner,
                // but should not be searched for test classes
                continue;
            }
            try {
                ClassLoader loader = createClassLoader(jar, abiName);
                DexFile file = new DexFile(jar);
                Enumeration<String> entries = file.entries();
                while (entries.hasMoreElements()) {
                    String e = entries.nextElement();
                    try {
                        Class<?> cls = loader.loadClass(e);
                        if (isTestClass(cls)) {
                            classes.add(cls);
                        }
                    } catch (ClassNotFoundException ex) {
                        System.out.println(String.format(
                                "Skipping dex entry %s in %s", e, jar));
                    }
                }
            } catch (IllegalAccessError | IOException e) {
                e.printStackTrace();
            } catch (Exception e) {
                throw new RuntimeException(jar, e);
            }
        }
        return classes;
    }

    private static ClassLoader createClassLoader(String jar, String abiName) {
        StringBuilder libPath = new StringBuilder();
        libPath.append(jar).append("!/lib/").append(abiName);
        return new PathClassLoader(
                jar, libPath.toString(), DalvikTestRunner.class.getClassLoader());
    }

    private static boolean isTestClass(Class<?> cls) {
        // FIXME(b/25154702): have to have a null check here because some
        // classes such as
        // SQLite.JDBC2z.JDBCPreparedStatement can be found in the classes.dex
        // by DexFile.entries
        // but trying to load them with DexFile.loadClass returns null.
        if (cls == null) {
            return false;
        }
        for (Annotation a : cls.getAnnotations()) {
            if (a.annotationType().getName().equals(JUNIT_IGNORE)) {
                return false;
            }
        }

        try {
            if (!hasPublicTestMethods(cls)) {
                return false;
            }
        } catch (Throwable exc) {
            throw new RuntimeException(cls.toString(), exc);
        }

        // TODO: Add junit4 support here
        int modifiers = cls.getModifiers();
        return (Test.class.isAssignableFrom(cls)
                && Modifier.isPublic(modifiers)
                && !Modifier.isStatic(modifiers)
                && !Modifier.isInterface(modifiers)
                && !Modifier.isAbstract(modifiers));
    }

    private static boolean hasPublicTestMethods(Class<?> cls) {
        for (Method m : cls.getDeclaredMethods()) {
            if (isPublicTestMethod(m)) {
                return true;
            }
        }
        return false;
    }

    private static boolean isPublicTestMethod(Method m) {
        boolean hasTestName = m.getName().startsWith("test");
        boolean takesNoParameters = (m.getParameterTypes().length == 0);
        boolean returnsVoid = m.getReturnType().equals(Void.TYPE);
        boolean isPublic = Modifier.isPublic(m.getModifiers());
        return hasTestName && takesNoParameters && returnsVoid && isPublic;
    }

    // TODO: expand this to setup and teardown things needed by Dalvik tests.
    private static class DalvikTestListener implements TestListener {
        /**
         * {@inheritDoc}
         */
        @Override
        public void startTest(Test test) {
            System.out.println(String.format("start-test:%s", getId(test)));
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void endTest(Test test) {
            System.out.println(String.format("end-test:%s", getId(test)));
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void addFailure(Test test, AssertionFailedError error) {
            System.out.println(String.format("failure:%s", stringify(error)));
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void addError(Test test, Throwable error) {
            System.out.println(String.format("failure:%s", stringify(error)));
        }

        private String getId(Test test) {
            String className = test.getClass().getName();
            if (test instanceof TestCase) {
                return String.format("%s#%s", className, ((TestCase) test).getName());
            }
            return className;
        }

        public static String stringify(Throwable error) {
            return Arrays.toString(error.getStackTrace()).replaceAll("\n", " ");
        }
    }

    private static class Config {
        Set<String> includes = new HashSet<>();
        Set<String> excludes = new HashSet<>();
        boolean collectTestsOnly = false;
        TestSuite suite;
    }

    /**
     * A {@link TestSuite} that can filter which tests run, given the include and exclude filters.
     *
     * This had to be private inner class because the test runner would find it and think it was a
     * suite of tests, but it has no tests in it, causing a crash.
     */
    private static class FilterableTestSuite extends TestSuite {

        private Set<String> mIncludes;
        private Set<String> mExcludes;

        public FilterableTestSuite(List<Class<?>> classes, Set<String> includes,
                Set<String> excludes) {
            super(classes.toArray(new Class<?>[classes.size()]));
            mIncludes = includes;
            mExcludes = excludes;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public int countTestCases() {
            return countTests(this);
        }

        private int countTests(Test test) {
            if (test instanceof TestSuite) {
                // If the test is a suite it could contain multiple tests, these need to be split
                // out into separate tests so they can be filtered
                TestSuite suite = (TestSuite) test;
                Enumeration<Test> enumerator = suite.tests();
                int count = 0;
                while (enumerator.hasMoreElements()) {
                    count += countTests(enumerator.nextElement());
                }
                return count;
            } else if (shouldRun(test)) {
                return 1;
            }
            return 0;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void runTest(Test test, TestResult result) {
            runTests(test, result);
        }

        private void runTests(Test test, TestResult result) {
            if (test instanceof TestSuite) {
                // If the test is a suite it could contain multiple tests, these need to be split
                // out into separate tests so they can be filtered
                TestSuite suite = (TestSuite) test;
                Enumeration<Test> enumerator = suite.tests();
                while (enumerator.hasMoreElements()) {
                    runTests(enumerator.nextElement(), result);
                }
            } else if (shouldRun(test)) {
                test.run(result);
            }
        }

        private boolean shouldRun(Test test) {
            String fullName = test.toString();
            String[] parts = fullName.split("[\\(\\)]");
            String className = parts[1];
            String methodName = String.format("%s#%s", className, parts[0]);
            int index = className.lastIndexOf('.');
            String packageName = index < 0 ? "" : className.substring(0, index);

            if (mExcludes.contains(packageName)) {
                // Skip package because it was excluded
                return false;
            }
            if (mExcludes.contains(className)) {
                // Skip class because it was excluded
                return false;
            }
            if (mExcludes.contains(methodName)) {
                // Skip method because it was excluded
                return false;
            }
            return mIncludes.isEmpty()
                    || mIncludes.contains(methodName)
                    || mIncludes.contains(className)
                    || mIncludes.contains(packageName);
        }
    }
}
