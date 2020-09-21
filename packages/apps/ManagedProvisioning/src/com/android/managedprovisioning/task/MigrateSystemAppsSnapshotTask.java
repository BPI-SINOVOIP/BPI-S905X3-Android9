/*
 * Copyright 2018, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.managedprovisioning.task;

import android.content.Context;
import android.os.FileUtils;

import com.android.managedprovisioning.common.ProvisionLogger;
import com.android.managedprovisioning.task.nonrequiredapps.SystemAppsSnapshot;

import java.io.File;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MigrateSystemAppsSnapshotTask extends AbstractProvisioningTask {
    private static final Pattern XML_FILE_NAME_PATTERN = Pattern.compile("(\\d+)\\.xml");

    public MigrateSystemAppsSnapshotTask(Context context, Callback callback) {
        super(context, null, callback);
    }

    @Override
    public void run(int userId) {
        migrateIfNecessary();
    }

    /**
     * Snapshot files are renamed from {user_id}.xml to {user_serial_number}.xml and moved
     * to the new folder.
     */
    private void migrateIfNecessary() {
        File legacyFolder = SystemAppsSnapshot.getLegacyFolder(mContext);
        if (!legacyFolder.exists()) {
            return;
        }
        ProvisionLogger.logi("Found legacy system_apps folder, kick start migration.");
        SystemAppsSnapshot.getFolder(mContext).mkdirs();
        File[] files = legacyFolder.listFiles();
        for (File file : files) {
            String fileName = file.getName();
            Matcher matcher = XML_FILE_NAME_PATTERN.matcher(fileName);
            if (!matcher.find()) {
                ProvisionLogger.logw("Found invalid file during migration: " + fileName);
                continue;
            }

            int userId = Integer.parseInt(matcher.group(1));
            File destination;
            try {
                destination = SystemAppsSnapshot.getSystemAppsFile(mContext, userId);
            } catch (IllegalArgumentException ex) {
                ProvisionLogger.logi(
                        "user " + userId + " no longer exists, skip migrating its snapshot file");
                continue;
            }
            ProvisionLogger.logi(
                    "Moving " + file.getAbsolutePath() + " to " + destination.getAbsolutePath());
            boolean success = file.renameTo(destination);
            if (!success) {
                ProvisionLogger.loge("Failed to migrate " + file.getAbsolutePath());
            }
        }
        FileUtils.deleteContentsAndDir(legacyFolder);
    }

    @Override
    public int getStatusMsgId() {
        // OTA only task, not used.
        return 0;
    }
}
