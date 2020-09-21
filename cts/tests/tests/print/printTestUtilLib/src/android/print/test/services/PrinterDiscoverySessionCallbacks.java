/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.print.test.services;

import android.os.CancellationSignal;
import android.print.PrinterId;
import android.printservice.CustomPrinterIconCallback;

import java.util.List;

public abstract class PrinterDiscoverySessionCallbacks {

    private StubbablePrinterDiscoverySession mSession;

    public void setSession(StubbablePrinterDiscoverySession session) {
        mSession = session;
    }

    public StubbablePrinterDiscoverySession getSession() {
        return mSession;
    }

    public abstract void onStartPrinterDiscovery(List<PrinterId> priorityList);

    public abstract void onStopPrinterDiscovery();

    public abstract void onValidatePrinters(List<PrinterId> printerIds);

    public abstract void onStartPrinterStateTracking(PrinterId printerId);

    public abstract void onRequestCustomPrinterIcon(PrinterId printerId,
            CancellationSignal cancellationSignal, CustomPrinterIconCallback callback);

    public abstract void onStopPrinterStateTracking(PrinterId printerId);

    public abstract void onDestroy();
}
