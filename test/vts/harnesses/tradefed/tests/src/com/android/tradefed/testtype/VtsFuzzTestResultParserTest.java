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

import com.android.ddmlib.Log;
import com.android.tradefed.result.ITestLifeCycleReceiver;
import com.android.tradefed.result.TestDescription;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Map;
import java.util.Vector;


/**
 * Unit tests for {@link VtsFuzzTestResultParserTest}.
 */
public class VtsFuzzTestResultParserTest extends TestCase {
    private static final String TEST_TYPE_DIR = "testtype";
    private static final String TEST_MODULE_NAME = "module";
    private static final String VTS_FUZZ_OUTPUT_FILE_1 = "vts_fuzz_output1.txt";
    private static final String VTS_FUZZ_OUTPUT_FILE_2 = "vts_fuzz_output2.txt";
    private static final String LOG_TAG = "VtsFuzzTestResultParserTest";

    /**
     * Helper to read a file from the res/testtype directory and return its contents as a String[]
     *
     * @param filename the name of the file (without the extension) in the res/testtype directory
     * @return a String[] of the
     */
    private String[] readInFile(String filename) {
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
            Log.e(LOG_TAG, "Output file does not exist: " + filename);
        }
        catch (IOException e) {
            Log.e(LOG_TAG, "Unable to read contents of an output file: " + filename);
        }
        return fileContents.toArray(new String[fileContents.size()]);
    }

    /**
     * Tests the parser for a passed test run output with 1 test.
     */
    @SuppressWarnings("unchecked")
    public void testParsePassedFile() throws Exception {
        String[] contents = readInFile(VTS_FUZZ_OUTPUT_FILE_1);
        ITestLifeCycleReceiver mockRunListener = EasyMock.createMock(ITestLifeCycleReceiver.class);
        mockRunListener.testRunStarted(TEST_MODULE_NAME, 1);
        mockRunListener.testEnded(
                (TestDescription) EasyMock.anyObject(), (Map<String, String>) EasyMock.anyObject());
        EasyMock.replay(mockRunListener);
        VtsFuzzTestResultParser resultParser =
                new VtsFuzzTestResultParser(TEST_MODULE_NAME, mockRunListener);
        resultParser.processNewLines(contents);
        resultParser.done();
    }

    /**
     * Tests the parser for a failed test run output with 1 test.
     */
    public void testParseFailedFile() throws Exception {
        String[] contents =  readInFile(VTS_FUZZ_OUTPUT_FILE_2);
        ITestLifeCycleReceiver mockRunListener = EasyMock.createMock(ITestLifeCycleReceiver.class);
        mockRunListener.testRunStarted(TEST_MODULE_NAME, 1);
        mockRunListener.testFailed(
                (TestDescription) EasyMock.anyObject(), (String) EasyMock.anyObject());
        mockRunListener.testEnded(
                (TestDescription) EasyMock.anyObject(), (Map<String, String>) EasyMock.anyObject());
        mockRunListener.testRunFailed((String)EasyMock.anyObject());
        EasyMock.replay(mockRunListener);
        VtsFuzzTestResultParser resultParser = new VtsFuzzTestResultParser(
            TEST_MODULE_NAME, mockRunListener);
        resultParser.processNewLines(contents);
        resultParser.done();
    }
}
