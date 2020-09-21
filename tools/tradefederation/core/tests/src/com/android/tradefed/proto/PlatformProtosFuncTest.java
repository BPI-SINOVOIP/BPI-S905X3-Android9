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
package com.android.tradefed.proto;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.os.BatteryStatsProto;
import android.service.batterystats.BatteryStatsServiceDumpProto;

import com.android.tradefed.device.CollectingByteOutputReceiver;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.IDeviceTest;

import com.google.protobuf.InvalidProtocolBufferException;

import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Functional tests for platformprotos dependency.
 *
 * <p>Since TradeFederation manifest branch does not contain frameworks/base, platformprotos was
 * added to prebuilds/misc/common as a prebuild so that TradeFederation can read protos in
 * frameworks/base. PlatformProtosFuncTest guards against proto parsing failure caused by outdated
 * platformprotos-prebuilt as its source code evolves over time.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class PlatformProtosFuncTest implements IDeviceTest {

    private static final String CMD_DUMP_BATTERYSTATS = "dumpsys batterystats --proto";

    private ITestDevice mDevice;

    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * Dump {@link BatteryStatsServiceDumpProto} into a byte array and parse it into proto object.
     */
    @Test
    public void testDumpAndReadBatteryStatsProto() throws Exception {

        CollectingByteOutputReceiver receiver = new CollectingByteOutputReceiver();
        getDevice().executeShellCommand(CMD_DUMP_BATTERYSTATS, receiver);

        byte[] protoBytes = receiver.getOutput();
        try {
            BatteryStatsServiceDumpProto bssdp = BatteryStatsServiceDumpProto.parseFrom(protoBytes);
            assertTrue(bssdp.hasBatterystats());
            BatteryStatsProto bs = bssdp.getBatterystats();
            assertTrue(bs.hasSystem());
            assertFalse(bs.getUidsList().isEmpty());
            assertTrue(bs.getUids(0).hasCpu());
        } catch (InvalidProtocolBufferException e) {
            fail("Invalid BatteryStatsServiceDumpProto: " + e.getMessage());
        }
    }
}
