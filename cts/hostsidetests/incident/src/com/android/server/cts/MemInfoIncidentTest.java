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

package com.android.server.cts;

import com.android.server.am.MemInfoDumpProto;
import com.android.server.am.MemInfoDumpProto.AppData;
import com.android.server.am.MemInfoDumpProto.MemItem;
import com.android.server.am.MemInfoDumpProto.ProcessMemory;

/** Test to check that ActivityManager properly outputs meminfo data. */
public class MemInfoIncidentTest extends ProtoDumpTestCase {

    public void testMemInfoDump() throws Exception {
        final MemInfoDumpProto dump =
                getDump(MemInfoDumpProto.parser(), "dumpsys -t 30000 meminfo -a --proto");

        verifyMemInfoDumpProto(dump, PRIVACY_NONE);
    }

    static void verifyMemInfoDumpProto(MemInfoDumpProto dump, final int filterLevel) throws Exception {
        assertTrue(dump.getUptimeDurationMs() >= 0);
        assertTrue(dump.getElapsedRealtimeMs() >= 0);

        for (ProcessMemory pm : dump.getNativeProcessesList()) {
            testProcessMemory(pm);
        }

        for (AppData ad : dump.getAppProcessesList()) {
            testAppData(ad);
        }

        for (MemItem mi : dump.getTotalPssByProcessList()) {
            testMemItem(mi);
        }
        for (MemItem mi : dump.getTotalPssByOomAdjustmentList()) {
            testMemItem(mi);
        }
        for (MemItem mi : dump.getTotalPssByCategoryList()) {
            testMemItem(mi);
        }

        assertTrue(0 <= dump.getTotalRamKb());
        assertTrue(0 <= dump.getCachedPssKb());
        assertTrue(0 <= dump.getCachedKernelKb());
        assertTrue(0 <= dump.getFreeKb());
        assertTrue(0 <= dump.getUsedPssKb());
        assertTrue(0 <= dump.getUsedKernelKb());

        // Ideally lost RAM would not be negative, but there's an issue where it's sometimes
        // calculated to be negative.
        // TODO: re-enable check once the underlying bug has been fixed.
        // assertTrue(0 <= dump.getLostRamKb());

        assertTrue(0 <= dump.getTotalZramKb());
        assertTrue(0 <= dump.getZramPhysicalUsedInSwapKb());
        assertTrue(0 <= dump.getTotalZramSwapKb());

        assertTrue(0 <= dump.getKsmSharingKb());
        assertTrue(0 <= dump.getKsmSharedKb());
        assertTrue(0 <= dump.getKsmUnsharedKb());
        assertTrue(0 <= dump.getKsmVolatileKb());

        assertTrue("Tuning_mb (" + dump.getTuningMb() + ") is not positive", 0 < dump.getTuningMb());
        assertTrue(0 < dump.getTuningLargeMb());

        assertTrue(0 <= dump.getOomKb());

        assertTrue(0 < dump.getRestoreLimitKb());
    }

    private static void testProcessMemory(ProcessMemory pm) throws Exception {
        assertNotNull(pm);

        assertTrue(0 < pm.getPid());
        // On most Linux machines, the max pid value is 32768 (=2^15), but, it can be set to any
        // value up to 4194304 (=2^22) if necessary.
        assertTrue(4194304 >= pm.getPid());

        testHeapInfo(pm.getNativeHeap());
        testHeapInfo(pm.getDalvikHeap());

        for (ProcessMemory.MemoryInfo mi : pm.getOtherHeapsList()) {
            testMemoryInfo(mi);
        }
        testMemoryInfo(pm.getUnknownHeap());
        testHeapInfo(pm.getTotalHeap());

        for (ProcessMemory.MemoryInfo mi : pm.getDalvikDetailsList()) {
            testMemoryInfo(mi);
        }

        ProcessMemory.AppSummary as = pm.getAppSummary();
        assertTrue(0 <= as.getJavaHeapPssKb());
        assertTrue(0 <= as.getNativeHeapPssKb());
        assertTrue(0 <= as.getCodePssKb());
        assertTrue(0 <= as.getStackPssKb());
        assertTrue(0 <= as.getGraphicsPssKb());
        assertTrue(0 <= as.getPrivateOtherPssKb());
        assertTrue(0 <= as.getSystemPssKb());
        assertTrue(0 <= as.getTotalSwapPss());
        assertTrue(0 <= as.getTotalSwapKb());
    }

    private static void testMemoryInfo(ProcessMemory.MemoryInfo mi) throws Exception {
        assertNotNull(mi);

        assertTrue(0 <= mi.getTotalPssKb());
        assertTrue(0 <= mi.getCleanPssKb());
        assertTrue(0 <= mi.getSharedDirtyKb());
        assertTrue(0 <= mi.getPrivateDirtyKb());
        assertTrue(0 <= mi.getSharedCleanKb());
        assertTrue(0 <= mi.getPrivateCleanKb());
        assertTrue(0 <= mi.getDirtySwapKb());
        assertTrue(0 <= mi.getDirtySwapPssKb());
    }

    private static void testHeapInfo(ProcessMemory.HeapInfo hi) throws Exception {
        assertNotNull(hi);

        testMemoryInfo(hi.getMemInfo());
        assertTrue(0 <= hi.getHeapSizeKb());
        assertTrue(0 <= hi.getHeapAllocKb());
        assertTrue(0 <= hi.getHeapFreeKb());
    }

    private static void testAppData(AppData ad) throws Exception {
        assertNotNull(ad);

        testProcessMemory(ad.getProcessMemory());

        AppData.ObjectStats os = ad.getObjects();
        assertTrue(0 <= os.getViewInstanceCount());
        assertTrue(0 <= os.getViewRootInstanceCount());
        assertTrue(0 <= os.getAppContextInstanceCount());
        assertTrue(0 <= os.getActivityInstanceCount());
        assertTrue(0 <= os.getGlobalAssetCount());
        assertTrue(0 <= os.getGlobalAssetManagerCount());
        assertTrue(0 <= os.getLocalBinderObjectCount());
        assertTrue(0 <= os.getProxyBinderObjectCount());
        assertTrue(0 <= os.getParcelMemoryKb());
        assertTrue(0 <= os.getParcelCount());
        assertTrue(0 <= os.getBinderObjectDeathCount());
        assertTrue(0 <= os.getOpenSslSocketCount());
        assertTrue(0 <= os.getWebviewInstanceCount());

        AppData.SqlStats ss = ad.getSql();
        assertTrue(0 <= ss.getMemoryUsedKb());
        assertTrue(0 <= ss.getPagecacheOverflowKb());
        assertTrue(0 <= ss.getMallocSizeKb());
        for (AppData.SqlStats.Database d : ss.getDatabasesList()) {
            assertTrue(0 <= d.getPageSize());
            assertTrue(0 <= d.getDbSize());
            assertTrue(0 <= d.getLookasideB());
        }
    }

    private static void testMemItem(MemItem mi) throws Exception {
        assertNotNull(mi);

        assertTrue(0 <= mi.getPssKb());
        assertTrue(0 <= mi.getSwapPssKb());

        for (MemItem smi : mi.getSubItemsList()) {
            testMemItem(smi);
        }
    }
}
