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
package com.google.android.car.kitchensink.storagelifetime;

import static android.system.OsConstants.O_APPEND;
import static android.system.OsConstants.O_RDWR;

import android.annotation.Nullable;
import android.car.Car;
import android.car.CarNotConnectedException;
import android.car.storagemonitoring.CarStorageMonitoringManager;
import android.car.storagemonitoring.CarStorageMonitoringManager.IoStatsListener;
import android.car.storagemonitoring.IoStats;
import android.car.storagemonitoring.IoStatsEntry;
import android.os.Bundle;
import android.os.StatFs;
import android.support.v4.app.Fragment;
import android.system.ErrnoException;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import com.google.android.car.kitchensink.KitchenSinkActivity;
import com.google.android.car.kitchensink.R;

import libcore.io.Libcore;

import java.io.File;
import java.io.FileDescriptor;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.List;

public class StorageLifetimeFragment extends Fragment {
    private static final String FILE_NAME = "storage.bin";
    private static final String TAG = "CAR.STORAGELIFETIME.KS";

    private static final int KILOBYTE = 1024;
    private static final int MEGABYTE = 1024 * 1024;

    private StatFs mStatFs;
    private KitchenSinkActivity mActivity;
    private TextView mStorageWearInfo;
    private ListView mStorageChangesHistory;
    private TextView mFreeSpaceInfo;
    private TextView mIoActivity;
    private CarStorageMonitoringManager mStorageManager;

    private final IoStatsListener mIoListener = new IoStatsListener() {
        @Override
        public void onSnapshot(IoStats snapshot) {
            if (mIoActivity != null) {
                mIoActivity.setText("");
                snapshot.getStats().forEach(uidIoStats -> {
                    final long bytesWrittenToStorage = uidIoStats.foreground.bytesWrittenToStorage +
                            uidIoStats.background.bytesWrittenToStorage;
                    final long fsyncCalls = uidIoStats.foreground.fsyncCalls +
                            uidIoStats.background.fsyncCalls;
                    if (bytesWrittenToStorage > 0 || fsyncCalls > 0) {
                        mIoActivity.append(String.format(
                            "uid = %d, runtime = %d, bytes writen to disk = %d, fsync calls = %d\n",
                            uidIoStats.uid,
                            uidIoStats.runtimeMillis,
                            bytesWrittenToStorage,
                            fsyncCalls));
                    }
                });
                final List<IoStatsEntry> totals;
                try {
                    totals = mStorageManager.getAggregateIoStats();
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Car not connected or not supported", e);
                    return;
                }

                final long totalBytesWrittenToStorage = totals.stream()
                        .mapToLong(stats -> stats.foreground.bytesWrittenToStorage +
                                stats.background.bytesWrittenToStorage)
                        .reduce(0L, (x,y)->x+y);
                final long totalFsyncCalls = totals.stream()
                        .mapToLong(stats -> stats.foreground.fsyncCalls +
                            stats.background.fsyncCalls)
                        .reduce(0L, (x,y)->x+y);

                mIoActivity.append(String.format(
                        "total bytes written to disk = %d, total fsync calls = %d",
                        totalBytesWrittenToStorage,
                        totalFsyncCalls));
            }
        }
    };

    // TODO(egranata): put this somewhere more useful than KitchenSink
    private static String preEolToString(int preEol) {
        switch (preEol) {
            case 1: return "normal";
            case 2: return "warning";
            case 3: return "urgent";
            default:
                return "unknown";
        }
    }

    private Path getFilePath() throws IOException {
        Path filePath = new File(mActivity.getFilesDir(), FILE_NAME).toPath();
        if (Files.notExists(filePath)) {
            Files.createFile(filePath);
        }
        return filePath;
    }

    private void writeBytesToFile(int size) {
        try {
            final Path filePath = getFilePath();
            byte[] data = new byte[size];
            SecureRandom.getInstanceStrong().nextBytes(data);
            Files.write(filePath,
                data,
                StandardOpenOption.APPEND);
        } catch (NoSuchAlgorithmException | IOException e) {
            Log.w(TAG, "could not append data", e);
        }
    }

    private void fsyncFile() {
        try {
            final Path filePath = getFilePath();
            FileDescriptor fd = Libcore.os.open(filePath.toString(), O_APPEND | O_RDWR, 0);
            if (!fd.valid()) {
                Log.w(TAG, "file descriptor is invalid");
                return;
            }
            // fill byteBuffer with arbitrary data in order to make an fsync() meaningful
            ByteBuffer byteBuffer = ByteBuffer.wrap(new byte[] {101, 110, 114, 105, 99, 111});
            Libcore.os.write(fd, byteBuffer);
            Libcore.os.fsync(fd);
            Libcore.os.close(fd);
        } catch (ErrnoException | IOException e) {
            Log.w(TAG, "could not fsync data", e);
        }
    }

    @Nullable
    @Override
    public View onCreateView(
            LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.storagewear, container, false);
        mActivity = (KitchenSinkActivity) getHost();
        mStorageWearInfo = view.findViewById(R.id.storage_wear_info);
        mStorageChangesHistory = view.findViewById(R.id.storage_events_list);
        mFreeSpaceInfo = view.findViewById(R.id.free_disk_space);
        mIoActivity = view.findViewById(R.id.last_io_snapshot);

        view.findViewById(R.id.write_one_kilobyte).setOnClickListener(
            v -> writeBytesToFile(KILOBYTE));

        view.findViewById(R.id.write_one_megabyte).setOnClickListener(
            v -> writeBytesToFile(MEGABYTE));

        view.findViewById(R.id.perform_fsync).setOnClickListener(
            v -> fsyncFile());

        return view;
    }

    private void reloadInfo() {
        try {
            mStatFs = new StatFs(mActivity.getFilesDir().getAbsolutePath());

            mStorageManager =
                (CarStorageMonitoringManager) mActivity.getCar().getCarManager(
                        Car.STORAGE_MONITORING_SERVICE);

            mStorageWearInfo.setText("Wear estimate: " +
                mStorageManager.getWearEstimate() + "\nPre EOL indicator: " +
                preEolToString(mStorageManager.getPreEolIndicatorStatus()));

            mStorageChangesHistory.setAdapter(new ArrayAdapter(mActivity,
                    R.layout.wear_estimate_change_textview,
                    mStorageManager.getWearEstimateHistory().toArray()));

            mFreeSpaceInfo.setText("Available blocks: " + mStatFs.getAvailableBlocksLong() +
                "\nBlock size: " + mStatFs.getBlockSizeLong() + " bytes" +
                "\nfor a total free space of: " +
                (mStatFs.getBlockSizeLong() * mStatFs.getAvailableBlocksLong() / MEGABYTE) + "MB");
        } catch (android.car.CarNotConnectedException|
                 android.support.car.CarNotConnectedException e) {
            Log.e(TAG, "Car not connected or not supported", e);
        }
    }

    private void registerListener() {
        try {
            mStorageManager.registerListener(mIoListener);
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Car not connected or not supported", e);
        }
    }

    private void unregisterListener() {
        try {
            mStorageManager.unregisterListener(mIoListener);
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Car not connected or not supported", e);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        reloadInfo();
        registerListener();
    }

    @Override
    public void onPause() {
        unregisterListener();
        super.onPause();
    }
}
