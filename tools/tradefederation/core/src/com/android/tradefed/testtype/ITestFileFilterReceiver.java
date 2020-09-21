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

import java.io.File;

/**
 * A runner that can receive a file specifying which tests to run and/or not to run.
 *
 * <p>A test will be run if it is included in the testFile provided, and not excluded by the
 * notTestFile or by an exclude filter provided to the test runner.
 * Either file should contain a line separated list of test names, formatted as filters.</p>
 *
 * <p>The format of the filters is defined by the runner, and could be structured as
 * &lt;package&gt;, &lt;package&gt;.&lt;class&gt;, &lt;package&gt;.&lt;class&gt;#&lt;method&gt; or
 * &lt;native_name&gt;. They can even be regexes.</p>
 */
public interface ITestFileFilterReceiver {

    /**
     * Sets the test file of includes. Does not ensure that testFile exists or is a file.
     */
    void setIncludeTestFile(File testFile);

    /**
     * Sets the test file of excludes. Does not ensure that testFile exists or is a file.
     */
    void setExcludeTestFile(File testFile);
}
