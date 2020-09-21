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

package vogar.target.junit3;

import junit.framework.Test;
import junit.framework.TestSuite;

/**
 * A suite that runs long running tests that are used to verify behavior of timeout.
 */
public class LongSuite {

    public static Test suite() {
        TestSuite testSuite = new TestSuite(LongSuite.class.getName());
        testSuite.addTestSuite(LongTest.class);
        testSuite.addTestSuite(LongTest2.class);
        return testSuite;
    }
}
