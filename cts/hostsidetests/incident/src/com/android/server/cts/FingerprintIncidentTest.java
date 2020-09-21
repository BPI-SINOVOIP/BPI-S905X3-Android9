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

import com.android.server.fingerprint.FingerprintServiceDumpProto;
import com.android.server.fingerprint.FingerprintUserStatsProto;
import com.android.server.fingerprint.PerformanceStatsProto;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;


/**
 * Test to check that the fingerprint service properly outputs its dump state.
 */
public class FingerprintIncidentTest extends ProtoDumpTestCase {
    public void testFingerprintServiceDump() throws Exception {
        // If the device doesn't support fingerprints, then pass.
        if (!supportsFingerprint(getDevice())) {
            return;
        }

        final FingerprintServiceDumpProto dump =
                getDump(FingerprintServiceDumpProto.parser(), "dumpsys fingerprint --proto");

        verifyFingerprintServiceDumpProto(dump, PRIVACY_NONE);
    }

    static void verifyFingerprintServiceDumpProto(FingerprintServiceDumpProto dump, int filterLevel) {
        for (int i = 0; i < dump.getUsersCount(); ++i) {
            final FingerprintUserStatsProto userStats = dump.getUsers(i);
            assertTrue(0 <= userStats.getUserId());
            assertTrue(0 <= userStats.getNumFingerprints());

            verifyPerformanceStatsProto(userStats.getNormal());
            verifyPerformanceStatsProto(userStats.getCrypto());
        }
    }

    private static void verifyPerformanceStatsProto(PerformanceStatsProto psp) {
        assertTrue(0 <= psp.getAccept());
        assertTrue(0 <= psp.getReject());
        assertTrue(0 <= psp.getAcquire());
        assertTrue(0 <= psp.getLockout());
        assertTrue(0 <= psp.getPermanentLockout());
    }

    static boolean supportsFingerprint(ITestDevice device) throws DeviceNotAvailableException {
        if (!device.hasFeature("android.hardware.fingerprint")) {
            CLog.d("Bypass as android.hardware.fingerprint is not supported.");
            return false;
        }
        return true;
    }
}

