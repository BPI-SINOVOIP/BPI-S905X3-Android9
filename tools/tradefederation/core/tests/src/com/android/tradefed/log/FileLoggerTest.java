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
package com.android.tradefed.log;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.util.StreamUtil;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

/** Unit tests for {@link FileLogger}. */
@RunWith(JUnit4.class)
public class FileLoggerTest {

    private static final String LOG_TAG = "FileLoggerTest";

    /**
     * Test logging to a logger.
     *
     * @throws ConfigurationException if unable to create log file
     * @throws IOException if unable to read from the file
     * @throws SecurityException if unable to delete the log file on cleanup
     */
    @Test
    public void testLogToLogger() throws ConfigurationException, IOException, SecurityException {
        String Text1 = "The quick brown fox jumps over the lazy doggie.";
        String Text2 = "Betty Botter bought some butter, 'But,' she said, 'this butter's bitter.'";
        String Text3 = "Wolf zombies quickly spot the jinxed grave.";
        BufferedReader logFileReader = null;
        FileLogger logger = new FileLogger();
        InputStreamSource logSource = null;

        try {
            logger.init();
            // Write 3 lines of text to the log...
            logger.printLog(LogLevel.INFO, LOG_TAG, Text1);
            String expectedText1 = LogUtil.getLogFormatString(LogLevel.INFO, LOG_TAG, Text1).trim();
            logger.printLog(LogLevel.VERBOSE, LOG_TAG, Text2);
            String expectedText2 =
                    LogUtil.getLogFormatString(LogLevel.VERBOSE, LOG_TAG, Text2).trim();
            logger.printLog(LogLevel.ASSERT, LOG_TAG, Text3);
            String expectedText3 =
                    LogUtil.getLogFormatString(LogLevel.ASSERT, LOG_TAG, Text3).trim();
            // Verify the 3 lines we logged
            logSource = logger.getLog();
            logFileReader = new BufferedReader(new InputStreamReader(
                    logSource.createInputStream()));

            String actualLogString = logFileReader.readLine().trim();
            assertEquals(trimTimestamp(expectedText1), trimTimestamp(actualLogString));

            actualLogString = logFileReader.readLine().trim();
            assertEquals(trimTimestamp(expectedText2), trimTimestamp(actualLogString));

            actualLogString = logFileReader.readLine().trim();
            assertEquals(trimTimestamp(expectedText3), trimTimestamp(actualLogString));
        }
        finally {
            StreamUtil.close(logFileReader);
            StreamUtil.cancel(logSource);
            logger.closeLog();
        }
    }

    /**
     * Remove the timestamp at the beginning of the log message.
     *
     * @param message log message with leading timestamp.
     * @return a {@link String} of message without leading timestamp.
     */
    private String trimTimestamp(String message) {
        // The log level character is prefixed to the log tag. For example:
        // 04-11 10:30:[50] I/FileLoggerTest: The quick brown fox jumps.
        int startIndex = message.indexOf(LOG_TAG) - 2;
        return message.substring(startIndex);
    }

    /**
     * Test behavior when {@link FileLogger#getLog()} is called after {@link FileLogger#closeLog()}.
     */
    @Test
    public void testGetLog_afterClose() throws Exception {
        FileLogger logger = new FileLogger();
        logger.init();
        logger.closeLog();
        // expect this to be silently handled
        logger.getLog();
    }

    /**
     * Test that no unexpected Exceptions occur if {@link FileLogger#printLog(LogLevel, String,
     * String)} is called after {@link FileLogger#closeLog()}
     */
    @Test
    public void testCloseLog() throws Exception {
        FileLogger logger = new FileLogger();
        logger.init();
        // test the package-private methods to capture any exceptions that occur
        logger.doCloseLog();
        logger.writeToLog("test2");
    }

    /**
     * Test behavior when {@link FileLogger#getLog()} is called when {@link FileLogger#init()} has
     * not been called.
     */
    @Test
    public void testGetLog_NoInit() {
        FileLogger logger = new FileLogger();
        // expect this to be silently handled
        logger.getLog();
    }

    /**
     * Test that the {@link FileLogger#clone()} properly copy all the options from the original
     * object.
     */
    @Test
    public void testClone() throws Exception {
        FileLogger logger = new FileLogger();
        OptionSetter setter = new OptionSetter(logger);
        setter.setOptionValue("max-log-size", "500");
        setter.setOptionValue("log-level", "INFO");
        FileLogger clone = (FileLogger) logger.clone();
        assertEquals(LogLevel.INFO, clone.getLogLevel());
        // We now have 2 distinct objects
        assertNotEquals(logger, clone);
        assertEquals(logger.getMaxLogSizeMbytes(), clone.getMaxLogSizeMbytes());
    }
}
