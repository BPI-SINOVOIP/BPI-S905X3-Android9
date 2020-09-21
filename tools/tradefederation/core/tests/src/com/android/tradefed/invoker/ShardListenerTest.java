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
package com.android.tradefed.invoker;

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.ILogSaver;
import com.android.tradefed.result.ILogSaverListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.LogFile;
import com.android.tradefed.result.LogSaverResultForwarder;
import com.android.tradefed.result.TestDescription;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Arrays;
import java.util.HashMap;

/** Unit tests for {@link ShardListener}. */
@RunWith(JUnit4.class)
public class ShardListenerTest {
    private ShardListener mShardListener;
    private ITestInvocationListener mMockListener;
    private IInvocationContext mContext;
    private ITestDevice mMockDevice;
    private ILogSaver mMockSaver;

    @Before
    public void setUp() {
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockSaver = EasyMock.createStrictMock(ILogSaver.class);
        mShardListener = new ShardListener(mMockListener);
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockDevice.getSerialNumber()).andStubReturn("serial");
        mContext = new InvocationContext();
        mContext.addDeviceBuildInfo("default", new BuildInfo());
        mContext.addAllocatedDevice("default", mMockDevice);
    }

    /** Ensure that all the events given to the shardlistener are replayed on invocationEnded. */
    @Test
    public void testBufferAndReplay() {
        mMockListener.invocationStarted(mContext);
        mMockListener.testRunStarted("run1", 1);
        TestDescription tid = new TestDescription("class1", "name1");
        mMockListener.testStarted(tid, 0l);
        mMockListener.testEnded(tid, 0l, new HashMap<String, Metric>());
        mMockListener.testRunEnded(0l, new HashMap<String, Metric>());
        mMockListener.invocationEnded(0l);

        EasyMock.replay(mMockListener, mMockDevice);
        mShardListener.invocationStarted(mContext);
        mShardListener.testRunStarted("run1", 1);
        mShardListener.testStarted(tid, 0l);
        mShardListener.testEnded(tid, 0l, new HashMap<String, Metric>());
        mShardListener.testRunEnded(0l, new HashMap<String, Metric>());
        mShardListener.invocationEnded(0l);
        EasyMock.verify(mMockListener, mMockDevice);
    }

    /** Test that the buffering of events is properly done in respect to the modules too. */
    @Test
    public void testBufferAndReplay_withModule() {
        IInvocationContext module1 = new InvocationContext();
        IInvocationContext module2 = new InvocationContext();
        mMockListener.invocationStarted(mContext);
        mMockListener.testModuleStarted(module1);
        mMockListener.testRunStarted("run1", 1);
        TestDescription tid = new TestDescription("class1", "name1");
        mMockListener.testStarted(tid, 0l);
        mMockListener.testEnded(tid, 0l, new HashMap<String, Metric>());
        mMockListener.testRunEnded(0l, new HashMap<String, Metric>());
        mMockListener.testRunStarted("run2", 1);
        mMockListener.testStarted(tid, 0l);
        mMockListener.testEnded(tid, 0l, new HashMap<String, Metric>());
        mMockListener.testRunEnded(0l, new HashMap<String, Metric>());
        mMockListener.testModuleEnded();
        // expectation on second module
        mMockListener.testModuleStarted(module2);
        mMockListener.testRunStarted("run3", 1);
        mMockListener.testStarted(tid, 0l);
        mMockListener.testEnded(tid, 0l, new HashMap<String, Metric>());
        mMockListener.testRunEnded(0l, new HashMap<String, Metric>());
        mMockListener.testModuleEnded();
        mMockListener.invocationEnded(0l);

        EasyMock.replay(mMockListener, mMockDevice);
        mShardListener.invocationStarted(mContext);
        // 1st module
        mShardListener.testModuleStarted(module1);
        mShardListener.testRunStarted("run1", 1);
        mShardListener.testStarted(tid, 0l);
        mShardListener.testEnded(tid, 0l, new HashMap<String, Metric>());
        mShardListener.testRunEnded(0l, new HashMap<String, Metric>());
        mShardListener.testRunStarted("run2", 1);
        mShardListener.testStarted(tid, 0l);
        mShardListener.testEnded(tid, 0l, new HashMap<String, Metric>());
        mShardListener.testRunEnded(0l, new HashMap<String, Metric>());
        mShardListener.testModuleEnded();
        // 2nd module
        mShardListener.testModuleStarted(module2);
        mShardListener.testRunStarted("run3", 1);
        mShardListener.testStarted(tid, 0l);
        mShardListener.testEnded(tid, 0l, new HashMap<String, Metric>());
        mShardListener.testRunEnded(0l, new HashMap<String, Metric>());
        mShardListener.testModuleEnded();

        mShardListener.invocationEnded(0l);
        EasyMock.verify(mMockListener, mMockDevice);
    }

    /** Test the full ordering structure during a sharded pattern. */
    @Test
    public void testLogOrderingForSharding() throws Exception {
        // Force ordering check
        ILogSaverListener mockListener = EasyMock.createStrictMock(ILogSaverListener.class);
        mockListener.setLogSaver(mMockSaver);

        mMockSaver.invocationStarted(mContext);
        EasyMock.expectLastCall().times(2);

        mockListener.invocationStarted(mContext);
        mockListener.testLog(
                EasyMock.eq("run-file"), EasyMock.eq(LogDataType.TEXT), EasyMock.anyObject());
        LogFile runFile = new LogFile("path", "url", false, LogDataType.TEXT, 0L);
        EasyMock.expect(
                        mMockSaver.saveLogData(
                                EasyMock.eq("run-file"),
                                EasyMock.eq(LogDataType.TEXT),
                                EasyMock.anyObject()))
                .andReturn(runFile);
        mockListener.testLogSaved(
                EasyMock.eq("run-file"),
                EasyMock.eq(LogDataType.TEXT),
                EasyMock.anyObject(),
                EasyMock.eq(runFile));

        mockListener.testLog(
                EasyMock.eq("test-file"), EasyMock.eq(LogDataType.TEXT), EasyMock.anyObject());
        LogFile testFile = new LogFile("path", "url", false, LogDataType.TEXT, 0L);
        EasyMock.expect(
                        mMockSaver.saveLogData(
                                EasyMock.eq("test-file"),
                                EasyMock.eq(LogDataType.TEXT),
                                EasyMock.anyObject()))
                .andReturn(testFile);
        mockListener.testLogSaved(
                EasyMock.eq("test-file"),
                EasyMock.eq(LogDataType.TEXT),
                EasyMock.anyObject(),
                EasyMock.eq(testFile));

        mockListener.testRunStarted("run1", 1);
        TestDescription tid = new TestDescription("class1", "name1");
        mockListener.testStarted(tid, 0l);
        // Log association played in order for the test.
        mockListener.logAssociation("test-file", testFile);
        mockListener.testEnded(tid, 0l, new HashMap<String, Metric>());
        // Log association to re-associate file to the run.
        mockListener.logAssociation("run-file", runFile);
        mockListener.testRunEnded(0l, new HashMap<String, Metric>());
        mockListener.invocationEnded(0l);
        EasyMock.expect(mockListener.getSummary()).andReturn(null);

        mMockSaver.invocationEnded(0L);
        EasyMock.expectLastCall().times(2);

        EasyMock.replay(mockListener, mMockSaver, mMockDevice);
        // Setup of sharding
        LogSaverResultForwarder originalInvocation =
                new LogSaverResultForwarder(mMockSaver, Arrays.asList(mockListener));
        ShardMasterResultForwarder masterForwarder =
                new ShardMasterResultForwarder(Arrays.asList(originalInvocation), 1);
        masterForwarder.invocationStarted(mContext);
        ShardListener shard1 = new ShardListener(masterForwarder);
        LogSaverResultForwarder shardedInvocation =
                new LogSaverResultForwarder(mMockSaver, Arrays.asList(shard1));

        shardedInvocation.invocationStarted(mContext);
        shardedInvocation.testRunStarted("run1", 1);
        shardedInvocation.testLog(
                "run-file", LogDataType.TEXT, new ByteArrayInputStreamSource("test".getBytes()));
        shardedInvocation.testStarted(tid, 0l);
        shardedInvocation.testLog(
                "test-file",
                LogDataType.TEXT,
                new ByteArrayInputStreamSource("test file".getBytes()));
        shardedInvocation.testEnded(tid, 0l, new HashMap<String, Metric>());
        shardedInvocation.testRunEnded(0l, new HashMap<String, Metric>());
        shardedInvocation.invocationEnded(0L);

        EasyMock.verify(mockListener, mMockSaver, mMockDevice);
    }
}
