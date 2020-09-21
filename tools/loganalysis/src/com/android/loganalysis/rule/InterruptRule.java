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

package com.android.loganalysis.rule;

import com.android.loganalysis.item.BugreportItem;
import com.android.loganalysis.item.InterruptItem;
import com.android.loganalysis.item.InterruptItem.InterruptInfoItem;
import com.android.loganalysis.item.InterruptItem.InterruptCategory;

import java.util.concurrent.TimeUnit;
import java.util.ArrayList;
import java.util.List;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * Rules definition for Interrupt rule
 */
public class InterruptRule extends AbstractPowerRule {

    private static final String INTERRUPT_ANALYSIS = "INTERRUPT_ANALYSIS";
    private static final long INTERRUPT_THRESHOLD_MS = 120000;

    private List<InterruptInfoItem> mOffendingInterruptsList;

    public InterruptRule (BugreportItem bugreportItem) {
        super(bugreportItem);
    }

    @Override
    public void applyRule() {
        mOffendingInterruptsList = new ArrayList<InterruptInfoItem>();
        InterruptItem interruptItem = getDetailedAnalysisItem().getInterruptItem();
        if (interruptItem == null || getTimeOnBattery() < 0) {
            return;
        }
        for (InterruptInfoItem interrupts : interruptItem.getInterrupts()) {
            final long interruptsPerMs = getTimeOnBattery()/interrupts.getInterruptCount();
            if (interruptsPerMs < INTERRUPT_THRESHOLD_MS) {
                mOffendingInterruptsList.add(interrupts);
            }
        }
    }

    @Override
    public JSONObject getAnalysis() {
        JSONObject interruptAnalysis = new JSONObject();
        StringBuilder analysis = new StringBuilder();
        if (mOffendingInterruptsList == null || mOffendingInterruptsList.size() <= 0) {
            analysis.append(String.format(
                    "No interrupts woke up device more frequent than %d secs.",
                    TimeUnit.MILLISECONDS.toSeconds(INTERRUPT_THRESHOLD_MS)));
        } else {
            for (InterruptInfoItem interrupts : mOffendingInterruptsList) {
                if (interrupts.getCategory() != InterruptCategory.UNKNOWN_INTERRUPT) {
                    analysis.append(String.format("Frequent interrupts from %s (%s). ",
                            interrupts.getCategory(),
                            interrupts.getName()));
                } else {
                    analysis.append(String.format("Frequent interrupts from %s. ",
                            interrupts.getName()));
                }
            }
        }
        try {
            interruptAnalysis.put(INTERRUPT_ANALYSIS, analysis.toString().trim());
        } catch (JSONException e) {
            // do nothing
        }
        return interruptAnalysis;
    }
}
