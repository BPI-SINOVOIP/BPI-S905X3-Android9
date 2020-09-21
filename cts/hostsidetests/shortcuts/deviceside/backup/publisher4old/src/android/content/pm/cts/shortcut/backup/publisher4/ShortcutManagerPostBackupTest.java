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
package android.content.pm.cts.shortcut.backup.publisher4;

import static com.android.server.pm.shortcutmanagertest.ShortcutManagerTestUtils.assertWith;
import static com.android.server.pm.shortcutmanagertest.ShortcutManagerTestUtils.list;
import static com.android.server.pm.shortcutmanagertest.ShortcutManagerTestUtils.setDefaultLauncher;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ShortcutInfo;
import android.content.pm.cts.shortcut.device.common.ShortcutManagerDeviceTestBase;
import android.os.PersistableBundle;
import android.text.TextUtils;

import java.security.SecureRandom;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class ShortcutManagerPostBackupTest extends ShortcutManagerDeviceTestBase {
    public void testRestoredOnOldVersion() {
        assertWith(getManager().getDynamicShortcuts())
                .isEmpty();

        assertWith(getManager().getPinnedShortcuts())
                .haveIds("ms1", "ms2")
                .areAllEnabled();

        assertWith(getManager().getManifestShortcuts())
                .haveIds("ms1", "ms2")
                .areAllPinned()
                .areAllEnabled();

        // At this point, s1 and s2 don't look to exist to the publisher, so it can publish a
        // dynamic shortcut and that should work.
        // But updateShortcuts() don't.

        final ShortcutInfo s1 = new ShortcutInfo.Builder(getContext(), "s1")
                .setShortLabel("shortlabel1_new_one")
                .setActivity(getActivity("MainActivity"))
                .setIntents(new Intent[]{new Intent("main")})
                .build();

        assertTrue(getManager().addDynamicShortcuts(list(s1)));

        final ShortcutInfo s2 = new ShortcutInfo.Builder(getContext(), "s2")
                .setShortLabel("shortlabel2_updated")
                .build();
        assertTrue(getManager().updateShortcuts(list(s2)));

        assertWith(getManager().getDynamicShortcuts())
                .haveIds("s1") // s2 not in the list.
                .areAllEnabled();

        assertWith(getManager().getPinnedShortcuts())
                .haveIds("ms1", "ms2", "s1") // s2 not in the list.
                .areAllEnabled();

    }

    public void testRestoredOnNewVersion() {
        assertWith(getManager().getDynamicShortcuts())
                .haveIds("s1")
                .areAllEnabled()
                .areAllPinned();

        assertWith(getManager().getManifestShortcuts())
                .haveIds("ms1", "ms2")
                .areAllPinned()
                .areAllEnabled();

        assertWith(getManager().getPinnedShortcuts())
                .haveIds("ms1", "ms2", "s1", "s2")
                .areAllEnabled();

        // This time, update should work.
        final ShortcutInfo s2 = new ShortcutInfo.Builder(getContext(), "s2")
                .setShortLabel("shortlabel2_updated")
                .build();
        assertTrue(getManager().updateShortcuts(list(s2)));

        assertWith(getManager().getPinnedShortcuts())
                .selectByIds("s2")
                .forAllShortcuts(si -> {
                    assertEquals("shortlabel2_updated", si.getShortLabel());
                });
    }

    public void testBackupDisabled() {
        assertWith(getManager().getDynamicShortcuts())
                .isEmpty();

        assertWith(getManager().getPinnedShortcuts())
                .haveIds("ms1", "ms2")
                .areAllEnabled();

        assertWith(getManager().getManifestShortcuts())
                .haveIds("ms1", "ms2")
                .areAllPinned()
                .areAllEnabled();

        // Backup was disabled, so either s1 nor s2 look to be restored to the app.
        // So when the app publishes s1, it'll be published as new.
        // But it'll still be pinned.
        final ShortcutInfo s1 = new ShortcutInfo.Builder(getContext(), "s1")
                .setShortLabel("shortlabel1_new_one")
                .setActivity(getActivity("MainActivity"))
                .setIntents(new Intent[]{new Intent("main")})
                .build();

        assertTrue(getManager().addDynamicShortcuts(list(s1)));

        assertWith(getManager().getDynamicShortcuts())
                .haveIds("s1") // s2 not in the list.
                .areAllPinned()
                .areAllEnabled()
                .forAllShortcuts(si -> {
                    // The app re-published the shortcut, not updated, so the fields that existed
                    // in the original shortcut is gone now.
                    assertNull(si.getExtras());
                });
    }

    public void testRestoreWrongKey() {
        // Restored pinned shortcuts are from a package with a different signature, so the dynamic
        // pinned shortcuts should be disabled-invisible.

        assertWith(getManager().getDynamicShortcuts())
                .isEmpty();

        assertWith(getManager().getPinnedShortcuts())
                .haveIds("ms1", "ms2");

        assertWith(getManager().getManifestShortcuts())
                .haveIds("ms1", "ms2")
                .areAllPinned()
                .areAllEnabled();

        final ShortcutInfo s1 = new ShortcutInfo.Builder(getContext(), "s1")
                .setShortLabel("shortlabel1_new_one")
                .setActivity(getActivity("MainActivity"))
                .setIntents(new Intent[]{new Intent("main")})
                .build();

        assertTrue(getManager().addDynamicShortcuts(list(s1)));

        final ShortcutInfo s2 = new ShortcutInfo.Builder(getContext(), "s2")
                .setShortLabel("shortlabel2_updated")
                .build();
        assertTrue(getManager().updateShortcuts(list(s2)));

        assertWith(getManager().getDynamicShortcuts())
                .haveIds("s1") // s2 not in the list.
                .areAllEnabled();
    }

    /**
     * Restored on an older version that have no manifest shortcuts.
     *
     * In this case, the publisher wouldn't see the manifest shortcuts, and they're overwritable.
     */
    public void testRestoreNoManifestOnOldVersion() {
        assertWith(getManager().getManifestShortcuts())
                .isEmpty();

        assertWith(getManager().getDynamicShortcuts())
                .isEmpty();

        assertWith(getManager().getPinnedShortcuts())
                .isEmpty();

        // ms1 was manifest/immutable, but can be overwritten.
        final ShortcutInfo ms1 = new ShortcutInfo.Builder(getContext(), "ms1")
                .setShortLabel("ms1_new_one")
                .setActivity(getActivity("MainActivity"))
                .setIntents(new Intent[]{new Intent("main")})
                .build();

        assertTrue(getManager().setDynamicShortcuts(list(ms1)));

        assertWith(getManager().getDynamicShortcuts())
                .haveIds("ms1");
        assertWith(getManager().getPinnedShortcuts())
                .haveIds("ms1");

        // Adding s1 should also work.
        final ShortcutInfo s1 = new ShortcutInfo.Builder(getContext(), "s1")
                .setShortLabel("s1_new_one")
                .setActivity(getActivity("MainActivity"))
                .setIntents(new Intent[]{new Intent("main")})
                .build();

        assertTrue(getManager().addDynamicShortcuts(list(s1)));

        assertWith(getManager().getDynamicShortcuts())
                .haveIds("s1", "ms1");
        assertWith(getManager().getPinnedShortcuts())
                .haveIds("s1", "ms1");

        // Update on ms2 should be no-op.
        final ShortcutInfo ms2 = new ShortcutInfo.Builder(getContext(), "ms2")
                .setShortLabel("ms2-updated")
                .build();
        assertTrue(getManager().updateShortcuts(list(ms2)));

        assertWith(getManager().getManifestShortcuts())
                .isEmpty();
        assertWith(getManager().getDynamicShortcuts())
                .haveIds("s1", "ms1")
                .areAllEnabled()
                .areAllPinned()
                .areAllMutable()

                .selectByIds("s1")
                .forAllShortcuts(si -> {
                    assertEquals("s1_new_one", si.getShortLabel());
                })

                .revertToOriginalList()
                .selectByIds("ms1")
                .forAllShortcuts(si -> {
                    assertEquals("ms1_new_one", si.getShortLabel());
                });
    }

    /**
     * Restored on the same (or newer) version that have no manifest shortcuts.
     *
     * In this case, the publisher will see the manifest shortcuts (as immutable pinned),
     * and they *cannot* be overwritten.
     */
    public void testRestoreNoManifestOnNewVersion() {
        assertWith(getManager().getManifestShortcuts())
                .isEmpty();

        assertWith(getManager().getDynamicShortcuts())
                .isEmpty();

        assertWith(getManager().getPinnedShortcuts())
                .haveIds("ms1", "ms2", "s1", "s2")
                .selectByIds("ms1", "ms2")
                .areAllDisabled()

                .revertToOriginalList()
                .selectByIds("s1", "s2")
                .areAllEnabled();
    }

    private void assertNoShortcuts() {
        assertWith(getManager().getDynamicShortcuts()).isEmpty();
        assertWith(getManager().getPinnedShortcuts()).isEmpty();
        assertWith(getManager().getManifestShortcuts()).isEmpty();
    }

    public void testInvisibleIgnored() throws Exception {
        assertNoShortcuts();

        // Make sure "disable" won't change the disabled reason. Also make sure "enable" won't
        // enable them.
        getManager().disableShortcuts(list("s1", "s2", "ms1"));
        assertNoShortcuts();

        getManager().enableShortcuts(list("ms1", "s2"));
        assertNoShortcuts();

        getManager().enableShortcuts(list("ms1"));
        assertNoShortcuts();

        getManager().removeDynamicShortcuts(list("s1", "ms1"));
        assertNoShortcuts();

        getManager().removeAllDynamicShortcuts();
        assertNoShortcuts();


        // Force launcher 4 to be the default launcher so it'll receive the pin request.
        setDefaultLauncher(getInstrumentation(),
                "android.content.pm.cts.shortcut.backup.launcher4/.MainActivity");

        // Update, set and add have been tested already, so let's test "pin".

        final CountDownLatch latch = new CountDownLatch(1);

        PersistableBundle pb = new PersistableBundle();
        pb.putBoolean("acceptit", true);

        final ShortcutInfo ms2 = new ShortcutInfo.Builder(getContext(), "ms2")
                .setShortLabel("ms2_new_one")
                .setActivity(getActivity("MainActivity"))
                .setIntents(new Intent[]{new Intent("main2")})
                .setExtras(pb)
                .build();

        final String myIntentAction = "cts-shortcut-intent_" + new SecureRandom().nextInt();
        final IntentFilter myFilter = new IntentFilter(myIntentAction);

        final BroadcastReceiver onResult = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                latch.countDown();
            }
        };
        getContext().registerReceiver(onResult, myFilter);
        assertTrue(getManager().requestPinShortcut(ms2,
                PendingIntent.getBroadcast(getContext(), 0, new Intent(myIntentAction),
                        PendingIntent.FLAG_CANCEL_CURRENT).getIntentSender()));

        assertTrue("Didn't receive requestPinShortcut() callback.",
                latch.await(30, TimeUnit.SECONDS));

        assertWith(getManager().getPinnedShortcuts())
                .haveIds("ms2")
                .areAllNotDynamic()
                .areAllNotManifest()
                .areAllMutable()
                .areAllPinned()
                .forAllShortcuts(si -> {
                    // requestPinShortcut() acts as an update in this case, so even though
                    // the original shortcut hada  long label, this one does not.
                    assertTrue(TextUtils.isEmpty(si.getLongLabel()));
                });
    }
}
