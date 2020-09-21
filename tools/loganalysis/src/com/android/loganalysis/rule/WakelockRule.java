/*
 * Copyright (C) 2015 The Android Open Source Project
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
import com.android.loganalysis.item.WakelockItem;
import com.android.loganalysis.item.WakelockItem.WakelockInfoItem;
import com.android.loganalysis.util.NumberFormattingUtil;

import java.util.ArrayList;
import java.util.List;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * Rules definition for wakelock
 */
public class WakelockRule extends AbstractPowerRule {

    private static final String WAKELOCK_ANALYSIS = "WAKELOCK_ANALYSIS";
    private static final float WAKELOCK_HELD_TIME_THRESHOLD_PERCENTAGE = 0.1f; // 10%

    private List<WakelockInfoItem> mOffendingWakelockList;

    public WakelockRule (BugreportItem bugreportItem) {
        super(bugreportItem);
    }

    @Override
    public void applyRule() {
        mOffendingWakelockList = new ArrayList<WakelockInfoItem>();
        WakelockItem wakelockItem = getDetailedAnalysisItem().getWakelockItem();
        if (wakelockItem != null && getTimeOnBattery() > 0) {
            long wakelockThreshold =  (long)(getTimeOnBattery()
                    * WAKELOCK_HELD_TIME_THRESHOLD_PERCENTAGE);

            for (WakelockInfoItem wakelocks : wakelockItem.getWakeLocks()) {
                if (wakelocks.getHeldTime() > wakelockThreshold) {
                    mOffendingWakelockList.add(wakelocks);
                }
            }
        }
    }

    @Override
    public JSONObject getAnalysis() {
        JSONObject wakelockAnalysis = new JSONObject();

        StringBuilder analysis = new StringBuilder();
        if (mOffendingWakelockList == null || mOffendingWakelockList.size() <= 0) {
            analysis.append("No wakelocks were held for more than 10% of time on battery.");
        } else {
            for (WakelockInfoItem wakelocks : mOffendingWakelockList) {
                analysis.append(String.format("%s %s is held for %s. ", wakelocks.getName(),
                        wakelocks.getCategory(),
                        NumberFormattingUtil.getDuration(wakelocks.getHeldTime())));
            }
        }
        try {
            wakelockAnalysis.put(WAKELOCK_ANALYSIS, analysis.toString().trim());
        } catch (JSONException e) {
          // do nothing
        }
        return wakelockAnalysis;
    }
}
