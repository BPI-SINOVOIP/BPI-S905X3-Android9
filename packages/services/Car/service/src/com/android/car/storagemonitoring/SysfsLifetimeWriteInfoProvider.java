/*
 * Copyright (C) 2018 The Android Open Source Project
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
import android.car.storagemonitoring.LifetimeWriteInfo;
import android.util.Log;
import com.android.internal.annotations.VisibleForTesting;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;

/**
 * <p>Loads lifetime write data for mounted filesystems via sysfs.</p>
 *
 * <p>ext4 and f2fs currently offer this information via
 * /sys/fs/<i>type</i>/<i>partition</i>/lifetime_write_kbytes VFS entry points.</p>
 */
public class SysfsLifetimeWriteInfoProvider implements LifetimeWriteInfoProvider {
    private static final String TAG = SysfsLifetimeWriteInfoProvider.class.getSimpleName();

    private static final String DEFAULT_PATH = "/sys/fs/";
    private static final String[] KNOWN_FILESYSTEMS = new String[] {
        "ext4",
        "f2fs"
    };
    private static final String FILENAME = "lifetime_write_kbytes";

    private final File mWriteInfosPath;

    public SysfsLifetimeWriteInfoProvider() {
        this(new File(DEFAULT_PATH));
    }

    @VisibleForTesting
    SysfsLifetimeWriteInfoProvider(File writeInfosPath) {
        mWriteInfosPath = writeInfosPath;
    }

    @Nullable
    private LifetimeWriteInfo tryParse(File dir) {
        File writefile = new File(dir, FILENAME);
        if (!writefile.exists() || !writefile.isFile()) {
            Log.d(TAG, writefile + " not a valid source of lifetime writes");
            return null;
        }
        List<String> datalines;
        try {
            datalines = Files.readAllLines(writefile.toPath());
        } catch (IOException e) {
            Log.e(TAG, "unable to read write info from " + writefile, e);
            return null;
        }
        if (datalines == null || datalines.size() != 1) {
            Log.e(TAG, "unable to read valid write info from " + writefile);
            return null;
        }
        String data = datalines.get(0);
        try {
            long writtenBytes = 1024L * Long.parseLong(data);
            if (writtenBytes < 0) {
                Log.e(TAG, "file at location " + writefile +
                    " contained a negative data amount " + data + ". Ignoring.");
                return null;
            } else {
                return new LifetimeWriteInfo(
                    dir.getName(),
                    dir.getParentFile().getName(),
                    writtenBytes);
            }
        } catch (NumberFormatException e) {
            Log.e(TAG, "unable to read valid write info from " + writefile, e);
            return null;
        }
    }

    @Override
    @NonNull
    public LifetimeWriteInfo[] load() {
        List<LifetimeWriteInfo> writeInfos = new ArrayList<>();

        for (String fstype : KNOWN_FILESYSTEMS) {
            File fspath = new File(mWriteInfosPath, fstype);
            if (!fspath.exists() || !fspath.isDirectory()) continue;
            Arrays.stream(fspath.listFiles(File::isDirectory))
                .map(this::tryParse)
                .filter(Objects::nonNull)
                .forEach(writeInfos::add);
        }

        return writeInfos.toArray(new LifetimeWriteInfo[0]);
    }
}
