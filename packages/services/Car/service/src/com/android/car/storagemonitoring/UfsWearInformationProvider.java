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
import java.io.IOException;
import java.util.List;
import java.util.Optional;
import java.util.Scanner;
import java.util.regex.MatchResult;
import java.util.regex.Pattern;

/**
 * Loads wear information from the UFS sysfs entry points.
 * sysfs exposes UFS lifetime data in /sys/devices/soc/624000.ufshc/health
 * The first line of the file contains the UFS version
 * Subsequent lines contains individual information points in the format:
 * Health Descriptor[Byte offset 0x%d]: %31s = 0x%hx
 * Of these we care about the key values bPreEOLInfo and bDeviceLifeTimeEstA bDeviceLifeTimeEstB
 */
public class UfsWearInformationProvider implements WearInformationProvider {
    private static File DEFAULT_FILE =
        new File("/sys/devices/soc/624000.ufshc/health");

    private File mFile;

    public UfsWearInformationProvider() {
        this(DEFAULT_FILE);
    }

    @VisibleForTesting
    public UfsWearInformationProvider(@NonNull File file) {
        mFile = file;
    }

    @Nullable
    @Override
    public WearInformation load() {
        List<String> lifetimeData;
        try {
            lifetimeData = java.nio.file.Files.readAllLines(mFile.toPath());
        } catch (IOException e) {
            Log.w(CarLog.TAG_STORAGE, "error reading " + mFile, e);
            return null;
        }
        if (lifetimeData == null || lifetimeData.size() < 4) {
            return null;
        }

        Pattern infoPattern = Pattern.compile(
                "Health Descriptor\\[Byte offset 0x\\d+\\]: (\\w+) = 0x([0-9a-fA-F]+)");

        Optional<Integer> lifetimeA = Optional.empty();
        Optional<Integer> lifetimeB = Optional.empty();
        Optional<Integer> eol = Optional.empty();

        for(String lifetimeInfo : lifetimeData) {
            Scanner scanner = new Scanner(lifetimeInfo);
            if (null == scanner.findInLine(infoPattern)) {
                continue;
            }
            MatchResult match = scanner.match();
            if (match.groupCount() != 2) {
                continue;
            }
            String name = match.group(1);
            String value = "0x" + match.group(2);
            try {
                switch (name) {
                    case "bPreEOLInfo":
                        eol = Optional.of(Integer.decode(value));
                        break;
                    case "bDeviceLifeTimeEstA":
                        lifetimeA = Optional.of(Integer.decode(value));
                        break;
                    case "bDeviceLifeTimeEstB":
                        lifetimeB = Optional.of(Integer.decode(value));
                        break;
                }
            } catch (NumberFormatException e) {
                Log.w(CarLog.TAG_STORAGE,
                    "trying to decode key " + name + " value " + value + " didn't parse properly", e);
            }
        }

        if (!lifetimeA.isPresent() || !lifetimeB.isPresent() || !eol.isPresent()) {
            return null;
        }

        return new WearInformation(convertLifetime(lifetimeA.get()),
            convertLifetime(lifetimeB.get()),
            adjustEol(eol.get()));
    }
}
