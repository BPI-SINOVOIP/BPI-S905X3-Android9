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
package com.android.tradefed.testtype.testdefs;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.InstrumentationTest;
import com.android.tradefed.testtype.MockInstrumentationTest;

import junit.framework.TestCase;

import org.easymock.Capture;
import org.easymock.EasyMock;
import org.easymock.IAnswer;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.HashMap;

/**
 * Unit tests for {@link XmlDefsTest}.
 */
public class XmlDefsTestTest extends TestCase {

    private static final String TEST_PATH = "foopath";
    private static final String TEST_DEF_DATA = XmlDefsParserTest.FULL_DATA;
    private static final String TEST_PKG = XmlDefsParserTest.TEST_PKG;
    private static final String TEST_COVERAGE_TARGET = XmlDefsParserTest.TEST_COVERAGE_TARGET;
    private ITestDevice mMockTestDevice;
    private ITestInvocationListener mMockListener;
    private XmlDefsTest mXmlTest;
    private MockInstrumentationTest mMockInstrumentationTest;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mMockTestDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockTestDevice.getSerialNumber()).andReturn("foo").anyTimes();
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockInstrumentationTest = new MockInstrumentationTest();

        mXmlTest = new XmlDefsTest() {
            @Override
            InstrumentationTest createInstrumentationTest() {
                return mMockInstrumentationTest;
            }
        };
        mXmlTest.setDevice(mMockTestDevice);
    }

    /**
     * Test the run normal case. Simple verification that expected data is passed along, etc.
     */
    public void testRun() throws DeviceNotAvailableException {
        mXmlTest.addRemoteFilePath(TEST_PATH);

        injectMockXmlData();
        mMockListener.testRunStarted(TEST_PKG, 0);
        Capture<HashMap<String, Metric>> captureMetrics = new Capture<HashMap<String, Metric>>();
        mMockListener.testRunEnded(EasyMock.anyLong(), EasyMock.capture(captureMetrics));
        EasyMock.replay(mMockTestDevice, mMockListener);
        mXmlTest.run(mMockListener);
        assertEquals(mMockListener, mMockInstrumentationTest.getListener());
        assertEquals(TEST_PKG, mMockInstrumentationTest.getPackageName());
        assertEquals(
                TEST_COVERAGE_TARGET,
                captureMetrics
                        .getValue()
                        .get(XmlDefsTest.COVERAGE_TARGET_KEY)
                        .getMeasurements()
                        .getSingleString());
    }

    private void injectMockXmlData() throws DeviceNotAvailableException {
        // TODO: it would be nice to mock out the file objects, so this test wouldn't need to do
        // IO
        mMockTestDevice.pullFile(EasyMock.eq(TEST_PATH), (File)EasyMock.anyObject());
        EasyMock.expectLastCall().andAnswer(new IAnswer<Object>() {
            @Override
            public Object answer() throws Throwable {
             // simulate the pull file by dumping data into local file
                FileOutputStream outStream;
                try {
                    outStream = new FileOutputStream((File)EasyMock.getCurrentArguments()[1]);
                    outStream.write(TEST_DEF_DATA.getBytes());
                    outStream.close();
                    return true;
                } catch (IOException e) {
                    fail(e.toString());
                }
                return false;
            }
        });
    }

    /**
     * Test a run that was aborted then resumed
     */
    public void testRun_resume() throws DeviceNotAvailableException {
        mXmlTest.addRemoteFilePath(TEST_PATH);
        // turn off sending of coverage for simplicity
        mXmlTest.setSendCoverage(false);
        injectMockXmlData();
        mMockInstrumentationTest.setException(new DeviceNotAvailableException());
        EasyMock.replay(mMockTestDevice, mMockListener);
        try {
            mXmlTest.run(mMockListener);
            fail("DeviceNotAvailableException not thrown");
        } catch (DeviceNotAvailableException e) {
            // expected
        }
        // verify InstrumentationTest.run was called
        assertEquals(mMockListener, mMockInstrumentationTest.getListener());
        mMockInstrumentationTest.setException(null);
        mMockInstrumentationTest.clearListener();
        // resume test run, on a different device
        ITestDevice newTestDevice = EasyMock.createMock(ITestDevice.class);
        mXmlTest.setDevice(newTestDevice);
        mXmlTest.run(mMockListener);
        // verify InstrumentationTest.run was called again, with same listener + different device
        assertEquals(mMockListener, mMockInstrumentationTest.getListener());
        assertEquals(newTestDevice, mMockInstrumentationTest.getDevice());
    }

    /**
     * Test that IllegalArgumentException is thrown when attempting run without setting device.
     */
    public void testRun_noDevice() throws Exception {
        mXmlTest.addRemoteFilePath(TEST_PATH);
        mXmlTest.setDevice(null);
        try {
            mXmlTest.run(mMockListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
        assertNull(mMockInstrumentationTest.getPackageName());
    }

    /**
     * Test that IllegalArgumentException is thrown when attempting run without setting any file
     * paths.
     */
    public void testRun_noPath() throws Exception {
        try {
            mXmlTest.run(mMockListener);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
        assertNull(mMockInstrumentationTest.getPackageName());
    }
}
