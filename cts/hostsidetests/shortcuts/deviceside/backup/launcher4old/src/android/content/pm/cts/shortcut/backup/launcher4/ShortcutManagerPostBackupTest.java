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
package android.content.pm.cts.shortcut.backup.launcher4;

import static com.android.server.pm.shortcutmanagertest.ShortcutManagerTestUtils.assertWith;

import android.content.pm.ShortcutInfo;
import android.content.pm.cts.shortcut.device.common.ShortcutManagerDeviceTestBase;

public class ShortcutManagerPostBackupTest extends ShortcutManagerDeviceTestBase {
    @Override
    protected void setUp() throws Exception {
        super.setUp();

        setAsDefaultLauncher(MainActivity.class);
    }

    public void testRestoredOnOldVersion() {
        assertWith(getPackageShortcuts(ShortcutManagerPreBackupTest.PUBLISHER4_PKG))
                .haveIds("ms1", "ms2", "s1", "s2")
                .areAllPinned()

                .selectByIds("ms1", "ms2")
                .areAllEnabled()
                .areAllManifest()

                // s1 is re-published, so it's enabled and dynamic.
                .revertToOriginalList()
                .selectByIds("s1")
                .areAllEnabled()
                .areAllDynamic()
                .forAllShortcuts(si -> {
                    assertEquals("shortlabel1_new_one", si.getShortLabel());

                    // The app re-published the shortcut, not updated, so the fields that existed
                    // in the original shortcut is gone now.
                    assertNull(si.getExtras());
                })

                .revertToOriginalList()
                .selectByIds("s2")
                .areAllDisabled()
                .areAllWithDisabledReason(ShortcutInfo.DISABLED_REASON_VERSION_LOWER)
                .forAllShortcuts(si -> {
                    // Note the label shouldn't be updated with the updateShortcuts() call.
                    assertEquals("shortlabel2", si.getShortLabel());
                })
                .areAllNotDynamic();
    }

    public void testRestoredOnNewVersion() {
        assertWith(getPackageShortcuts(ShortcutManagerPreBackupTest.PUBLISHER4_PKG))
                .haveIds("ms1", "ms2", "s1", "s2")
                .areAllPinned()

                .selectByIds("ms1", "ms2")
                .areAllEnabled()
                .areAllManifest()

                // s1 is re-published, so it's enabled and dynamic.
                .revertToOriginalList()
                .selectByIds("s1", "s2")
                .areAllEnabled()

                .revertToOriginalList()
                .selectByIds("s1")
                .areAllDynamic()
                .forAllShortcuts(si -> {
                    assertEquals("shortlabel1_new_one", si.getShortLabel());

                    // The app re-published the shortcut, not updated, so the fields that existed
                    // in the original shortcut is gone now.
                    assertNull(si.getExtras());
                })

                .revertToOriginalList()
                .selectByIds("s2")
                .areAllNotDynamic()
                .forAllShortcuts(si -> {
                    assertEquals("shortlabel2_updated", si.getShortLabel());
                })
                ;
    }


    public void testRestoreWrongKey() {
        assertWith(getPackageShortcuts(ShortcutManagerPreBackupTest.PUBLISHER4_PKG))
                .haveIds("ms1", "ms2", "s1", "s2")
                .areAllPinned()

                .selectByIds("ms1", "ms2")
                .areAllEnabled()
                .areAllManifest()

                // s1 is re-published, so it's enabled and dynamic.
                .revertToOriginalList()
                .selectByIds("s1")
                .areAllEnabled()
                .areAllDynamic()
                .forAllShortcuts(si -> {
                    assertEquals("shortlabel1_new_one", si.getShortLabel());

                    // The app re-published the shortcut, not updated, so the fields that existed
                    // in the original shortcut is gone now.
                    assertNull(si.getExtras());
                })


                // updateShortcuts() shouldn't work on it, so it keeps the original label.
                .revertToOriginalList()
                .selectByIds("s2")
                .areAllNotDynamic()
                .forAllShortcuts(si -> {
                    assertEquals("shortlabel2", si.getShortLabel());
                })
        ;
    }

    public void testRestoreNoManifestOnOldVersion() {
        assertWith(getPackageShortcuts(ShortcutManagerPreBackupTest.PUBLISHER4_PKG))
                .haveIds("ms1", "ms2", "s1", "s2")
                .areAllPinned()

                .selectByIds("s1", "ms1")
                .areAllEnabled()
                .areAllDynamic()
                .areAllMutable()

                .revertToOriginalList()
                .selectByIds("s2", "ms2")
                .areAllDisabled();
    }

    public void testRestoreNoManifestOnNewVersion() {
        assertWith(getPackageShortcuts(ShortcutManagerPreBackupTest.PUBLISHER4_PKG))
                .haveIds("ms1", "ms2", "s1", "s2")
                .areAllPinned()

                .selectByIds("ms1", "ms2")
                .areAllDisabled()
                .areAllImmutable()
                .areAllWithDisabledReason(ShortcutInfo.DISABLED_REASON_APP_CHANGED)

                .revertToOriginalList()
                .selectByIds("s1", "s2")
                .areAllEnabled()
                .areAllMutable();
    }

    public void testInvisibleIgnored() {
        assertWith(getPackageShortcuts(ShortcutManagerPreBackupTest.PUBLISHER4_PKG))
                .haveIds("ms1", "ms2", "s1", "s2")

                .selectByIds("ms1", "s1", "s2")
                .areAllPinned()
                .areAllDisabled()
                .areAllWithDisabledReason(ShortcutInfo.DISABLED_REASON_VERSION_LOWER)
                .areAllNotDynamic()
                .areAllNotManifest()

                .revertToOriginalList()
                .selectByIds("ms2")
                .areAllEnabled()
                .areAllPinned()
                .areAllNotDynamic()
                .areAllNotManifest();
    }
}
