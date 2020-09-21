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

import android.printservice.PrintJob;

public abstract class PrintServiceCallbacks {

    private StubbablePrintService mService;

    public StubbablePrintService getService() {
        return mService;
    }

    public void setService(StubbablePrintService service) {
        mService = service;
    }

    public abstract PrinterDiscoverySessionCallbacks onCreatePrinterDiscoverySessionCallbacks();

    public abstract void onRequestCancelPrintJob(PrintJob printJob);

    public abstract void onPrintJobQueued(PrintJob printJob);
}
