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
package com.android.tradefed.util;

import com.android.tradefed.log.LogUtil.CLog;

import org.junit.runner.Description;

import java.lang.annotation.Annotation;
import java.lang.reflect.AnnotatedElement;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

/**
 * Helper class for filtering tests
 */
public class TestFilterHelper {

    /** The include filters of the test name to run */
    private Set<String> mIncludeFilters = new HashSet<>();

    /** The exclude filters of the test name to run */
    private Set<String> mExcludeFilters = new HashSet<>();

    /** The include annotations of the test to run */
    private Set<String> mIncludeAnnotations = new HashSet<>();

    /** The exclude annotations of the test to run */
    private Set<String> mExcludeAnnotations = new HashSet<>();

    public TestFilterHelper() {
    }

    public TestFilterHelper(Collection<String> includeFilters, Collection<String> excludeFilters,
            Collection<String> includeAnnotation, Collection<String> excludeAnnotation) {
        mIncludeFilters.addAll(includeFilters);
        mExcludeFilters.addAll(excludeFilters);
        mIncludeAnnotations.addAll(includeAnnotation);
        mExcludeAnnotations.addAll(excludeAnnotation);
    }

    /**
     * Adds a filter of which tests to include
     */
    public void addIncludeFilter(String filter) {
        mIncludeFilters.add(filter);
    }

    /**
     * Adds the {@link Set} of filters of which tests to include
     */
    public void addAllIncludeFilters(Set<String> filters) {
        mIncludeFilters.addAll(filters);
    }

    /**
     * Adds a filter of which tests to exclude
     */
    public void addExcludeFilter(String filter) {
        mExcludeFilters.add(filter);
    }

    /**
     * Adds the {@link Set} of filters of which tests to exclude.
     */
    public void addAllExcludeFilters(Set<String> filters) {
        mExcludeFilters.addAll(filters);
    }

    /**
     * Adds an include annotation of the test to run
     */
    public void addIncludeAnnotation(String annotation) {
        mIncludeAnnotations.add(annotation);
    }

    /**
     * Adds the {@link Set} of include annotation of the test to run
     */
    public void addAllIncludeAnnotation(Set<String> annotations) {
        mIncludeAnnotations.addAll(annotations);
    }

    /**
     * Adds an exclude annotation of the test to run
     */
    public void addExcludeAnnotation(String notAnnotation) {
        mExcludeAnnotations.add(notAnnotation);
    }

    /**
     * Adds the {@link Set} of exclude annotation of the test to run
     */
    public void addAllExcludeAnnotation(Set<String> notAnnotations) {
        mExcludeAnnotations.addAll(notAnnotations);
    }

    public Set<String> getIncludeFilters() {
        return mIncludeFilters;
    }

    public Set<String> getExcludeFilters() {
        return mExcludeFilters;
    }

    public Set<String> getIncludeAnnotation() {
        return mIncludeAnnotations;
    }

    public Set<String> getExcludeAnnotation() {
        return mExcludeAnnotations;
    }


    /**
     * Check if an element that has annotation passes the filter
     *
     * @param annotatedElement the element to filter
     * @return true if the test should run, false otherwise
     */
    public boolean shouldTestRun(AnnotatedElement annotatedElement) {
        return shouldTestRun(Arrays.asList(annotatedElement.getAnnotations()));
    }

    /**
     * Check if the {@link Description} that contains annotations passes the filter
     *
     * @param desc the element to filter
     * @return true if the test should run, false otherwise
     */
    public boolean shouldTestRun(Description desc) {
        return shouldTestRun(desc.getAnnotations());
    }

    /**
     * Internal helper to determine if a particular test should run based on its annotations.
     */
    private boolean shouldTestRun(Collection<Annotation> annotationsList) {
        if (isExcluded(annotationsList)) {
            return false;
        }
        return isIncluded(annotationsList);
    }

    private boolean isExcluded(Collection<Annotation> annotationsList) {
        if (!mExcludeAnnotations.isEmpty()) {
            for (Annotation a : annotationsList) {
                if (mExcludeAnnotations.contains(a.annotationType().getName())) {
                    // If any of the method annotation match an ExcludeAnnotation, don't run it
                    CLog.i("Skipping %s, ExcludeAnnotation exclude it", a);
                    return true;
                }
            }
        }
        return false;
    }

    private boolean isIncluded(Collection<Annotation> annotationsList) {
        if (!mIncludeAnnotations.isEmpty()) {
            Set<String> neededAnnotation = new HashSet<String>();
            neededAnnotation.addAll(mIncludeAnnotations);
            for (Annotation a : annotationsList) {
                if (neededAnnotation.contains(a.annotationType().getName())) {
                    neededAnnotation.remove(a.annotationType().getName());
                }
            }
            if (neededAnnotation.size() != 0) {
                // The test needs to have all the include annotation to pass.
                CLog.i("Skipping, IncludeAnnotation filtered it");
                return false;
            }
        }
        return true;
    }

    /**
     * Check if an element that has annotation passes the filter
     *
     * @param packageName name of the method's package
     * @param classObj method's class
     * @param method test method
     * @return true if the test method should run, false otherwise
     */
    public boolean shouldRun(String packageName, Class<?> classObj, Method method) {
        String className = classObj.getName();
        String methodName = String.format("%s#%s", className, method.getName());
        if (!shouldRunFilter(packageName, className, methodName)) {
            return false;
        }
        // If class is explicitly annotated to be excluded.
        if (isExcluded(Arrays.asList(classObj.getAnnotations()))) {
            return false;
        }
        // if class include but method exclude, we exclude
        if (isIncluded(Arrays.asList(classObj.getAnnotations()))
                && isExcluded(Arrays.asList(method.getAnnotations()))) {
            return false;
        }
        // If a class is explicitly included and check above says method could run, we skip method
        // check, it will be included.
        if (mIncludeAnnotations.isEmpty()
                || !isIncluded(Arrays.asList(classObj.getAnnotations()))) {
            if (!shouldTestRun(method)) {
                return false;
            }
        }
        return mIncludeFilters.isEmpty()
                || mIncludeFilters.contains(methodName)
                || mIncludeFilters.contains(className)
                || mIncludeFilters.contains(packageName);
    }

    /**
     * Check if an element that has annotation passes the filter
     *
     * @param desc a {@link Description} that describes the test.
     * @return true if the test method should run, false otherwise
     */
    public boolean shouldRun(Description desc) {
        // We need to build the packageName for a description object
        Class<?> classObj = null;
        try {
            classObj = Class.forName(desc.getClassName());
        } catch (ClassNotFoundException e) {
            throw new IllegalArgumentException(String.format("Could not load Test class %s",
                    classObj), e);
        }
        String packageName = classObj.getPackage().getName();

        String className = desc.getClassName();
        String methodName = String.format("%s#%s", className, desc.getMethodName());
        if (!shouldRunFilter(packageName, className, methodName)) {
            return false;
        }
        if (!shouldTestRun(desc)) {
            return false;
        }
        return mIncludeFilters.isEmpty()
                || mIncludeFilters.contains(methodName)
                || mIncludeFilters.contains(className)
                || mIncludeFilters.contains(packageName);
    }

    /**
     * Internal helper to check if a particular test should run based on its package, class, method
     * names.
     */
    private boolean shouldRunFilter(String packageName, String className, String methodName) {
        if (mExcludeFilters.contains(packageName)) {
            // Skip package because it was excluded
            CLog.i("Skip package %s because it was excluded", packageName);
            return false;
        }
        if (mExcludeFilters.contains(className)) {
            // Skip class because it was excluded
            CLog.i("Skip class %s because it was excluded", className);
            return false;
        }
        if (mExcludeFilters.contains(methodName)) {
            // Skip method because it was excluded
            CLog.i("Skip method %s in class %s because it was excluded", methodName, className);
            return false;
        }
        return true;
    }
}
