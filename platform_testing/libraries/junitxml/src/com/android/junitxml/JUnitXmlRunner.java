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

import org.junit.internal.TextListener;
import org.junit.runner.JUnitCore;
import org.junit.runner.Result;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.stream.Stream;

/**
 * A JUnit runner that is intended to use as replacement of JUnitCore
 * which in addition to printing the test failures to stdout, will
 * write the results in XML format to the path specified in the env
 * variable XML_OUTPUT_FILE.
 *
 * <p>To use this runner:
 *     {@code TEST_WORKSPACE=[...]
 *            XML_OUTPUT_FILE=[...]
 *            java -cp junitxml.jar [...] \
 *            com.android.junitxml.JUnitXmlRunner [Test classes]}
 */
public class JUnitXmlRunner {

    private static XmlRunListener getRunListener() {
        String outputFile = System.getenv("XML_OUTPUT_FILE");
        String suiteName = System.getenv("TEST_WORKSPACE");
        if (outputFile != null && outputFile.length() > 0) {
            try {
                return new XmlRunListener(
                        new FileOutputStream(outputFile),
                        suiteName != null ? suiteName : "Unknown test suite");
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            }
        }
        return null;
    }

    public static void main(String... args) {
        JUnitCore core = new JUnitCore();
        try {
            TextListener textListener = new TextListener(System.out);
            core.addListener(textListener);
            XmlRunListener xmlListener = getRunListener();
            if (xmlListener != null) {
                core.addListener(xmlListener);
            }
            Result result = core.run(Stream.of(args)
                    .map(test -> {
                        try {
                            return Class.forName(test);
                        } catch (ClassNotFoundException e) {
                            throw new RuntimeException(e);
                        }
                    })
                    .toArray(Class[]::new));
            if (xmlListener != null) {
                xmlListener.endTestSuite();
            }
            System.exit(result.wasSuccessful() ? 0 : 1);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}

