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

import junit.framework.TestCase;

import java.io.ByteArrayOutputStream;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Unit test for {@link TableFormatter}.
 */
public class TableFormatterTest extends TestCase {

    public void testDisplayTable() {
        List<List<String>> testData = new ArrayList<List<String>>();

        testData.add(Arrays.asList("1234", "12", "1234", "1"));
        testData.add(Arrays.asList("12", "1234", "1", "11"));
        final String expectedOutput = "1234  12    1234  1   \n" +
                                      "12    1234  1     11  \n";

        String actualOutput = doDisplayTable(testData);
        assertEquals(expectedOutput, actualOutput);
    }

    /**
     * Test displaying a table with different sized rows
     */
    public void testDisplayTable_missized() {
        List<List<String>> testData = new ArrayList<List<String>>();

        testData.add(Arrays.asList("1234", "12", "1234"));
        testData.add(Arrays.asList("12", "1234"));
        final String expectedOutput = "1234  12    1234  \n" +
                                      "12    1234  \n";

        String actualOutput = doDisplayTable(testData);
        assertEquals(expectedOutput, actualOutput);
    }

    /**
     * Call {@link TableFormatter#displayTable(List, PrintWriter)} using given table and return
     * result as a {@link String}
     */
    private String doDisplayTable(List<List<String>> testData) {
        ByteArrayOutputStream outStream = new ByteArrayOutputStream();
        PrintWriter writer = new PrintWriter(outStream);
        new TableFormatter().displayTable(testData, writer);
        writer.flush();
        writer.close();
        String actualOutput = outStream.toString();
        return actualOutput;
    }

}
