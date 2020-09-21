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

import android.service.print.ActivePrintServiceProto;
import android.service.print.CachedPrintJobProto;
import android.service.print.PrinterDiscoverySessionProto;
import android.service.print.PrintDocumentInfoProto;
import android.service.print.PrinterIdProto;
import android.service.print.PrinterInfoProto;
import android.service.print.PrintJobInfoProto;
import android.service.print.PrintServiceDumpProto;
import android.service.print.PrintSpoolerInternalStateProto;
import android.service.print.PrintSpoolerStateProto;
import android.service.print.PrintUserStateProto;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil;

/**
 * Test proto dump of print
 */
public class PrintProtoTest extends ProtoDumpTestCase {
    /**
     * Test that print dump is reasonable
     *
     * @throws Exception
     */
    public void testDump() throws Exception {
        // If the device doesn't support printing, then pass.
        if (!supportsPrinting(getDevice())) {
            LogUtil.CLog.d("Bypass as android.software.print is not supported.");
            return;
        }

        PrintServiceDumpProto dump = getDump(PrintServiceDumpProto.parser(),
                "dumpsys print --proto");

        verifyPrintServiceDumpProto(dump, PRIVACY_NONE);
    }

    static void verifyPrintServiceDumpProto(PrintServiceDumpProto dump, final int filterLevel) throws Exception {
        if (dump.getUserStatesCount() > 0) {
            PrintUserStateProto userState = dump.getUserStatesList().get(0);
            assertEquals(0, userState.getUserId());
        }

        for (PrintUserStateProto pus : dump.getUserStatesList()) {
            for (ActivePrintServiceProto aps : pus.getActiveServicesList()) {
                verifyActivePrintServiceProto(aps, filterLevel);
            }
            for (CachedPrintJobProto cpj : pus.getCachedPrintJobsList()) {
                verifyPrintJobInfoProto(cpj.getPrintJob(), filterLevel);
            }
            for (PrinterDiscoverySessionProto pds : pus.getDiscoverySessionsList()) {
                verifyPrinterDiscoverySessionProto(pds, filterLevel);
            }
            verifyPrintSpoolerStateProto(pus.getPrintSpoolerState(), filterLevel);
        }
    }

    private static void verifyActivePrintServiceProto(ActivePrintServiceProto aps, final int filterLevel) throws Exception {
        for (PrinterIdProto pip : aps.getTrackedPrintersList()) {
            verifyPrinterIdProto(pip, filterLevel);
        }
    }

    private static void verifyPrinterDiscoverySessionProto(PrinterDiscoverySessionProto pds, final int filterLevel) throws Exception {
        for (PrinterIdProto pip : pds.getTrackedPrinterRequestsList()) {
            verifyPrinterIdProto(pip, filterLevel);
        }
        for (PrinterInfoProto pip : pds.getPrinterList()) {
            verifyPrinterInfoProto(pip, filterLevel);
        }
    }

    private static void verifyPrintDocumentInfoProto(PrintDocumentInfoProto pdi, final int filterLevel) throws Exception {
        if (filterLevel == PRIVACY_AUTO) {
            assertTrue(pdi.getName().isEmpty());
        }
        assertTrue(0 <= pdi.getPageCount());
        assertTrue(0 <= pdi.getDataSize());
    }

    private static void verifyPrinterIdProto(PrinterIdProto pip, final int filterLevel) throws Exception {
        if (filterLevel == PRIVACY_AUTO) {
            assertTrue(pip.getLocalId().isEmpty());
        }
    }

    private static void verifyPrinterInfoProto(PrinterInfoProto pip, final int filterLevel) throws Exception {
        verifyPrinterIdProto(pip.getId(), filterLevel);
        if (filterLevel == PRIVACY_AUTO) {
            assertTrue(pip.getName().isEmpty());
            assertTrue(pip.getDescription().isEmpty());
        }
        assertTrue(
                PrinterInfoProto.Status.getDescriptor().getValues()
                        .contains(pip.getStatus().getValueDescriptor()));
    }

    private static void verifyPrintJobInfoProto(PrintJobInfoProto pji, final int filterLevel) throws Exception {
        if (filterLevel == PRIVACY_AUTO) {
            assertTrue(pji.getLabel().isEmpty());
            assertTrue(pji.getPrintJobId().isEmpty());
            assertTrue(pji.getTag().isEmpty());
        }
        assertTrue(
                PrintJobInfoProto.State.getDescriptor().getValues()
                        .contains(pji.getState().getValueDescriptor()));
        verifyPrinterIdProto(pji.getPrinter(), filterLevel);
        verifyPrintDocumentInfoProto(pji.getDocumentInfo(), filterLevel);
    }

    private static void verifyPrintSpoolerStateProto(PrintSpoolerStateProto pss, final int filterLevel) throws Exception {
        verifyPrintSpoolerInternalStateProto(pss.getInternalState(), filterLevel);
    }

    private static void verifyPrintSpoolerInternalStateProto(PrintSpoolerInternalStateProto psis, final int filterLevel) throws Exception {
        for (PrintJobInfoProto pji : psis.getPrintJobsList()) {
            verifyPrintJobInfoProto(pji, filterLevel);
        }
        if (filterLevel == PRIVACY_AUTO) {
            assertTrue(0 == psis.getPrintJobFilesCount());
        }
    }

    static boolean supportsPrinting(ITestDevice device) throws DeviceNotAvailableException {
        return device.hasFeature("android.software.print");
    }
}
