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
package com.android.tradefed.testtype;

import java.util.Set;

/**
 * A runner that can filter which tests to run based on annotations.
 *
 * <p>A test will be run IFF it matches one or more of the include filters AND does not match any
 * of the exclude filters. If no include filters are given all tests should be run as long as they
 * do not match any of the exclude filters.</p>
 */
public interface ITestAnnotationFilterReceiver {
    /**
     * Adds an annotation to include if a tests if marked with it.
     */
    void addIncludeAnnotation(String annotation);

    /**
     * Adds an annotation to exclude if a tests if marked with it.
     */
    void addExcludeAnnotation(String notAnnotation);

    /**
     * Adds a {@link Set} of annotations to include if a tests if marked with it.
     */
    void addAllIncludeAnnotation(Set<String> annotations);

    /**
     * Adds a {@link Set} of annotations to exclude if a tests if marked with it.
     */
    void addAllExcludeAnnotation(Set<String> notAnnotations);
}
