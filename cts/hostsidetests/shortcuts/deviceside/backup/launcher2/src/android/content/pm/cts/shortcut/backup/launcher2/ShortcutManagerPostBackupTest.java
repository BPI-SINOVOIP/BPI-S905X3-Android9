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
package android.content.pm.cts.shortcut.backup.launcher2;

import static com.android.server.pm.shortcutmanagertest.ShortcutManagerTestUtils.assertWith;

import android.content.pm.ShortcutInfo;
import android.content.pm.cts.shortcut.device.common.ShortcutManagerDeviceTestBase;

public class ShortcutManagerPostBackupTest extends ShortcutManagerDeviceTestBase {
    @Override
    protected void setUp() throws Exception {
        super.setUp();

        setAsDefaultLauncher(MainActivity.class);
    }

    public void testWithUninstall_afterAppRestore() {
        assertWith(getPackageShortcuts(ShortcutManagerPreBackupTest.PUBLISHER1_PKG))
                .haveIds("ms1", "ms2", "s3")
                .areAllEnabled()

                .selectByIds("s3", "ms2")
                .areAllPinned()

                .revertToOriginalList()
                .selectByIds("ms1")
                .areAllNotPinned();

        // Note s3 and ms2 were disabled before backup, so they were not backed up.
        assertWith(getPackageShortcuts(ShortcutManagerPreBackupTest.PUBLISHER2_PKG))
                .haveIds("ms1", "ms2", "s2")
                .areAllEnabled()

                .selectByIds("s2", "ms1")
                .areAllPinned()

                .revertToOriginalList()
                .selectByIds("ms2")
                .areAllNotPinned();

        // Package3 doesn't support backup&restore.
        // However, the manifest-shortcuts will be republished anyway, so they're still pinned.
        // The dynamic shortcuts can't be restored, but we'll still restore them as disabled
        // shortcuts that are not visible to the publisher.
        assertWith(getPackageShortcuts(ShortcutManagerPreBackupTest.PUBLISHER3_PKG))
                .haveIds("ms1", "ms2", "s2", "s3")
                .areAllPinned()

                .selectByIds("ms1", "ms2")
                .areAllEnabled()
                .areAllWithDisabledReason(ShortcutInfo.DISABLED_REASON_NOT_DISABLED)

                .revertToOriginalList()
                .selectByIds("s2", "s3")
                .areAllDisabled()
                .areAllWithDisabledReason(ShortcutInfo.DISABLED_REASON_BACKUP_NOT_SUPPORTED)
        ;
    }
}
