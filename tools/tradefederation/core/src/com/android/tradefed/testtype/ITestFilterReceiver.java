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
package com.android.tradefed.testtype;

import java.util.Set;

/**
 * A runner that can filter which tests to run.
 *
 * <p>A test will be run IFF it matches one or more of the include filters AND does not match any
 * of the exclude filters. If no include filters are given all tests should be run as long as they
 * do not match any of the exclude filters.</p>
 *
 * <p>The format of the filters is defined by the runner, and could be structured as
 * &lt;package&gt;, &lt;package&gt;.&lt;class&gt;, &lt;package&gt;.&lt;class&gt;#&lt;method&gt; or
 * &lt;native_name&gt;. They can even be regexes.</p>
 */
public interface ITestFilterReceiver {

    /**
     * Adds a filter of which tests to include.
     */
    void addIncludeFilter(String filter);

    /**
     * Adds the {@link Set} of filters of which tests to include.
     */
    void addAllIncludeFilters(Set<String> filters);

    /**
     * Adds a filter of which tests to exclude.
     */
    void addExcludeFilter(String filter);

    /**
     * Adds the {@link Set} of filters of which tests to exclude.
     */
    void addAllExcludeFilters(Set<String> filters);
}
