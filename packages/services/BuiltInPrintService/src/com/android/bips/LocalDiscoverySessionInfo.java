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

package com.android.bips;

import android.print.PrinterId;
import android.printservice.PrintService;
import android.util.JsonReader;
import android.util.JsonWriter;
import android.util.Log;

import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Persistent information about discovery sessions
 */
class LocalDiscoverySessionInfo {
    private static final String TAG = LocalDiscoverySessionInfo.class.getSimpleName();
    private static final boolean DEBUG = false;

    private static final String CACHE_FILE = TAG + ".json";
    private static final String NAME_KNOWN_GOOD = "knownGood";
    private static final String NAME_PRIORITY = "priority";

    private static final int KNOWN_GOOD_MAX = 50;

    private final PrintService mService;
    private final File mCacheFile;
    private final List<PrinterId> mKnownGood = new ArrayList<>();
    private List<PrinterId> mPriority = new ArrayList<>();

    LocalDiscoverySessionInfo(PrintService service) {
        mService = service;
        mCacheFile = new File(service.getCacheDir(), CACHE_FILE);
        load();
    }

    /**
     * Load cached info from storage, if possible
     */
    private void load() {
        if (!mCacheFile.exists()) {
            return;
        }
        try (JsonReader reader = new JsonReader(new FileReader(mCacheFile))) {
            reader.beginObject();
            while (reader.hasNext()) {
                switch (reader.nextName()) {
                    case NAME_KNOWN_GOOD:
                        mKnownGood.addAll(loadPrinterIds(reader));
                        break;
                    case NAME_PRIORITY:
                        mPriority.addAll(loadPrinterIds(reader));
                        break;
                    default:
                        reader.skipValue();
                        break;
                }
            }
            reader.endObject();
        } catch (IOException e) {
            Log.w(TAG, "Failed to read info from " + CACHE_FILE, e);
        }
    }

    private List<PrinterId> loadPrinterIds(JsonReader reader) throws IOException {
        List<PrinterId> list = new ArrayList<>();
        reader.beginArray();
        while (reader.hasNext()) {
            String localId = reader.nextString();
            list.add(mService.generatePrinterId(localId));
        }
        reader.endArray();
        return list;
    }

    /**
     * Save cached info to storage, if possible
     */
    void save() {
        try (JsonWriter writer = new JsonWriter(new FileWriter(mCacheFile))) {
            writer.beginObject();

            writer.name(NAME_KNOWN_GOOD);
            savePrinterIds(writer, mKnownGood, KNOWN_GOOD_MAX);

            writer.name(NAME_PRIORITY);
            savePrinterIds(writer, mPriority, mPriority.size());

            writer.endObject();
        } catch (IOException e) {
            Log.w(TAG, "Failed to write known good list", e);
        }
    }

    private void savePrinterIds(JsonWriter writer, List<PrinterId> ids, int max)
            throws IOException {
        writer.beginArray();
        int count = Math.min(max, ids.size());
        for (int i = 0; i < count; i++) {
            writer.value(ids.get(i).getLocalId());
        }
        writer.endArray();
    }

    /**
     * Return true if the ID indicates a printer with a successful capability request in the past
     */
    boolean isKnownGood(PrinterId printerId) {
        return mKnownGood.contains(printerId);
    }

    void setKnownGood(PrinterId printerId) {
        mKnownGood.remove(printerId);
        mKnownGood.add(0, printerId);
    }
}
