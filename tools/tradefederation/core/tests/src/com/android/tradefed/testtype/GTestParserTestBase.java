/*
 * Copyright (C) 2010 The Android Open Source Project
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

import com.android.tradefed.log.LogUtil.CLog;

import junit.framework.TestCase;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Vector;


/**
 * Base test class for various GTest parsers
 */
public abstract class GTestParserTestBase extends TestCase {
    protected static final String TEST_TYPE_DIR = "testtype";
    protected static final String TEST_MODULE_NAME = "module";
    protected static final String GTEST_OUTPUT_FILE_1 = "gtest_output1.txt";
    protected static final String GTEST_OUTPUT_FILE_2 = "gtest_output2.txt";
    protected static final String GTEST_OUTPUT_FILE_3 = "gtest_output3.txt";
    protected static final String GTEST_OUTPUT_FILE_4 = "gtest_output4.txt";
    protected static final String GTEST_OUTPUT_FILE_5 = "gtest_output5.txt";
    protected static final String GTEST_OUTPUT_FILE_6 = "gtest_output6.txt";
    protected static final String GTEST_OUTPUT_FILE_7 = "gtest_output7.txt";
    protected static final String GTEST_OUTPUT_FILE_8 = "gtest_output8.txt";
    protected static final String GTEST_OUTPUT_FILE_9 = "gtest_output9.txt";
    protected static final String GTEST_LIST_FILE_1 = "gtest_list1.txt";
    protected static final String GTEST_LIST_FILE_2 = "gtest_list2.txt";
    protected static final String GTEST_LIST_FILE_3 = "gtest_list3.txt";
    protected static final String GTEST_LIST_FILE_4 = "gtest_list4.txt";

    /**
     * Helper to read a file from the res/testtype directory and return its contents as a String[]
     *
     * @param filename the name of the file (without the extension) in the res/testtype directory
     * @return a String[] of the
     */
    protected String[] readInFile(String filename) {
        Vector<String> fileContents = new Vector<String>();
        try {
            InputStream gtestResultStream1 = getClass().getResourceAsStream(File.separator +
                    TEST_TYPE_DIR + File.separator + filename);
            BufferedReader reader = new BufferedReader(new InputStreamReader(gtestResultStream1));
            String line = null;
            while ((line = reader.readLine()) != null) {
                fileContents.add(line);
            }
        }
        catch (NullPointerException e) {
            CLog.e("Gest output file does not exist: " + filename);
        }
        catch (IOException e) {
            CLog.e("Unable to read contents of gtest output file: " + filename);
        }
        return fileContents.toArray(new String[fileContents.size()]);
    }
}
