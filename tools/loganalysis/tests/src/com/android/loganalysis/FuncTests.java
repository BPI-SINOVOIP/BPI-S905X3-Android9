/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.loganalysis;

import com.android.loganalysis.parser.BugreportParserFuncTest;
import com.android.loganalysis.parser.LogcatParserFuncTest;
import com.android.loganalysis.parser.MonkeyLogParserFuncTest;

import junit.framework.Test;
import junit.framework.TestSuite;

/**
 * A test suite for all log analysis functional tests.
 */
public class FuncTests extends TestSuite {

    public FuncTests() {
        super();

        addTestSuite(BugreportParserFuncTest.class);
        addTestSuite(LogcatParserFuncTest.class);
        addTestSuite(MonkeyLogParserFuncTest.class);
    }

    public static Test suite() {
        return new FuncTests();
    }
}
