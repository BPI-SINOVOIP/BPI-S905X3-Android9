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

package com.android.bips.ui;

import android.content.Context;
import android.preference.Preference;

import com.android.bips.BuiltInPrintService;
import com.android.bips.R;
import com.android.bips.discovery.DiscoveredPrinter;

class PrinterPreference extends Preference {
    private final BuiltInPrintService mPrintService;
    private DiscoveredPrinter mPrinter;
    private boolean mAdding = false;

    PrinterPreference(Context context, BuiltInPrintService printService,
            DiscoveredPrinter printer, boolean adding) {
        super(context);
        mPrintService = printService;
        mPrinter = printer;
        mAdding = adding;
        updatePrinter(printer);
    }

    void updatePrinter(DiscoveredPrinter printer) {
        mPrinter = printer;
        setLayoutResource(R.layout.printer_item);
        if (mAdding) {
            setTitle(getContext().getString(R.string.add_named, printer.name));
        } else {
            setTitle(printer.name);
        }

        setSummary(mPrintService.getDescription(printer));
        setIcon(R.drawable.ic_printer);
    }

    DiscoveredPrinter getPrinter() {
        return mPrinter;
    }
}
