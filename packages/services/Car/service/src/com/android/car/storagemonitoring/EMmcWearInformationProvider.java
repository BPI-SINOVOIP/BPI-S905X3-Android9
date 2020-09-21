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
import android.annotation.Nullable;
import android.util.Log;
import com.android.car.CarLog;
import com.android.internal.annotations.VisibleForTesting;
import java.io.File;
import java.io.FileReader;
import java.io.BufferedReader;
import java.io.IOException;

/**
 * Loads wear information from the eMMC sysfs entry points.
 * sysfs exposes eMMC lifetime data in /sys/bus/mmc/devices/mmc0:0001/{pre_eol_info|life_time}
 * The first file contains the value of the PRE_EOL_INFO register as an hexadecimal string
 * The second file contains the values of the LIFE_TIME_EST A and B registers as two hexadecimal
 * strings separated by a space.
 */
public class EMmcWearInformationProvider implements WearInformationProvider {
    private static File DEFAULT_LIFE_TIME_FILE =
        new File("/sys/bus/mmc/devices/mmc0:0001/life_time");

    private static File DEFAULT_PRE_EOL_FILE =
        new File("/sys/bus/mmc/devices/mmc0:0001/pre_eol_info");

    private File mLifetimeFile;
    private File mPreEolFile;

    public EMmcWearInformationProvider() {
        this(DEFAULT_LIFE_TIME_FILE, DEFAULT_PRE_EOL_FILE);
    }

    @VisibleForTesting
    EMmcWearInformationProvider(@NonNull File lifetimeFile, @NonNull File preEolFile) {
        mLifetimeFile = lifetimeFile;
        mPreEolFile = preEolFile;
    }

    private String readLineFromFile(File f) {
        if (!f.exists() || !f.isFile()) {
            Log.i(CarLog.TAG_STORAGE, f + " does not exist or is not a file");
            return null;
        }

        try {
            BufferedReader reader = new BufferedReader(new FileReader(f));
            String data = reader.readLine();
            reader.close();
            return data;
        } catch (IOException e) {
            Log.w(CarLog.TAG_STORAGE, f + " cannot be read from", e);
            return null;
        }
    }

    @Nullable
    @Override
    public WearInformation load() {
        String lifetimeData = readLineFromFile(mLifetimeFile);
        String eolData = readLineFromFile(mPreEolFile);

        if ((lifetimeData == null) || (eolData == null)) {
            return null;
        }

        String[] lifetimes = lifetimeData.split(" ");
        if (lifetimes.length != 2) {
            Log.w(CarLog.TAG_STORAGE, "lifetime data not in expected format: " + lifetimeData);
            return null;
        }

        int lifetimeA;
        int lifetimeB;
        int eol;

        try {
            lifetimeA = Integer.decode(lifetimes[0]);
            lifetimeB = Integer.decode(lifetimes[1]);
            eol = Integer.decode("0x" + eolData);
        } catch (NumberFormatException e) {
            Log.w(CarLog.TAG_STORAGE,
                    "lifetime data not in expected format: " + lifetimeData, e);
            return null;
        }

        return new WearInformation(convertLifetime(lifetimeA),
                convertLifetime(lifetimeB),
                adjustEol(eol));
    }
}
