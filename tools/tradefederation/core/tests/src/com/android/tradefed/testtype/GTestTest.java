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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.ddmlib.FileListingService;
import com.android.ddmlib.IShellOutputReceiver;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.MockFileUtil;
import com.android.tradefed.result.ITestInvocationListener;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.util.concurrent.TimeUnit;


/** Unit tests for {@link GTest}. */
@RunWith(JUnit4.class)
public class GTestTest {
    private static final String GTEST_FLAG_FILTER = "--gtest_filter";
    private ITestInvocationListener mMockInvocationListener = null;
    private IShellOutputReceiver mMockReceiver = null;
    private ITestDevice mMockITestDevice = null;
    private GTest mGTest;
    private OptionSetter mSetter;

    /** Helper to initialize the various EasyMocks we'll need. */
    @Before
    public void setUp() throws Exception {
        mMockInvocationListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockReceiver = EasyMock.createMock(IShellOutputReceiver.class);
        mMockITestDevice = EasyMock.createMock(ITestDevice.class);
        mMockReceiver.flush();
        EasyMock.expectLastCall().anyTimes();
        EasyMock.expect(mMockITestDevice.getSerialNumber()).andStubReturn("serial");
        mGTest =
                new GTest() {
                    @Override
                    IShellOutputReceiver createResultParser(
                            String runName, ITestInvocationListener listener) {
                        return mMockReceiver;
                    }

                    @Override
                    GTestXmlResultParser createXmlParser(
                            String testRunName, ITestInvocationListener listener) {
                        return new GTestXmlResultParser(testRunName, listener) {
                            @Override
                            public void parseResult(File f, CollectingOutputReceiver output) {
                                return;
                            }
                        };
                    }
                };
        mGTest.setDevice(mMockITestDevice);
        mSetter = new OptionSetter(mGTest);
    }

    /**
     * Helper that replays all mocks.
     */
    private void replayMocks() {
      EasyMock.replay(mMockInvocationListener, mMockITestDevice, mMockReceiver);
    }

    /**
     * Helper that verifies all mocks.
     */
    private void verifyMocks() {
      EasyMock.verify(mMockInvocationListener, mMockITestDevice, mMockReceiver);
    }

    /** Test run when the test dir is not found on the device. */
    @Test
    public void testRun_noTestDir() throws DeviceNotAvailableException {
        EasyMock.expect(mMockITestDevice.doesFileExist(GTest.DEFAULT_NATIVETEST_PATH))
                .andReturn(false);
        replayMocks();
        mGTest.run(mMockInvocationListener);
        verifyMocks();
    }

    /** Test run when no device is set should throw an exception. */
    @Test
    public void testRun_noDevice() throws DeviceNotAvailableException {
        mGTest.setDevice(null);
        replayMocks();
        try {
            mGTest.run(mMockInvocationListener);
            fail("an exception should have been thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
        verifyMocks();
    }

    /** Test the run method for a couple tests */
    @Test
    public void testRun() throws DeviceNotAvailableException {
        final String nativeTestPath = GTest.DEFAULT_NATIVETEST_PATH;
        final String test1 = "test1";
        final String test2 = "test2";
        final String testPath1 = String.format("%s/%s", nativeTestPath, test1);
        final String testPath2 = String.format("%s/%s", nativeTestPath, test2);

        MockFileUtil.setMockDirContents(mMockITestDevice, nativeTestPath, test1, test2);
        EasyMock.expect(mMockITestDevice.doesFileExist(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath + "/test1")).andReturn(false);
        // report the file as executable
        EasyMock.expect(mMockITestDevice.executeShellCommand("ls -l " + testPath1))
                .andReturn("-rwxr-xr-x 1 root shell 1000 2009-01-01 00:00 " + testPath1);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath + "/test2")).andReturn(false);
        EasyMock.expect(mMockITestDevice.executeShellCommand("ls -l " + testPath2))
                .andReturn("-rwxr-xr-x 1 root shell 1000 2009-01-01 00:00 " + testPath2);
        String[] files = new String[] {"test1", "test2"};
        EasyMock.expect(mMockITestDevice.getChildren(nativeTestPath)).andReturn(files);
        mMockITestDevice.executeShellCommand(EasyMock.contains(test1),
                EasyMock.same(mMockReceiver), EasyMock.anyLong(),
                (TimeUnit)EasyMock.anyObject(), EasyMock.anyInt());
        mMockITestDevice.executeShellCommand(EasyMock.contains(test2),
                EasyMock.same(mMockReceiver), EasyMock.anyLong(),
                (TimeUnit)EasyMock.anyObject(), EasyMock.anyInt());

        replayMocks();

        mGTest.run(mMockInvocationListener);
        verifyMocks();
    }

    /** Test the run method when module name is specified */
    @Test
    public void testRun_moduleName() throws DeviceNotAvailableException {
        final String module = "test1";
        final String modulePath = String.format("%s%s%s",
                GTest.DEFAULT_NATIVETEST_PATH, FileListingService.FILE_SEPARATOR, module);
        MockFileUtil.setMockDirContents(mMockITestDevice, modulePath, new String[] {});

        mGTest.setModuleName(module);

        EasyMock.expect(mMockITestDevice.doesFileExist(modulePath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(modulePath)).andReturn(false);
        mMockITestDevice.executeShellCommand(EasyMock.contains(modulePath),
                EasyMock.same(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(), EasyMock.anyInt());
        // report the file as executable
        EasyMock.expect(mMockITestDevice.executeShellCommand("ls -l " + modulePath))
                .andReturn("-rwxr-xr-x 1 root shell 1000 2009-01-01 00:00 " + modulePath);

        replayMocks();

        mGTest.run(mMockInvocationListener);
        verifyMocks();
    }

    /** Test the run method for a test in a subdirectory */
    @Test
    public void testRun_nested() throws DeviceNotAvailableException {
        final String nativeTestPath = GTest.DEFAULT_NATIVETEST_PATH;
        final String subFolderName = "subFolder";
        final String test1 = "test1";
        final String test1Path = String.format("%s%s%s%s%s", nativeTestPath,
                FileListingService.FILE_SEPARATOR,
                subFolderName,
                FileListingService.FILE_SEPARATOR, test1);
        MockFileUtil.setMockDirPath(mMockITestDevice, nativeTestPath, subFolderName, test1);
        EasyMock.expect(mMockITestDevice.doesFileExist(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath + "/" + subFolderName))
                .andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(test1Path)).andReturn(false);
        // report the file as executable
        EasyMock.expect(mMockITestDevice.executeShellCommand("ls -l " + test1Path))
                .andReturn("-rwxr-xr-x 1 root shell 1000 2009-01-01 00:00 " + test1Path);
        String[] files = new String[] {subFolderName};
        EasyMock.expect(mMockITestDevice.getChildren(nativeTestPath)).andReturn(files);
        String[] files2 = new String[] {"test1"};
        EasyMock.expect(mMockITestDevice.getChildren(nativeTestPath + "/" + subFolderName))
                .andReturn(files2);
        mMockITestDevice.executeShellCommand(EasyMock.contains(test1Path),
                EasyMock.same(mMockReceiver),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(), EasyMock.anyInt());

        replayMocks();

        mGTest.run(mMockInvocationListener);
        verifyMocks();
    }

    /**
     * Helper function to do the actual filtering test.
     *
     * @param filterString The string to search for in the Mock, to verify filtering was called
     * @throws DeviceNotAvailableException
     */
    private void doTestFilter(String filterString) throws DeviceNotAvailableException {
        String nativeTestPath = GTest.DEFAULT_NATIVETEST_PATH;
        String testPath = nativeTestPath + "/test1";
        // configure the mock file system to have a single test
        MockFileUtil.setMockDirContents(mMockITestDevice, nativeTestPath, "test1");
        EasyMock.expect(mMockITestDevice.doesFileExist(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(testPath)).andReturn(false);
        // report the file as executable
        EasyMock.expect(mMockITestDevice.executeShellCommand("ls -l " + testPath))
                .andReturn("-rwxr-xr-x 1 root shell 1000 2009-01-01 00:00 " + testPath);
        String[] files = new String[] {"test1"};
        EasyMock.expect(mMockITestDevice.getChildren(nativeTestPath)).andReturn(files);
        mMockITestDevice.executeShellCommand(EasyMock.contains(filterString),
                EasyMock.same(mMockReceiver), EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(),
                EasyMock.anyInt());
        replayMocks();
        mGTest.run(mMockInvocationListener);

        verifyMocks();
    }

    /** Test the include filtering of test methods. */
    @Test
    public void testIncludeFilter() throws DeviceNotAvailableException {
        String includeFilter1 = "abc";
        String includeFilter2 = "def";
        mGTest.addIncludeFilter(includeFilter1);
        mGTest.addIncludeFilter(includeFilter2);
        doTestFilter(String.format("%s=%s:%s", GTEST_FLAG_FILTER, includeFilter1, includeFilter2));
    }

    /** Test the exclude filtering of test methods. */
    @Test
    public void testExcludeFilter() throws DeviceNotAvailableException {
        String excludeFilter1 = "*don?tRunMe*";
        mGTest.addExcludeFilter(excludeFilter1);

        doTestFilter(String.format(
                "%s=-%s", GTEST_FLAG_FILTER, excludeFilter1));
    }

    /** Test simultaneous include and exclude filtering of test methods. */
    @Test
    public void testIncludeAndExcludeFilters() throws DeviceNotAvailableException {
        String includeFilter1 = "pleaseRunMe";
        String includeFilter2 = "andMe";
        String excludeFilter1 = "dontRunMe";
        String excludeFilter2 = "orMe";
        mGTest.addIncludeFilter(includeFilter1);
        mGTest.addExcludeFilter(excludeFilter1);
        mGTest.addIncludeFilter(includeFilter2);
        mGTest.addExcludeFilter(excludeFilter2);

        doTestFilter(String.format("%s=%s:%s-%s:%s", GTEST_FLAG_FILTER,
              includeFilter1, includeFilter2, excludeFilter1, excludeFilter2));
    }

    /** Test behavior for command lines too long to be run by ADB */
    @Test
    public void testCommandTooLong() throws DeviceNotAvailableException {
        String deviceScriptPath = "/data/local/tmp/gtest_script.sh";
        StringBuilder filterString = new StringBuilder(GTEST_FLAG_FILTER);
        filterString.append("=-");
        for (int i = 0; i < 100; i++) {
            if (i != 0) {
                filterString.append(":");
            }
            String filter = String.format("ExcludeClass%d", i);
            filterString.append(filter);
            mGTest.addExcludeFilter(filter);
        }
        // filter string will be longer than GTest.GTEST_CMD_CHAR_LIMIT

        String nativeTestPath = GTest.DEFAULT_NATIVETEST_PATH;
        String testPath = nativeTestPath + "/test1";
        // configure the mock file system to have a single test
        MockFileUtil.setMockDirContents(mMockITestDevice, nativeTestPath, "test1");
        EasyMock.expect(mMockITestDevice.doesFileExist(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(testPath)).andReturn(false);
        // report the file as executable
        EasyMock.expect(mMockITestDevice.executeShellCommand("ls -l " + testPath))
                .andReturn("-rwxr-xr-x 1 root shell 1000 2009-01-01 00:00 " + testPath);
        String[] files = new String[] {"test1"};
        EasyMock.expect(mMockITestDevice.getChildren(nativeTestPath)).andReturn(files);
        // Expect push of script file
        EasyMock.expect(mMockITestDevice.pushString(EasyMock.<String>anyObject(),
                EasyMock.eq(deviceScriptPath))).andReturn(Boolean.TRUE);
        // chmod 755 for the shell script
        EasyMock.expect(mMockITestDevice.executeShellCommand(EasyMock.contains("chmod")))
                .andReturn("")
                .times(1);
        // Expect command to run shell script, rather than direct adb command
        mMockITestDevice.executeShellCommand(EasyMock.eq(String.format("sh %s", deviceScriptPath)),
                EasyMock.same(mMockReceiver), EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(),
                EasyMock.anyInt());
        // Expect deletion of file on device
        EasyMock.expect(mMockITestDevice.executeShellCommand(
                EasyMock.eq(String.format("rm %s", deviceScriptPath)))).andReturn("");
        replayMocks();
        mGTest.run(mMockInvocationListener);

        verifyMocks();
    }

    /** Empty file exclusion regex filter should not skip any files */
    @Test
    public void testFileExclusionRegexFilter_emptyfilters() throws Exception {
        // report /test_file as executable
        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mockDevice.executeShellCommand("ls -l /test_file"))
                .andReturn("-rwxr-xr-x 1 root shell 7 2009-01-01 00:00 /test_file");
        EasyMock.replay(mockDevice);
        mGTest.setDevice(mockDevice);
        assertFalse(mGTest.shouldSkipFile("/test_file"));
        EasyMock.verify(mockDevice);
    }

    /** File exclusion regex filter should skip invalid filepath. */
    @Test
    public void testFileExclusionRegexFilter_invalidInputString() throws Exception {
        assertTrue(mGTest.shouldSkipFile(null));
        assertTrue(mGTest.shouldSkipFile(""));
    }

    /** File exclusion regex filter should skip matched filepaths. */
    @Test
    public void testFileExclusionRegexFilter_skipMatched() throws Exception {
        // report all files as executable
        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mockDevice.executeShellCommand("ls -l /some/path/file/run_me"))
                .andReturn("-rwxr-xr-x 1 root shell 7 2009-01-01 00:00 /some/path/file/run_me");
        EasyMock.expect(mockDevice.executeShellCommand("ls -l /some/path/file/run_me2"))
                .andReturn("-rwxr-xr-x 1 root shell 7 2009-01-01 00:00 /some/path/file/run_me2");
        EasyMock.expect(mockDevice.executeShellCommand("ls -l /some/path/file/run_me.not"))
                .andReturn("-rwxr-xr-x 1 root shell 7 2009-01-01 00:00 /some/path/file/run_me.not");
        EasyMock.replay(mockDevice);
        mGTest.setDevice(mockDevice);
        // Skip files ending in .not
        mGTest.addFileExclusionFilterRegex(".*\\.not");
        assertFalse(mGTest.shouldSkipFile("/some/path/file/run_me"));
        assertFalse(mGTest.shouldSkipFile("/some/path/file/run_me2"));
        assertTrue(mGTest.shouldSkipFile("/some/path/file/run_me.not"));
        EasyMock.verify(mockDevice);
    }

    /** File exclusion regex filter for multi filters. */
    @Test
    public void testFileExclusionRegexFilter_skipMultiMatched() throws Exception {
        // report all files as executable
        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mockDevice.executeShellCommand("ls -l /some/path/file/run_me"))
                .andReturn("-rwxr-xr-x 1 root shell 7 2009-01-01 00:00 /some/path/file/run_me");
        EasyMock.expect(mockDevice.executeShellCommand("ls -l /some/path/file/run_me.not"))
                .andReturn("-rwxr-xr-x 1 root shell 7 2009-01-01 00:00 /some/path/file/run_me.not");
        EasyMock.expect(mockDevice.executeShellCommand("ls -l /some/path/file/run_me.not2"))
                .andReturn(
                        "-rwxr-xr-x 1 root shell 7 2009-01-01 00:00 /some/path/file/run_me.not2");
        EasyMock.replay(mockDevice);
        mGTest.setDevice(mockDevice);
        // Skip files ending in .not
        mGTest.addFileExclusionFilterRegex(".*\\.not");
        // Also skip files ending in .not2
        mGTest.addFileExclusionFilterRegex(".*\\.not2");
        assertFalse(mGTest.shouldSkipFile("/some/path/file/run_me"));
        assertTrue(mGTest.shouldSkipFile("/some/path/file/run_me.not"));
        assertTrue(mGTest.shouldSkipFile("/some/path/file/run_me.not2"));
    }

    /** Test the run method for a couple tests */
    @Test
    public void testRunXml() throws Exception {
        mSetter.setOptionValue("xml-output", "true");

        final String nativeTestPath = GTest.DEFAULT_NATIVETEST_PATH;
        final String test1 = "test1";
        final String test2 = "test2";
        final String testPath1 = String.format("%s/%s", nativeTestPath, test1);
        final String testPath2 = String.format("%s/%s", nativeTestPath, test2);

        MockFileUtil.setMockDirContents(mMockITestDevice, nativeTestPath, test1, test2);
        EasyMock.expect(mMockITestDevice.doesFileExist(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(nativeTestPath)).andReturn(true);
        EasyMock.expect(mMockITestDevice.isDirectory(testPath1)).andReturn(false);
        EasyMock.expect(mMockITestDevice.executeShellCommand("ls -l " + testPath1))
                .andReturn("-rwxr-xr-x 1 root shell 7 2009-01-01 00:00 " + testPath1);
        EasyMock.expect(mMockITestDevice.isDirectory(testPath2)).andReturn(false);
        EasyMock.expect(mMockITestDevice.executeShellCommand("ls -l " + testPath2))
                .andReturn("-rwxr-xr-x 1 root shell 7 2009-01-01 00:00 " + testPath2);
        String[] files = new String[] {"test1", "test2"};
        EasyMock.expect(mMockITestDevice.getChildren(nativeTestPath)).andReturn(files);
        EasyMock.expect(mMockITestDevice.executeShellCommand(EasyMock.contains("rm")))
                .andReturn("")
                .times(2);
        EasyMock.expect(mMockITestDevice.pullFile((String)EasyMock.anyObject(),
                (File)EasyMock.anyObject())).andStubReturn(true);
        mMockITestDevice.executeShellCommand(EasyMock.contains(test1),
                (CollectingOutputReceiver) EasyMock.anyObject(),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(), EasyMock.anyInt());
        mMockITestDevice.executeShellCommand(EasyMock.contains(test2),
                (CollectingOutputReceiver) EasyMock.anyObject(),
                EasyMock.anyLong(), (TimeUnit)EasyMock.anyObject(), EasyMock.anyInt());
        replayMocks();

        mGTest.run(mMockInvocationListener);
        verifyMocks();
    }

    @Test
    public void testGetFileName() {
        String expected = "bar";
        String s1 = "/foo/" + expected;
        String s2 = expected;
        String s3 = "/foo/";
        assertEquals(expected, mGTest.getFileName(s1));
        assertEquals(expected, mGTest.getFileName(s2));
        try {
            mGTest.getFileName(s3);
            fail("Expected IllegalArgumentException not thrown");
        } catch (IllegalArgumentException iae) {
            // expected
        }
    }

    /** Test the include filtering by file of test methods. */
    @Test
    public void testFileFilter() throws Exception {
        String fileFilter = "presubmit";
        mSetter.setOptionValue("test-filter-key", fileFilter);
        String expectedFilterFile = String.format("%s/test1%s",
                GTest.DEFAULT_NATIVETEST_PATH, GTest.FILTER_EXTENSION);
        String fakeContent = "{\n" +
                             "    \"presubmit\": {\n" +
                             "        \"filter\": \"Foo1.*:Foo2.*\"\n" +
                             "    },\n" +
                             "    \"continuous\": {\n" +
                             "        \"filter\": \"Foo1.*:Foo2.*:Bar.*\"\n" +
                             "    }\n" +
                             "}\n";
        EasyMock.expect(mMockITestDevice.doesFileExist(expectedFilterFile)).andReturn(true);
        EasyMock.expect(mMockITestDevice.executeShellCommand("cat \"" + expectedFilterFile + "\""))
                .andReturn(fakeContent);
        doTestFilter(String.format("%s=%s", GTEST_FLAG_FILTER, "Foo1.*:Foo2.*"));
    }

    /**
     * Test the include filtering by providing a non existing filter. No filter will be applied in
     * this case.
     */
    @Test
    public void testFileFilter_notfound() throws Exception {
        String fileFilter = "garbage";
        mSetter.setOptionValue("test-filter-key", fileFilter);
        String expectedFilterFile = String.format("%s/test1%s",
                GTest.DEFAULT_NATIVETEST_PATH, GTest.FILTER_EXTENSION);
        String fakeContent = "{\n" +
                             "    \"presubmit\": {\n" +
                             "        \"filter\": \"Foo1.*:Foo2.*\"\n" +
                             "    },\n" +
                             "    \"continuous\": {\n" +
                             "        \"filter\": \"Foo1.*:Foo2.*:Bar.*\"\n" +
                             "    }\n" +
                             "}\n";
        EasyMock.expect(mMockITestDevice.doesFileExist(expectedFilterFile)).andReturn(true);
        EasyMock.expect(mMockITestDevice.executeShellCommand("cat \"" + expectedFilterFile + "\""))
                .andReturn(fakeContent);
        doTestFilter("");
    }

    /** Test {@link GTest#getGTestCmdLine(String, String)} with default options. */
    @Test
    public void testGetGTestCmdLine_defaults() {
        String cmd_line = mGTest.getGTestCmdLine("test_path", "flags");
        assertEquals("test_path flags", cmd_line);
    }

    /** Test {@link GTest#getGTestCmdLine(String, String)} with non-default user. */
    @Test
    public void testGetGTestCmdLine_runAs() throws Exception {
        mSetter.setOptionValue("run-test-as", "shell");

        String cmd_line = mGTest.getGTestCmdLine("test_path", "flags");
        assertEquals("su shell test_path flags", cmd_line);
    }

    /** Test GTest command line string for sharded tests. */
    @Test
    public void testGetGTestCmdLine_testShard() {
        mGTest.setShardIndex(1);
        mGTest.setShardCount(3);

        String cmd_line = mGTest.getGTestCmdLine("test_path", "flags");
        assertEquals("GTEST_SHARD_INDEX=1 GTEST_TOTAL_SHARDS=3 test_path flags", cmd_line);
    }

    /**
     * Verifies that {@link GTest#isDeviceFileExecutable(String)} recognizes regular executable file
     *
     * @throws Exception
     */
    @Test
    public void testIsDeviceFileExecutable_executable_rwx() throws Exception {
        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mockDevice.executeShellCommand("ls -l /system/bin/ping"))
                .andReturn("-rwxr-xr-x 1 root shell 42824 2009-01-01 00:00 /system/bin/ping");
        EasyMock.replay(mockDevice);
        mGTest.setDevice(mockDevice);
        assertTrue(mGTest.isDeviceFileExecutable("/system/bin/ping"));
        EasyMock.verify(mockDevice);
    }

    /**
     * Verifies that {@link GTest#isDeviceFileExecutable(String)} recognizes symlink'd executable
     * file
     *
     * @throws Exception
     */
    @Test
    public void testIsDeviceFileExecutable_executable_lrwx() throws Exception {
        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mockDevice.executeShellCommand("ls -l /system/bin/start"))
                .andReturn(
                        "lrwxr-xr-x 1 root shell 7 2009-01-01 00:00 /system/bin/start -> toolbox");
        EasyMock.replay(mockDevice);
        mGTest.setDevice(mockDevice);
        assertTrue(mGTest.isDeviceFileExecutable("/system/bin/start"));
        EasyMock.verify(mockDevice);
    }

    /**
     * Verifies that {@link GTest#isDeviceFileExecutable(String)} recognizes non-executable file
     *
     * @throws Exception
     */
    @Test
    public void testIsDeviceFileExecutable_notExecutable() throws Exception {
        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mockDevice.executeShellCommand("ls -l /system/build.prop"))
                .andReturn("-rw-r--r-- 1 root root 5020 2009-01-01 00:00 /system/build.prop");
        EasyMock.replay(mockDevice);
        mGTest.setDevice(mockDevice);
        assertFalse(mGTest.isDeviceFileExecutable("/system/build.prop"));
        EasyMock.verify(mockDevice);
    }

    /**
     * Verifies that {@link GTest#isDeviceFileExecutable(String)} recognizes a directory listing
     *
     * @throws Exception
     */
    @Test
    public void testIsDeviceFileExecutable_directory() throws Exception {
        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mockDevice.executeShellCommand("ls -l /system"))
                .andReturn(
                        "total 416\n"
                                + "drwxr-xr-x 74 root root    4096 2009-01-01 00:00 app\n"
                                + "drwxr-xr-x  3 root shell   8192 2009-01-01 00:00 bin\n"
                                + "-rw-r--r--  1 root root    5020 2009-01-01 00:00 build.prop\n"
                                + "drwxr-xr-x 15 root root    4096 2009-01-01 00:00 etc\n"
                                + "drwxr-xr-x  2 root root    4096 2009-01-01 00:00 fake-libs\n"
                                + "drwxr-xr-x  2 root root    8192 2009-01-01 00:00 fonts\n"
                                + "drwxr-xr-x  4 root root    4096 2009-01-01 00:00 framework\n"
                                + "drwxr-xr-x  6 root root    8192 2009-01-01 00:00 lib\n"
                                + "drwx------  2 root root    4096 1970-01-01 00:00 lost+found\n"
                                + "drwxr-xr-x  3 root root    4096 2009-01-01 00:00 media\n"
                                + "drwxr-xr-x 68 root root    4096 2009-01-01 00:00 priv-app\n"
                                + "-rw-r--r--  1 root root  137093 2009-01-01 00:00 recovery-from-boot.p\n"
                                + "drwxr-xr-x  9 root root    4096 2009-01-01 00:00 usr\n"
                                + "drwxr-xr-x  8 root shell   4096 2009-01-01 00:00 vendor\n"
                                + "drwxr-xr-x  2 root shell   4096 2009-01-01 00:00 xbin\n");
        EasyMock.replay(mockDevice);
        mGTest.setDevice(mockDevice);
        assertFalse(mGTest.isDeviceFileExecutable("/system"));
        EasyMock.verify(mockDevice);
    }
}
