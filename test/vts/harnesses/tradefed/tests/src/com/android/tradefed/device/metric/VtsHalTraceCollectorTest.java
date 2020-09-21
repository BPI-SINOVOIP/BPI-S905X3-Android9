/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.tradefed.device.metric;

import static org.junit.Assert.assertTrue;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.util.FileUtil;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Unit tests for {@link VtsHalTraceCollector}.
 */
@RunWith(JUnit4.class)
public class VtsHalTraceCollectorTest {
    private static final String VTS_TMP_DIR = VtsHalTraceCollector.VTS_TMP_DIR;
    private static final String TRACE_PATH = VtsHalTraceCollector.TRACE_PATH;

    private Map<String, String> mBuildAttributes = new HashMap<>();
    private List<IBuildInfo> mBuildInfos = new ArrayList<IBuildInfo>();
    private List<ITestDevice> mDevices = new ArrayList<ITestDevice>();
    private VtsHalTraceCollector mCollect;
    private IBuildInfo mMockBuildInfo;
    private ITestDevice mMockDevice;
    private IInvocationContext mMockContext;
    private ITestInvocationListener mMockListener;

    @Before
    public void setUp() throws Exception {
        mMockDevice = EasyMock.createNiceMock(ITestDevice.class);
        mMockBuildInfo = EasyMock.createNiceMock(IBuildInfo.class);
        mMockListener = EasyMock.createNiceMock(ITestInvocationListener.class);
        mMockContext = EasyMock.createNiceMock(IInvocationContext.class);
        mCollect = new VtsHalTraceCollector() {
            @Override
            public String getRunName() {
                return "device1 testRun";
            }
        };
        mCollect.init(mMockContext, mMockListener);
    }

    @Test
    public void testOnTestRunEnd() throws Exception {
        File traceBaseDir = FileUtil.createTempDir("vts-trace");
        File traceDir = null;
        try {
            mBuildAttributes.put(TRACE_PATH, traceBaseDir.getAbsolutePath());
            EasyMock.expect(mMockBuildInfo.getBuildAttributes()).andReturn(mBuildAttributes);
            mDevices.add(mMockDevice);
            mBuildInfos.add(mMockBuildInfo);
            EasyMock.expect(mMockContext.getDevices()).andReturn(mDevices);
            EasyMock.expect(mMockContext.getBuildInfos()).andReturn(mBuildInfos);
            String listResult = VTS_TMP_DIR + "test1.vts.trace\n" + VTS_TMP_DIR + "test2.vts.trace";
            EasyMock.expect(mMockDevice.executeShellCommand(
                                    EasyMock.eq(String.format("ls %s/*.vts.trace", VTS_TMP_DIR))))
                    .andReturn(listResult);
            traceDir = new File(traceBaseDir, "device1_testRun/");
            File testTrace1 = new File(traceDir, "test1.vts.trace");
            File testTrace2 = new File(traceDir, "test2.vts.trace");
            EasyMock.expect(mMockDevice.pullFile(EasyMock.eq(VTS_TMP_DIR + "test1.vts.trace"),
                                    EasyMock.eq(testTrace1)))
                    .andReturn(true)
                    .times(1);
            EasyMock.expect(mMockDevice.pullFile(EasyMock.eq(VTS_TMP_DIR + "test2.vts.trace"),
                                    EasyMock.eq(testTrace2)))
                    .andReturn(true)
                    .times(1);

            EasyMock.replay(mMockBuildInfo);
            EasyMock.replay(mMockContext);
            EasyMock.replay(mMockDevice);

            mCollect.onTestRunEnd(null, null);
            assertTrue(traceDir.exists());

            EasyMock.verify(mMockBuildInfo);
            EasyMock.verify(mMockContext);
            EasyMock.verify(mMockDevice);

        } finally {
            FileUtil.recursiveDelete(traceDir);
            FileUtil.recursiveDelete(traceBaseDir);
        }
    }
}
