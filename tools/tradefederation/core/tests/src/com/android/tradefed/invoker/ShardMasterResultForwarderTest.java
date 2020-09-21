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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.times;

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.ILogSaver;
import com.android.tradefed.result.ILogSaverListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.LogSaverResultForwarder;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/** Unit tests for {@link ShardMasterResultForwarder}. */
@RunWith(JUnit4.class)
public class ShardMasterResultForwarderTest {
    private ShardMasterResultForwarder mShardMaster;
    @Mock private ITestInvocationListener mMockListener;
    @Mock private LogListenerTestInterface mMockLogListener;
    @Mock private ILogSaver mMockLogSaver;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        List<ITestInvocationListener> listListener = new ArrayList<>();
        listListener.add(mMockListener);
        mShardMaster = new ShardMasterResultForwarder(listListener, 2);
    }

    /**
     * Test that build info attributes from each shard are carried to the main build info for the
     * same device.
     */
    @Test
    public void testForwardBuildInfo() {
        IInvocationContext main = new InvocationContext();
        IBuildInfo mainBuild = new BuildInfo();
        main.addAllocatedDevice("device1", Mockito.mock(ITestDevice.class));
        main.addDeviceBuildInfo("device1", mainBuild);
        assertTrue(mainBuild.getBuildAttributes().isEmpty());

        InvocationContext shard1 = new InvocationContext();
        IBuildInfo shardBuild1 = new BuildInfo();
        shard1.addAllocatedDevice("device1", Mockito.mock(ITestDevice.class));
        shard1.addDeviceBuildInfo("device1", shardBuild1);
        shardBuild1.addBuildAttribute("shard1", "value1");

        InvocationContext shard2 = new InvocationContext();
        IBuildInfo shardBuild2 = new BuildInfo();
        shard2.addAllocatedDevice("device1", Mockito.mock(ITestDevice.class));
        shard2.addDeviceBuildInfo("device1", shardBuild2);
        shardBuild2.addBuildAttribute("shard2", "value2");

        mShardMaster.invocationStarted(main);
        mShardMaster.invocationStarted(shard1);
        mShardMaster.invocationStarted(shard2);
        mShardMaster.invocationEnded(0l);
        mShardMaster.invocationEnded(1l);

        assertEquals("value1", mainBuild.getBuildAttributes().get("shard1"));
        assertEquals("value2", mainBuild.getBuildAttributes().get("shard2"));
    }

    /**
     * Test to ensure that even with multi devices, the build attributes of each matching device are
     * copied to the main build.
     */
    @Test
    public void testForwardBuildInfo_multiDevice() {
        IInvocationContext main = new InvocationContext();
        IBuildInfo mainBuild1 = new BuildInfo();
        main.addAllocatedDevice("device1", Mockito.mock(ITestDevice.class));
        main.addDeviceBuildInfo("device1", mainBuild1);
        assertTrue(mainBuild1.getBuildAttributes().isEmpty());
        // Second device
        IBuildInfo mainBuild2 = new BuildInfo();
        main.addAllocatedDevice("device2", Mockito.mock(ITestDevice.class));
        main.addDeviceBuildInfo("device2", mainBuild2);
        assertTrue(mainBuild2.getBuildAttributes().isEmpty());

        InvocationContext shard1 = new InvocationContext();
        IBuildInfo shardBuild1 = new BuildInfo();
        shard1.addAllocatedDevice("device1", Mockito.mock(ITestDevice.class));
        shard1.addDeviceBuildInfo("device1", shardBuild1);
        shardBuild1.addBuildAttribute("shard1", "value1");
        // second device on shard 1
        IBuildInfo shardBuild1_2 = new BuildInfo();
        shard1.addAllocatedDevice("device2", Mockito.mock(ITestDevice.class));
        shard1.addDeviceBuildInfo("device2", shardBuild1_2);
        shardBuild1_2.addBuildAttribute("shard1_device2", "value1_device2");

        InvocationContext shard2 = new InvocationContext();
        IBuildInfo shardBuild2 = new BuildInfo();
        shard2.addAllocatedDevice("device1", Mockito.mock(ITestDevice.class));
        shard2.addDeviceBuildInfo("device1", shardBuild2);
        shardBuild2.addBuildAttribute("shard2", "value2");
        // second device on shard 2
        IBuildInfo shardBuild2_2 = new BuildInfo();
        shard2.addAllocatedDevice("device2", Mockito.mock(ITestDevice.class));
        shard2.addDeviceBuildInfo("device2", shardBuild2_2);
        shardBuild2_2.addBuildAttribute("shard2_device2", "value2_device2");

        mShardMaster.invocationStarted(main);
        mShardMaster.invocationStarted(shard1);
        mShardMaster.invocationStarted(shard2);
        mShardMaster.invocationEnded(0l);
        mShardMaster.invocationEnded(1l);

        assertEquals("value1", mainBuild1.getBuildAttributes().get("shard1"));
        assertEquals("value2", mainBuild1.getBuildAttributes().get("shard2"));
        assertEquals(2, mainBuild1.getBuildAttributes().size());
        assertEquals("value1_device2", mainBuild2.getBuildAttributes().get("shard1_device2"));
        assertEquals("value2_device2", mainBuild2.getBuildAttributes().get("shard2_device2"));
        // Each build only received the matching device build from shards, nothing more.
        assertEquals(2, mainBuild2.getBuildAttributes().size());
    }

    /** Test interface to check a reporter implementing {@link ILogSaverListener}. */
    public interface LogListenerTestInterface extends ITestInvocationListener, ILogSaverListener {}

    /** Test that the log saver is only called once during a sharding setup. */
    @Test
    public void testForward_Sharded() throws Exception {
        // Setup the reporters like in a sharding session
        ShardMasterResultForwarder reporter =
                new ShardMasterResultForwarder(Arrays.asList(mMockLogListener), 1);
        ShardListener shardListener = new ShardListener(reporter);
        LogSaverResultForwarder invocationLogger =
                new LogSaverResultForwarder(mMockLogSaver, Arrays.asList(shardListener));
        IInvocationContext main = new InvocationContext();
        IBuildInfo mainBuild1 = new BuildInfo();
        main.addAllocatedDevice("device1", Mockito.mock(ITestDevice.class));
        main.addDeviceBuildInfo("device1", mainBuild1);

        invocationLogger.invocationStarted(main);
        invocationLogger.testLog(
                "fakeData", LogDataType.TEXT, new ByteArrayInputStreamSource("test".getBytes()));
        invocationLogger.invocationEnded(500L);

        // Log saver only saved the file once.
        Mockito.verify(mMockLogSaver, times(1))
                .saveLogData(Mockito.any(), Mockito.any(), Mockito.any());
        Mockito.verify(mMockLogListener, times(1)).invocationStarted(Mockito.eq(main));
        Mockito.verify(mMockLogListener, times(1))
                .testLog(Mockito.any(), Mockito.any(), Mockito.any());
        // The callback was received all the way to the last reporter.
        Mockito.verify(mMockLogListener, times(1))
                .testLogSaved(Mockito.any(), Mockito.any(), Mockito.any(), Mockito.any());
        Mockito.verify(mMockLogListener, times(1)).invocationEnded(500L);
    }
}
