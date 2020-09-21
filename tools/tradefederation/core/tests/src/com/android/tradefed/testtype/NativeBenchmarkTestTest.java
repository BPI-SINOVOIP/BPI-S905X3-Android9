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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import com.android.ddmlib.FileListingService;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.IFileEntry;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.HashMap;
import java.util.concurrent.TimeUnit;

/** Unit tests for {@link NativeBenchmarkTest}. */
@RunWith(JUnit4.class)
public class NativeBenchmarkTestTest {

    private NativeBenchmarkTest mBenchmark;
    private ITestInvocationListener mListener;
    private ITestDevice mDevice;

    @Before
    public void setUp() {
        mBenchmark = new NativeBenchmarkTest();
        mListener = EasyMock.createMock(ITestInvocationListener.class);
        mDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mDevice.getSerialNumber()).andStubReturn("SERIAL");
    }

    private void replayMocks(Object... mocks) {
        EasyMock.replay(mListener, mDevice);
        EasyMock.replay(mocks);
    }

    private void verifyMocks(Object... mocks) {
        EasyMock.verify(mListener, mDevice);
        EasyMock.verify(mocks);
    }

    @Test
    public void testRun_noDevice() throws DeviceNotAvailableException {
        try {
            replayMocks();
            mBenchmark.run(mListener);
            fail("An exception should have been thrown");
        } catch (IllegalArgumentException expected) {
            assertEquals("Device has not been set", expected.getMessage());
            verifyMocks();
        }
    }

    @Test
    public void testGetTestPath() {
        String res = mBenchmark.getTestPath();
        assertEquals(NativeBenchmarkTest.DEFAULT_TEST_PATH, res);
    }

    @Test
    public void testGetTestPath_withModule() throws Exception {
        OptionSetter setter = new OptionSetter(mBenchmark);
        setter.setOptionValue("benchmark-module-name", "TEST");
        String res = mBenchmark.getTestPath();
        String expected = String.format("%s%s%s", NativeBenchmarkTest.DEFAULT_TEST_PATH,
                FileListingService.FILE_SEPARATOR, "TEST");
        assertEquals(expected, res);
    }

    @Test
    public void testRun_noFileEntry() throws DeviceNotAvailableException {
        mBenchmark.setDevice(mDevice);
        EasyMock.expect(mDevice.getFileEntry((String) EasyMock.anyObject())).andReturn(null);
        replayMocks();
        mBenchmark.run(mListener);
        verifyMocks();
    }

    @Test
    public void testRun_setMaxFrequency() throws Exception {
        mBenchmark =
                new NativeBenchmarkTest() {
                    @Override
                    protected void doRunAllTestsInSubdirectory(
                            IFileEntry rootEntry,
                            ITestDevice testDevice,
                            ITestInvocationListener listener)
                            throws DeviceNotAvailableException {
                        // empty on purpose
                    }
                };
        mBenchmark.setDevice(mDevice);
        OptionSetter setter = new OptionSetter(mBenchmark);
        setter.setOptionValue("max-cpu-freq", "true");
        IFileEntry fakeEntry = EasyMock.createMock(IFileEntry.class);
        EasyMock.expect(mDevice.getFileEntry((String) EasyMock.anyObject())).andReturn(fakeEntry);
        EasyMock.expect(mDevice.executeShellCommand(EasyMock.eq("cat "
                + "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq > "
                + "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq"))).andReturn("");
        EasyMock.expect(mDevice.executeShellCommand(EasyMock.eq("cat "
                + "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq > "
                + "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq"))).andReturn("");
        replayMocks(fakeEntry);
        mBenchmark.run(mListener);
        verifyMocks(fakeEntry);
    }

    @Test
    public void testRun_doRunAllTestsInSubdirectory() throws Exception {
        final String fakeRunName = "RUN_NAME";
        final String fakeFullPath = "/path/" + fakeRunName;
        mBenchmark.setDevice(mDevice);
        IFileEntry fakeEntry = EasyMock.createMock(IFileEntry.class);
        EasyMock.expect(fakeEntry.isDirectory()).andReturn(false);
        EasyMock.expect(fakeEntry.getName()).andStubReturn(fakeRunName);
        EasyMock.expect(fakeEntry.getFullEscapedPath()).andReturn(fakeFullPath);
        EasyMock.expect(mDevice.getFileEntry((String) EasyMock.anyObject())).andReturn(fakeEntry);
        EasyMock.expect(mDevice.executeShellCommand(EasyMock.eq("chmod 755 " + fakeFullPath)))
                .andReturn("");
        mDevice.executeShellCommand(EasyMock.eq(fakeFullPath + " -n 1000 -d 0.000000 -c 1 -s 1"),
                EasyMock.anyObject(), EasyMock.anyLong(), EasyMock.eq(TimeUnit.MILLISECONDS),
                EasyMock.eq(0));
        EasyMock.expectLastCall();
        mListener.testRunStarted(EasyMock.eq(fakeRunName), EasyMock.anyInt());
        EasyMock.expectLastCall();
        mListener.testRunEnded(EasyMock.anyLong(), (HashMap<String, Metric>) EasyMock.anyObject());
        EasyMock.expectLastCall();
        replayMocks(fakeEntry);
        mBenchmark.run(mListener);
        verifyMocks(fakeEntry);
    }
}
