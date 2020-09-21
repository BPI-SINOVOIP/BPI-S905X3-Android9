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
package com.android.tradefed.result;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.ArrayUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.io.File;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Unit tests for {@link DeviceFileReporter}
 */
public class DeviceFileReporterTest extends TestCase {
    DeviceFileReporter dfr = null;
    ITestDevice mDevice = null;
    ITestInvocationListener mListener = null;

    // Used to control what ISS is returned
    InputStreamSource mDfrIss = null;

    @SuppressWarnings("serial")
    private static class FakeFile extends File {
        private final String mName;
        private final long mSize;

        FakeFile(String name, long size) {
            super(name);
            mName = name;
            mSize = size;
        }
        @Override
        public String toString() {
            return mName;
        }
        @Override
        public long length() {
            return mSize;
        }
    }

    @Override
    public void setUp() throws Exception {
        mDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mDevice.getSerialNumber()).andStubReturn("serial");

        mListener = EasyMock.createMock(ITestInvocationListener.class);
        dfr =
                new DeviceFileReporter(mDevice, mListener) {
                    @Override
                    InputStreamSource createIssForFile(File file) {
                        return mDfrIss;
                    }
                };
    }

    public void testSimple() throws Exception {
        final String result = "/data/tombstones/tombstone_00\r\n";
        final String filename = "/data/tombstones/tombstone_00";
        final String tombstone = "What do you want on your tombstone?";
        dfr.addPatterns("/data/tombstones/*");

        EasyMock.expect(mDevice.executeShellCommand(EasyMock.eq("ls /data/tombstones/*")))
          .andReturn(result);
        // This gets passed verbatim to createIssForFile above
        EasyMock.expect(mDevice.pullFile(EasyMock.eq(filename)))
                .andReturn(new FakeFile(filename, tombstone.length()));

        mDfrIss = new ByteArrayInputStreamSource(tombstone.getBytes());
        // FIXME: use captures here to make sure we get the string back out
        mListener.testLog(EasyMock.eq(filename), EasyMock.eq(LogDataType.UNKNOWN),
                EasyMock.eq(mDfrIss));

        replayMocks();
        dfr.run();
        verifyMocks();
    }

    // Files' paths should be trimmed to remove white spaces at the end of the lines.
    public void testTrim() throws Exception {
        // Result with trailing white spaces.
        final String result = "/data/tombstones/tombstone_00  \r\n";

        final String filename = "/data/tombstones/tombstone_00";
        final String tombstone = "What do you want on your tombstone?";
        dfr.addPatterns("/data/tombstones/*");

        EasyMock.expect(mDevice.executeShellCommand(EasyMock.eq("ls /data/tombstones/*")))
            .andReturn(result);
        // This gets passed verbatim to createIssForFile above
        EasyMock.expect(mDevice.pullFile(EasyMock.eq(filename)))
            .andReturn(new FakeFile(filename, tombstone.length()));

        mDfrIss = new ByteArrayInputStreamSource(tombstone.getBytes());
        mListener.testLog(EasyMock.eq(filename), EasyMock.eq(LogDataType.UNKNOWN),
            EasyMock.eq(mDfrIss));

        replayMocks();
        List<String> filenames = dfr.run();
        assertEquals(filename, filenames.get(0));
        verifyMocks();
    }

    public void testLineEnding_LF() throws Exception {
        final String[] filenames = {"/data/tombstones/tombstone_00",
                "/data/tombstones/tombstone_01",
                "/data/tombstones/tombstone_02",
                "/data/tombstones/tombstone_03",
                "/data/tombstones/tombstone_04"};
        String result = ArrayUtil.join("\n", (Object[])filenames);
        final String tombstone = "What do you want on your tombstone?";
        dfr.addPatterns("/data/tombstones/*");

        EasyMock.expect(mDevice.executeShellCommand((String)EasyMock.anyObject()))
                .andReturn(result);
        mDfrIss = new ByteArrayInputStreamSource(tombstone.getBytes());
        // This gets passed verbatim to createIssForFile above
        for (String filename : filenames) {
            EasyMock.expect(mDevice.pullFile(EasyMock.eq(filename))).andReturn(
                    new FakeFile(filename, tombstone.length()));

            // FIXME: use captures here to make sure we get the string back out
            mListener.testLog(EasyMock.eq(filename), EasyMock.eq(LogDataType.UNKNOWN),
                    EasyMock.eq(mDfrIss));
        }
        replayMocks();
        dfr.run();
        verifyMocks();
    }

    public void testLineEnding_CRLF() throws Exception {
        final String[] filenames = {"/data/tombstones/tombstone_00",
                "/data/tombstones/tombstone_01",
                "/data/tombstones/tombstone_02",
                "/data/tombstones/tombstone_03",
                "/data/tombstones/tombstone_04"};
        String result = ArrayUtil.join("\r\n", (Object[])filenames);
        final String tombstone = "What do you want on your tombstone?";
        dfr.addPatterns("/data/tombstones/*");

        EasyMock.expect(mDevice.executeShellCommand((String)EasyMock.anyObject()))
                .andReturn(result);
        mDfrIss = new ByteArrayInputStreamSource(tombstone.getBytes());
        // This gets passed verbatim to createIssForFile above
        for (String filename : filenames) {
            EasyMock.expect(mDevice.pullFile(EasyMock.eq(filename))).andReturn(
                    new FakeFile(filename, tombstone.length()));

            // FIXME: use captures here to make sure we get the string back out
            mListener.testLog(EasyMock.eq(filename), EasyMock.eq(LogDataType.UNKNOWN),
                    EasyMock.eq(mDfrIss));
        }
        replayMocks();
        dfr.run();
        verifyMocks();
    }

    /**
     * Make sure that the Reporter behaves as expected when a file is matched by multiple patterns
     */
    public void testRepeat_skip() throws Exception {
        final String result1 = "/data/files/file.png\r\n";
        final String result2 = "/data/files/file.png\r\n/data/files/file.xml\r\n";
        final String pngFilename = "/data/files/file.png";
        final String xmlFilename = "/data/files/file.xml";
        final Map<String, LogDataType> patMap = new HashMap<>(2);
        patMap.put("/data/files/*.png", LogDataType.PNG);
        patMap.put("/data/files/*", LogDataType.UNKNOWN);

        final String pngContents = "This is PNG data";
        final String xmlContents = "<!-- This is XML data -->";
        final InputStreamSource pngIss = new ByteArrayInputStreamSource(pngContents.getBytes());
        final InputStreamSource xmlIss = new ByteArrayInputStreamSource(xmlContents.getBytes());

        dfr =
                new DeviceFileReporter(mDevice, mListener) {
                    @Override
                    InputStreamSource createIssForFile(File file) {
                        if (file.toString().endsWith(".png")) {
                            return pngIss;
                        } else if (file.toString().endsWith(".xml")) {
                            return xmlIss;
                        }
                        return null;
                    }
                };
        dfr.addPatterns(patMap);
        dfr.setInferUnknownDataTypes(false);

        // Set file listing pulling, and reporting expectations
        // Expect that we go through the entire process for the PNG file, and then go through
        // the entire process again for the XML file
        EasyMock.expect(mDevice.executeShellCommand((String)EasyMock.anyObject()))
                .andReturn(result1);
        EasyMock.expect(mDevice.pullFile(EasyMock.eq(pngFilename)))
                .andReturn(new FakeFile(pngFilename, pngContents.length()));
        mListener.testLog(EasyMock.eq(pngFilename), EasyMock.eq(LogDataType.PNG),
                EasyMock.eq(pngIss));

        EasyMock.expect(mDevice.executeShellCommand((String)EasyMock.anyObject()))
                .andReturn(result2);
        EasyMock.expect(mDevice.pullFile(EasyMock.eq(xmlFilename)))
                .andReturn(new FakeFile(xmlFilename, xmlContents.length()));
        mListener.testLog(EasyMock.eq(xmlFilename), EasyMock.eq(LogDataType.UNKNOWN),
                EasyMock.eq(xmlIss));

        replayMocks();
        dfr.run();
        verifyMocks();
        // FIXME: use captures here to make sure we get the string back out
    }

    /**
     * Make sure that the Reporter behaves as expected when a file is matched by multiple patterns
     */
    public void testRepeat_noSkip() throws Exception {
        final String result1 = "/data/files/file.png\r\n";
        final String result2 = "/data/files/file.png\r\n/data/files/file.xml\r\n";
        final String pngFilename = "/data/files/file.png";
        final String xmlFilename = "/data/files/file.xml";
        final Map<String, LogDataType> patMap = new HashMap<>(2);
        patMap.put("/data/files/*.png", LogDataType.PNG);
        patMap.put("/data/files/*", LogDataType.UNKNOWN);

        final String pngContents = "This is PNG data";
        final String xmlContents = "<!-- This is XML data -->";
        final InputStreamSource pngIss = new ByteArrayInputStreamSource(pngContents.getBytes());
        final InputStreamSource xmlIss = new ByteArrayInputStreamSource(xmlContents.getBytes());

        dfr =
                new DeviceFileReporter(mDevice, mListener) {
                    @Override
                    InputStreamSource createIssForFile(File file) {
                        if (file.toString().endsWith(".png")) {
                            return pngIss;
                        } else if (file.toString().endsWith(".xml")) {
                            return xmlIss;
                        }
                        return null;
                    }
                };
        dfr.addPatterns(patMap);
        dfr.setInferUnknownDataTypes(false);
        // this should cause us to see three pulls instead of two
        dfr.setSkipRepeatFiles(false);

        // Set file listing pulling, and reporting expectations
        // Expect that we go through the entire process for the PNG file, and then go through
        // the entire process again for the PNG file (again) and the XML file
        EasyMock.expect(mDevice.executeShellCommand((String)EasyMock.anyObject()))
                .andReturn(result1);
        EasyMock.expect(mDevice.pullFile(EasyMock.eq(pngFilename)))
                .andReturn(new FakeFile(pngFilename, pngContents.length()));
        mListener.testLog(EasyMock.eq(pngFilename), EasyMock.eq(LogDataType.PNG),
                EasyMock.eq(pngIss));

        // Note that the PNG file is picked up with the UNKNOWN data type this time
        EasyMock.expect(mDevice.executeShellCommand((String)EasyMock.anyObject()))
                .andReturn(result2);
        EasyMock.expect(mDevice.pullFile(EasyMock.eq(pngFilename)))
                .andReturn(new FakeFile(pngFilename, pngContents.length()));
        mListener.testLog(EasyMock.eq(pngFilename), EasyMock.eq(LogDataType.UNKNOWN),
                EasyMock.eq(pngIss));
        EasyMock.expect(mDevice.pullFile(EasyMock.eq(xmlFilename)))
                .andReturn(new FakeFile(xmlFilename, xmlContents.length()));
        mListener.testLog(EasyMock.eq(xmlFilename), EasyMock.eq(LogDataType.UNKNOWN),
                EasyMock.eq(xmlIss));

        replayMocks();
        dfr.run();
        verifyMocks();
        // FIXME: use captures here to make sure we get the string back out
    }

    /**
     * Make sure that we correctly handle the case where a file doesn't exist while matching the
     * exact name.
     * <p />
     * This verifies a fix for a bug where we would mistakenly treat the
     * "file.txt: No such file or directory" message as a filename.  This would happen when we tried
     * to match an exact filename that doesn't exist, rather than using a shell glob.
     */
    public void testNoExist() throws Exception {
        final String file = "/data/traces.txt";
        final String result = file + ": No such file or directory\r\n";
        dfr.addPatterns(file);

        EasyMock.expect(mDevice.executeShellCommand((String)EasyMock.anyObject()))
                .andReturn(result);

        replayMocks();
        dfr.run();
        // No pull attempt should happen
        verifyMocks();
    }

    public void testTwoFiles() throws Exception {
        final String result = "/data/tombstones/tombstone_00\r\n/data/tombstones/tombstone_01\r\n";
        final String filename1 = "/data/tombstones/tombstone_00";
        final String filename2 = "/data/tombstones/tombstone_01";
        final String tombstone = "What do you want on your tombstone?";
        dfr.addPatterns("/data/tombstones/*");

        // Search the filesystem
        EasyMock.expect(mDevice.executeShellCommand((String)EasyMock.anyObject()))
                .andReturn(result);

        // Log the first file
        // This gets passed verbatim to createIssForFile above
        EasyMock.expect(mDevice.pullFile(EasyMock.eq(filename1)))
                .andReturn(new FakeFile(filename1, tombstone.length()));
        mDfrIss = new ByteArrayInputStreamSource(tombstone.getBytes());
        // FIXME: use captures here to make sure we get the string back out
        mListener.testLog(EasyMock.eq(filename1), EasyMock.eq(LogDataType.UNKNOWN),
                EasyMock.eq(mDfrIss));

        // Log the second file
        // This gets passed verbatim to createIssForFile above
        EasyMock.expect(mDevice.pullFile(EasyMock.eq(filename2)))
                .andReturn(new FakeFile(filename2, tombstone.length()));
        // FIXME: use captures here to make sure we get the string back out
        mListener.testLog(EasyMock.eq(filename2), EasyMock.eq(LogDataType.UNKNOWN),
                EasyMock.eq(mDfrIss));

        replayMocks();
        dfr.run();
        verifyMocks();
    }

    /**
     * Make sure that data type inference works as expected
     */
    public void testInferDataTypes() throws Exception {
        final String result = "/data/files/file.png\r\n/data/files/file.xml\r\n" +
                "/data/files/file.zip\r\n";
        final String[] filenames = {"/data/files/file.png", "/data/files/file.xml",
                "/data/files/file.zip"};
        final LogDataType[] expTypes = {LogDataType.PNG, LogDataType.XML, LogDataType.ZIP};
        dfr.addPatterns("/data/files/*");

        final String contents = "these are file contents";
        mDfrIss = new ByteArrayInputStreamSource(contents.getBytes());

        EasyMock.expect(mDevice.executeShellCommand((String)EasyMock.anyObject()))
                .andReturn(result);
        // This gets passed verbatim to createIssForFile above
        for (int i = 0; i < filenames.length; ++i) {
            final String filename = filenames[i];
            final LogDataType expType = expTypes[i];
            EasyMock.expect(mDevice.pullFile(EasyMock.eq(filename)))
                    .andReturn(new FakeFile(filename, contents.length()));

            // FIXME: use captures here to make sure we get the string back out
            mListener.testLog(EasyMock.eq(filename), EasyMock.eq(expType),
                    EasyMock.eq(mDfrIss));
        }

        replayMocks();
        dfr.run();
        verifyMocks();
    }


    private void replayMocks() {
        EasyMock.replay(mDevice, mListener);
    }

    private void verifyMocks() {
        EasyMock.verify(mDevice, mListener);
    }
}
