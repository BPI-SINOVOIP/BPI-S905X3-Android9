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
package com.android.car.storagemonitoring;

import android.annotation.NonNull;
import android.car.storagemonitoring.WearEstimateChange;
import android.util.JsonWriter;
import com.android.car.CarStorageMonitoringService;
import com.android.car.R;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.function.BiPredicate;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * This class represents the entire history of flash wear changes as tracked
 * by CarStorageMonitoringService. It is a set of WearEstimateRecords.
 *
 * This is convertible to a list of WearEstimateChanges given policy about what constitutes
 * acceptable or not acceptable degradation across change events. This policy is subject to
 * modifications across OS versions, and as such it is not suitable for permanent storage.
 */
public class WearHistory {
    private final List<WearEstimateRecord> mWearHistory = new ArrayList<>();

    public WearHistory() {}

    WearHistory(@NonNull JSONObject jsonObject) throws JSONException {
        final JSONArray wearHistory = jsonObject.getJSONArray("wearHistory");
        for (int i = 0; i < wearHistory.length(); ++i) {
            JSONObject wearRecordJson = wearHistory.getJSONObject(i);
            WearEstimateRecord wearRecord = new WearEstimateRecord(wearRecordJson);
            add(wearRecord);
        }
    }

    public static WearHistory fromRecords(@NonNull WearEstimateRecord... records) {
        WearHistory wearHistory = new WearHistory();
        Arrays.stream(records).forEach(wearHistory::add);
        return wearHistory;
    }

    public static WearHistory fromJson(@NonNull File in) throws IOException, JSONException {
        JSONObject jsonObject = new JSONObject(new String(Files.readAllBytes(in.toPath())));
        return new WearHistory(jsonObject);
    }

    public void writeToJson(@NonNull JsonWriter out) throws IOException {
        out.beginObject();
        out.name("wearHistory").beginArray();
        for (WearEstimateRecord wearRecord : mWearHistory) {
            wearRecord.writeToJson(out);
        }
        out.endArray();
        out.endObject();
    }

    public boolean add(@NonNull WearEstimateRecord record) {
        if (record != null && mWearHistory.add(record)) {
            mWearHistory.sort((WearEstimateRecord o1, WearEstimateRecord o2) ->
                Long.valueOf(o1.getTotalCarServiceUptime()).compareTo(
                    o2.getTotalCarServiceUptime()));
            return true;
        }
        return false;
    }

    public int size() {
        return mWearHistory.size();
    }

    public WearEstimateRecord get(int i) {
        return mWearHistory.get(i);
    }

    public WearEstimateRecord getLast() {
        return get(size() - 1);
    }

    public List<WearEstimateChange> toWearEstimateChanges(
            long acceptableHoursPerOnePercentFlashWear) {
        // current technology allows us to detect wear in 10% increments
        final int WEAR_PERCENTAGE_INCREMENT = 10;
        final long acceptableWearRate = WEAR_PERCENTAGE_INCREMENT *
                Duration.ofHours(acceptableHoursPerOnePercentFlashWear).toMillis();
        final int numRecords = size();

        if (numRecords == 0) return Collections.emptyList();

        List<WearEstimateChange> result = new ArrayList<>();
        result.add(get(0).toWearEstimateChange(true));

        for (int i = 1; i < numRecords; ++i) {
            WearEstimateRecord previousRecord = get(i - 1);
            WearEstimateRecord currentRecord = get(i);
            final long timeForChange =
                    currentRecord.getTotalCarServiceUptime() -
                            previousRecord.getTotalCarServiceUptime();
            final boolean isAcceptableDegradation = timeForChange >= acceptableWearRate;
            result.add(currentRecord.toWearEstimateChange(isAcceptableDegradation));
        }

        return Collections.unmodifiableList(result);
    }

    @Override
    public boolean equals(Object other) {
        if (other instanceof WearHistory) {
            WearHistory wi = (WearHistory)other;
            return wi.mWearHistory.equals(mWearHistory);
        }
        return false;
    }

    @Override
    public int hashCode() {
        return mWearHistory.hashCode();
    }

    @Override
    public String toString() {
        return mWearHistory.stream().map(WearEstimateRecord::toString).reduce(
            "WearHistory[size = " + size() + "] -> ",
            (String s, String t) -> s + ", " + t);
    }
}
