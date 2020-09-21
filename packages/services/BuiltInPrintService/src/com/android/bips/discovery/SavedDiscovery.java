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

package com.android.bips.discovery;

import android.net.Uri;
import android.util.JsonReader;
import android.util.JsonWriter;
import android.util.Log;

import com.android.bips.BuiltInPrintService;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * A {@link Discovery} subclass managing a list of {@link DiscoveredPrinter} objects, persisted
 * between application sessions
 */
public abstract class SavedDiscovery extends Discovery {
    private static final String TAG = SavedDiscovery.class.getSimpleName();
    private static final boolean DEBUG = false;

    // manualPrinters for backward-compatibility
    private static final List<String> PRINTER_LIST_NAMES = Arrays.asList("printers",
            "manualPrinters");

    private final File mCacheFile;
    private final List<DiscoveredPrinter> mSavedPrinters;

    SavedDiscovery(BuiltInPrintService printService) {
        super(printService);
        mCacheFile = new File(printService.getCacheDir(), getClass().getSimpleName() + ".json");
        mSavedPrinters = load();
    }

    /**
     * Add a persisted printer, returning true if a change occurred
     */
    boolean addSavedPrinter(DiscoveredPrinter printer) {
        Uri printerUri = printer.getUri();
        DiscoveredPrinter old = find(printerUri);
        if (old != null) {
            if (printer.equals(old)) {
                return false;
            }
            mSavedPrinters.remove(old);
        }

        mSavedPrinters.add(0, printer);
        save();
        return true;
    }

    /**
     * Return a DiscoveredPrinter having the specified URI, or null
     */
    private DiscoveredPrinter find(Uri printerUri) {
        for (DiscoveredPrinter printer : mSavedPrinters) {
            if (printer.getUri().equals(printerUri)) {
                return printer;
            }
        }
        return null;
    }

    /**
     * Remove the first saved printer matching the supplied printer path
     */
    @Override
    public void removeSavedPrinter(Uri printerPath) {
        for (DiscoveredPrinter printer : mSavedPrinters) {
            if (printer.path.equals(printerPath)) {
                mSavedPrinters.remove(printer);
                save();
                return;
            }
        }
    }

    /**
     * Return a non-modifiable list of saved printers
     */
    @Override
    public List<DiscoveredPrinter> getSavedPrinters() {
        return Collections.unmodifiableList(mSavedPrinters);
    }

    /**
     * Load the list from storage, or return an empty list
     */
    private List<DiscoveredPrinter> load() {
        List<DiscoveredPrinter> printers = new ArrayList<>();
        if (!mCacheFile.exists()) {
            return printers;
        }

        try (JsonReader reader = new JsonReader(new BufferedReader(new FileReader(mCacheFile)))) {
            reader.beginObject();
            while (reader.hasNext()) {
                String itemName = reader.nextName();
                if (PRINTER_LIST_NAMES.contains(itemName)) {
                    reader.beginArray();
                    while (reader.hasNext()) {
                        printers.add(new DiscoveredPrinter(reader));
                    }
                    reader.endArray();
                }
            }
            reader.endObject();
        } catch (IllegalStateException | IOException ignored) {
            Log.w(TAG, "Error while loading from " + mCacheFile, ignored);
        }
        if (DEBUG) Log.d(TAG, "Loaded size=" + printers.size() + " from " + mCacheFile);
        return printers;
    }

    /**
     * Save the current list to storage
     */
    private void save() {
        if (mCacheFile.exists()) {
            mCacheFile.delete();
        }

        try (JsonWriter writer = new JsonWriter(new BufferedWriter(new FileWriter(mCacheFile)))) {
            writer.beginObject();
            writer.name(PRINTER_LIST_NAMES.get(0));
            writer.beginArray();
            for (DiscoveredPrinter printer : mSavedPrinters) {
                printer.write(writer);
            }
            writer.endArray();
            writer.endObject();
        } catch (NullPointerException | IOException e) {
            Log.w(TAG, "Error while storing to " + mCacheFile, e);
        }
    }
}
