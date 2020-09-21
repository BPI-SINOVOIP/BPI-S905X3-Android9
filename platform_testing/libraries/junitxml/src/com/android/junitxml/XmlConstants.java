/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.junitxml;

/**
 * It is an adaptation of the schema used by Ant,
 * org.apache.tools.ant.taskdefs.optional.junit.XMLJUnitResultFormatter.
 */
public interface XmlConstants {

    String ELEMENT_TESTSUITE = "testsuite";
    String ELEMENT_TESTCASE = "testcase";
    String ELEMENT_PROPERTIES = "properties";
    String ELEMENT_PROPERTY = "property";
    String ELEMENT_FAILURE = "failure";
    String ELEMENT_SKIPPED = "skipped";
    String ELEMENT_ERROR = "error";

    String ATTR_TESTSUITE_NAME = "name";
    String ATTR_TESTSUITE_TESTS = "tests";
    String ATTR_TESTSUITE_FAILURES = "failures";
    String ATTR_TESTSUITE_ERRORS = "errors";
    String ATTR_TESTSUITE_TIME = "time";
    String ATTR_TESTSUITE_SKIPPED = "skipped";
    String ATTR_TESTSUITE_HOSTNAME = "hostname";

    String ATTR_TESTCASE_NAME = "name";
    String ATTR_TESTCASE_CLASSNAME = "classname";
    String ATTR_TESTCASE_TIME = "time";

    String ATTR_PROPERTY_NAME = "name";
    String ATTR_PROPERTY_VALUE = "value";

    String ATTR_FAILURE_MESSAGE = "message";
    String ATTR_FAILURE_TYPE = "type";

    String ATTR_SKIPPED_MESSAGE = "message";
}

