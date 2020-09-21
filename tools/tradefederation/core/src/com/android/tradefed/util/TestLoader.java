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
package com.android.tradefed.util;

import com.android.ddmlib.Log;
import com.android.tradefed.util.ClassPathScanner.ExternalClassNameFilter;

import junit.framework.JUnit4TestAdapter;
import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import org.junit.runners.Suite.SuiteClasses;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Method;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.Collection;
import java.util.Iterator;
import java.util.Set;

/**
 * A class for loading all JUnit3 tests in a jar file
 */
public class TestLoader {

    private static final String LOG_TAG = "TestLoader";

    /**
     * Creates a {@link Test} containing all the {@link TestCase} found in given jar
     *
     * @param testJarFile the jar file to load tests from
     * @param dependentJars the additional jar files which classes in testJarFile depend on
     * @return the {@link Test} containing all tests
     */
    public Test loadTests(File testJarFile, Collection<File> dependentJars) {
        ClassPathScanner scanner = new ClassPathScanner();
        try {
            Set<String> classNames = scanner.getEntriesFromJar(testJarFile,
                    new ExternalClassNameFilter());

            ClassLoader jarClassLoader = buildJarClassLoader(testJarFile, dependentJars);
            return loadTests(classNames, jarClassLoader);
        } catch (IOException e) {
            Log.e(LOG_TAG, String.format("IOException when loading test classes from jar %s",
                    testJarFile.getAbsolutePath()));
            Log.e(LOG_TAG, e);
        }
        return null;
    }

    private ClassLoader buildJarClassLoader(File jarFile, Collection<File> dependentJars)
            throws MalformedURLException {
        URL[] urls = new URL[dependentJars.size() + 1];
        urls[0] = jarFile.toURI().toURL();
        Iterator<File> jarIter = dependentJars.iterator();
        for (int i=1; i <= dependentJars.size(); i++) {
            urls[i] = jarIter.next().toURI().toURL();
        }
        return new URLClassLoader(urls);
    }

    @SuppressWarnings("unchecked")
    private Test loadTests(Set<String> classNames, ClassLoader classLoader) {
        TestSuite testSuite = new TestSuite();
        for (String className : classNames) {
            try {
                Class<?> testClass = Class.forName(className, true, classLoader);
                if (TestCase.class.isAssignableFrom(testClass)) {
                    testSuite.addTestSuite((Class<? extends TestCase>)testClass);
                } else if (hasJUnit4Annotation(testClass)) {
                    testSuite.addTest(new JUnit4TestAdapter(testClass));
                }
            } catch (NoClassDefFoundError e) {
                Log.e(LOG_TAG, e);
            } catch (ClassNotFoundException e) {
                Log.e(LOG_TAG, e);
            } catch (RuntimeException e) {
                // catch this to prevent one bad test from stopping run
                Log.e(LOG_TAG, e);
            }
        }
        return testSuite;
    }

    /**
     * Helper to device if a class should be loaded as JUnit4.
     */
    private boolean hasJUnit4Annotation(Class<?> classObj) {
        if (classObj.isAnnotationPresent(SuiteClasses.class)) {
            return true;
        }
        for (Method m : classObj.getMethods()) {
            if (m.isAnnotationPresent(org.junit.Test.class)) {
                return true;
            }
        }
        return false;
    }
}
