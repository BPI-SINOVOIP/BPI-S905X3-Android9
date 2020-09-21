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

package com.android.server.cts;

import android.service.diskstats.DiskStatsAppSizesProto;
import android.service.diskstats.DiskStatsCachedValuesProto;
import android.service.diskstats.DiskStatsFreeSpaceProto;
import android.service.diskstats.DiskStatsServiceDumpProto;
import com.android.tradefed.device.ITestDevice;

/**
 * Test proto dump of diskstats
 */
public class DiskStatsProtoTest extends ProtoDumpTestCase {
    /**
     * Test that diskstats dump is reasonable
     *
     * @throws Exception
     */
    public void testDump() throws Exception {
        final DiskStatsServiceDumpProto dump = getDump(DiskStatsServiceDumpProto.parser(),
                "dumpsys diskstats --proto");

        verifyDiskStatsServiceDumpProto(dump, PRIVACY_NONE, getDevice());
    }

    static void verifyDiskStatsServiceDumpProto(DiskStatsServiceDumpProto dump, final int filterLevel, ITestDevice device) throws Exception {
        // At least one partition listed
        assertTrue(dump.getPartitionsFreeSpaceCount() > 0);
        // Test latency
        boolean testError = dump.getHasTestError();
        if (testError) {
            assertNotNull(dump.getErrorMessage());
            if (filterLevel == PRIVACY_AUTO) {
                assertTrue(dump.getErrorMessage().isEmpty());
            }
        } else {
            assertTrue(dump.getWrite512BLatencyMillis() < 100); // Less than 100ms
        }
        assertTrue(dump.getWrite512BLatencyMillis() >= 0); // Non-negative
        assertTrue(dump.getBenchmarkedWriteSpeedKbps() >= 0); // Non-negative

        for (DiskStatsFreeSpaceProto dsfs : dump.getPartitionsFreeSpaceList()) {
            verifyDiskStatsFreeSpaceProto(dsfs);
        }

        DiskStatsServiceDumpProto.EncryptionType encryptionType = dump.getEncryption();
        if ("file".equals(device.getProperty("ro.crypto.type"))) {
            assertEquals(DiskStatsServiceDumpProto.EncryptionType.ENCRYPTION_FILE_BASED,
                    encryptionType);
        }
        assertTrue(DiskStatsServiceDumpProto.EncryptionType.getDescriptor().getValues()
                .contains(encryptionType.getValueDescriptor()));

        verifyDiskStatsCachedValuesProto(dump.getCachedFolderSizes());
    }

    private static void verifyDiskStatsAppSizesProto(DiskStatsAppSizesProto dsas) throws Exception {
        assertTrue(dsas.getAppSizeKb() >= 0);
        assertTrue(dsas.getCacheSizeKb() >= 0);
        assertTrue(dsas.getAppDataSizeKb() >= 0);
    }

    private static void verifyDiskStatsCachedValuesProto(DiskStatsCachedValuesProto dscv) throws Exception {
        assertTrue(dscv.getAggAppsSizeKb() >= 0);
        assertTrue(dscv.getAggAppsCacheSizeKb() >= 0);
        assertTrue(dscv.getPhotosSizeKb() >= 0);
        assertTrue(dscv.getVideosSizeKb() >= 0);
        assertTrue(dscv.getAudioSizeKb() >= 0);
        assertTrue(dscv.getDownloadsSizeKb() >= 0);
        assertTrue(dscv.getSystemSizeKb() >= 0);
        assertTrue(dscv.getOtherSizeKb() >= 0);
        assertTrue(dscv.getAggAppsDataSizeKb() >= 0);

        for (DiskStatsAppSizesProto dsas : dscv.getAppSizesList()) {
            verifyDiskStatsAppSizesProto(dsas);
        }
    }

    private static void verifyDiskStatsFreeSpaceProto(DiskStatsFreeSpaceProto dsfs) throws Exception {
        assertTrue(DiskStatsFreeSpaceProto.Folder.getDescriptor().getValues()
                .contains(dsfs.getFolder().getValueDescriptor()));
        assertTrue(dsfs.getAvailableSpaceKb() >= 0);
        assertTrue(dsfs.getTotalSpaceKb() >= 0);
    }
}
