/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.permission.cts;

import static android.app.AppOpsManager.MODE_ALLOWED;
import static android.app.AppOpsManager.MODE_DEFAULT;
import static android.app.AppOpsManager.MODE_ERRORED;
import static android.app.AppOpsManager.MODE_IGNORED;
import static android.app.AppOpsManager.OPSTR_READ_CALENDAR;
import static android.app.AppOpsManager.OPSTR_READ_SMS;
import static android.app.AppOpsManager.OPSTR_RECORD_AUDIO;
import static com.android.compatibility.common.util.AppOpsUtils.allowedOperationLogged;
import static com.android.compatibility.common.util.AppOpsUtils.rejectedOperationLogged;
import static com.android.compatibility.common.util.AppOpsUtils.setOpMode;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyZeroInteractions;

import android.Manifest.permission;
import android.app.AppOpsManager;
import android.app.AppOpsManager.OnOpChangedListener;
import android.content.Context;
import android.os.Process;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.compatibility.common.util.AppOpsUtils;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class AppOpsTest extends InstrumentationTestCase {
    // Notifying OnOpChangedListener callbacks is an async operation, so we define a timeout.
    private static final int MODE_WATCHER_TIMEOUT_MS = 5000;

    private AppOpsManager mAppOps;
    private Context mContext;
    private String mOpPackageName;
    private int mMyUid;

    // These permissions and opStrs must map to the same op codes.
    private static Map<String, String> permissionToOpStr = new HashMap<>();

    static {
        permissionToOpStr.put(permission.ACCESS_COARSE_LOCATION,
                AppOpsManager.OPSTR_COARSE_LOCATION);
        permissionToOpStr.put(permission.ACCESS_FINE_LOCATION, AppOpsManager.OPSTR_FINE_LOCATION);
        permissionToOpStr.put(permission.READ_CONTACTS, AppOpsManager.OPSTR_READ_CONTACTS);
        permissionToOpStr.put(permission.WRITE_CONTACTS, AppOpsManager.OPSTR_WRITE_CONTACTS);
        permissionToOpStr.put(permission.READ_CALL_LOG, AppOpsManager.OPSTR_READ_CALL_LOG);
        permissionToOpStr.put(permission.WRITE_CALL_LOG, AppOpsManager.OPSTR_WRITE_CALL_LOG);
        permissionToOpStr.put(permission.READ_CALENDAR, AppOpsManager.OPSTR_READ_CALENDAR);
        permissionToOpStr.put(permission.WRITE_CALENDAR, AppOpsManager.OPSTR_WRITE_CALENDAR);
        permissionToOpStr.put(permission.CALL_PHONE, AppOpsManager.OPSTR_CALL_PHONE);
        permissionToOpStr.put(permission.READ_SMS, AppOpsManager.OPSTR_READ_SMS);
        permissionToOpStr.put(permission.RECEIVE_SMS, AppOpsManager.OPSTR_RECEIVE_SMS);
        permissionToOpStr.put(permission.RECEIVE_MMS, AppOpsManager.OPSTR_RECEIVE_MMS);
        permissionToOpStr.put(permission.RECEIVE_WAP_PUSH, AppOpsManager.OPSTR_RECEIVE_WAP_PUSH);
        permissionToOpStr.put(permission.SEND_SMS, AppOpsManager.OPSTR_SEND_SMS);
        permissionToOpStr.put(permission.READ_SMS, AppOpsManager.OPSTR_READ_SMS);
        permissionToOpStr.put(permission.WRITE_SETTINGS, AppOpsManager.OPSTR_WRITE_SETTINGS);
        permissionToOpStr.put(permission.SYSTEM_ALERT_WINDOW,
                AppOpsManager.OPSTR_SYSTEM_ALERT_WINDOW);
        permissionToOpStr.put(permission.ACCESS_NOTIFICATIONS,
                AppOpsManager.OPSTR_ACCESS_NOTIFICATIONS);
        permissionToOpStr.put(permission.CAMERA, AppOpsManager.OPSTR_CAMERA);
        permissionToOpStr.put(permission.RECORD_AUDIO, AppOpsManager.OPSTR_RECORD_AUDIO);
        permissionToOpStr.put(permission.READ_PHONE_STATE, AppOpsManager.OPSTR_READ_PHONE_STATE);
        permissionToOpStr.put(permission.ADD_VOICEMAIL, AppOpsManager.OPSTR_ADD_VOICEMAIL);
        permissionToOpStr.put(permission.USE_SIP, AppOpsManager.OPSTR_USE_SIP);
        permissionToOpStr.put(permission.PROCESS_OUTGOING_CALLS,
                AppOpsManager.OPSTR_PROCESS_OUTGOING_CALLS);
        permissionToOpStr.put(permission.BODY_SENSORS, AppOpsManager.OPSTR_BODY_SENSORS);
        permissionToOpStr.put(permission.READ_CELL_BROADCASTS,
                AppOpsManager.OPSTR_READ_CELL_BROADCASTS);
        permissionToOpStr.put(permission.READ_EXTERNAL_STORAGE,
                AppOpsManager.OPSTR_READ_EXTERNAL_STORAGE);
        permissionToOpStr.put(permission.WRITE_EXTERNAL_STORAGE,
                AppOpsManager.OPSTR_WRITE_EXTERNAL_STORAGE);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = getInstrumentation().getContext();
        mAppOps = (AppOpsManager) mContext.getSystemService(Context.APP_OPS_SERVICE);
        mOpPackageName = mContext.getOpPackageName();
        mMyUid = Process.myUid();
        assertNotNull(mAppOps);

        // Reset app ops state for this test package to the system default.
        AppOpsUtils.reset(mOpPackageName);
    }

    public void testNoteOpAndCheckOp() throws Exception {
        setOpMode(mOpPackageName, OPSTR_READ_SMS, MODE_ALLOWED);
        assertEquals(MODE_ALLOWED, mAppOps.noteOp(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        assertEquals(MODE_ALLOWED, mAppOps.noteOpNoThrow(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        assertEquals(MODE_ALLOWED, mAppOps.checkOp(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        assertEquals(MODE_ALLOWED, mAppOps.checkOpNoThrow(OPSTR_READ_SMS, mMyUid, mOpPackageName));

        setOpMode(mOpPackageName, OPSTR_READ_SMS, MODE_IGNORED);
        assertEquals(MODE_IGNORED, mAppOps.noteOp(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        assertEquals(MODE_IGNORED, mAppOps.noteOpNoThrow(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        assertEquals(MODE_IGNORED, mAppOps.checkOp(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        assertEquals(MODE_IGNORED, mAppOps.checkOpNoThrow(OPSTR_READ_SMS, mMyUid, mOpPackageName));

        setOpMode(mOpPackageName, OPSTR_READ_SMS, MODE_DEFAULT);
        assertEquals(MODE_DEFAULT, mAppOps.noteOp(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        assertEquals(MODE_DEFAULT, mAppOps.noteOpNoThrow(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        assertEquals(MODE_DEFAULT, mAppOps.checkOp(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        assertEquals(MODE_DEFAULT, mAppOps.checkOpNoThrow(OPSTR_READ_SMS, mMyUid, mOpPackageName));

        setOpMode(mOpPackageName, OPSTR_READ_SMS, MODE_ERRORED);
        assertEquals(MODE_ERRORED, mAppOps.noteOpNoThrow(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        assertEquals(MODE_ERRORED, mAppOps.checkOpNoThrow(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        try {
            mAppOps.noteOp(OPSTR_READ_SMS, mMyUid, mOpPackageName);
            fail("SecurityException expected");
        } catch (SecurityException expected) {
        }
        try {
            mAppOps.checkOp(OPSTR_READ_SMS, mMyUid, mOpPackageName);
            fail("SecurityException expected");
        } catch (SecurityException expected) {
        }
    }

    public void testStartOpAndFinishOp() throws Exception {
        setOpMode(mOpPackageName, OPSTR_READ_SMS, MODE_ALLOWED);
        assertEquals(MODE_ALLOWED, mAppOps.startOp(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        mAppOps.finishOp(OPSTR_READ_SMS, mMyUid, mOpPackageName);
        assertEquals(MODE_ALLOWED, mAppOps.startOpNoThrow(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        mAppOps.finishOp(OPSTR_READ_SMS, mMyUid, mOpPackageName);

        setOpMode(mOpPackageName, OPSTR_READ_SMS, MODE_IGNORED);
        assertEquals(MODE_IGNORED, mAppOps.startOp(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        assertEquals(MODE_IGNORED, mAppOps.startOpNoThrow(OPSTR_READ_SMS, mMyUid, mOpPackageName));

        setOpMode(mOpPackageName, OPSTR_READ_SMS, MODE_DEFAULT);
        assertEquals(MODE_DEFAULT, mAppOps.startOp(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        assertEquals(MODE_DEFAULT, mAppOps.startOpNoThrow(OPSTR_READ_SMS, mMyUid, mOpPackageName));

        setOpMode(mOpPackageName, OPSTR_READ_SMS, MODE_ERRORED);
        assertEquals(MODE_ERRORED, mAppOps.startOpNoThrow(OPSTR_READ_SMS, mMyUid, mOpPackageName));
        try {
            mAppOps.startOp(OPSTR_READ_SMS, mMyUid, mOpPackageName);
            fail("SecurityException expected");
        } catch (SecurityException expected) {
        }
    }

    public void testCheckPackagePassesCheck() throws Exception {
        mAppOps.checkPackage(mMyUid, mOpPackageName);
        mAppOps.checkPackage(Process.SYSTEM_UID, "android");
    }

    public void testCheckPackageDoesntPassCheck() throws Exception {
        try {
            // Package name doesn't match UID.
            mAppOps.checkPackage(Process.SYSTEM_UID, mOpPackageName);
            fail("SecurityException expected");
        } catch (SecurityException expected) {
        }

        try {
            // Package name doesn't match UID.
            mAppOps.checkPackage(mMyUid, "android");
            fail("SecurityException expected");
        } catch (SecurityException expected) {
        }

        try {
            // Package name missing
            mAppOps.checkPackage(mMyUid, "");
            fail("SecurityException expected");
        } catch (SecurityException expected) {
        }
    }

    public void testWatchingMode() throws Exception {
        OnOpChangedListener watcher = mock(OnOpChangedListener.class);
        try {
            setOpMode(mOpPackageName, OPSTR_READ_SMS, MODE_ALLOWED);

            mAppOps.startWatchingMode(OPSTR_READ_SMS, mOpPackageName, watcher);

            // Make a change to the app op's mode.
            reset(watcher);
            setOpMode(mOpPackageName, OPSTR_READ_SMS, MODE_ERRORED);
            verify(watcher, timeout(MODE_WATCHER_TIMEOUT_MS))
                    .onOpChanged(OPSTR_READ_SMS, mOpPackageName);

            // Make another change to the app op's mode.
            reset(watcher);
            setOpMode(mOpPackageName, OPSTR_READ_SMS, MODE_ALLOWED);
            verify(watcher, timeout(MODE_WATCHER_TIMEOUT_MS))
                    .onOpChanged(OPSTR_READ_SMS, mOpPackageName);

            // Set mode to the same value as before - expect no call to the listener.
            reset(watcher);
            setOpMode(mOpPackageName, OPSTR_READ_SMS, MODE_ALLOWED);
            verifyZeroInteractions(watcher);

            mAppOps.stopWatchingMode(watcher);

            // Make a change to the app op's mode. Since we already stopped watching the mode, the
            // listener shouldn't be called.
            reset(watcher);
            setOpMode(mOpPackageName, OPSTR_READ_SMS, MODE_ERRORED);
            verifyZeroInteractions(watcher);
        } finally {
            // Clean up registered watcher.
            mAppOps.stopWatchingMode(watcher);
        }
    }

    @SmallTest
    public void testAllOpsHaveOpString() {
        Set<String> opStrs = new HashSet<>();
        for (String opStr : AppOpsManager.getOpStrs()) {
            assertNotNull("Each app op must have an operation string defined", opStr);
            opStrs.add(opStr);
        }
        assertEquals("Not all op strings are unique", AppOpsManager._NUM_OP, opStrs.size());
    }

    @SmallTest
    public void testOpCodesUnique() {
        String[] opStrs = AppOpsManager.getOpStrs();
        Set<Integer> opCodes = new HashSet<>();
        for (String opStr : opStrs) {
            opCodes.add(AppOpsManager.strOpToOp(opStr));
        }
        assertEquals("Not all app op codes are unique", opStrs.length, opCodes.size());
    }

    @SmallTest
    public void testPermissionMapping() {
        for (String permission : permissionToOpStr.keySet()) {
            testPermissionMapping(permission, permissionToOpStr.get(permission));
        }
    }

    private void testPermissionMapping(String permission, String opStr) {
        // Do the public value => internal op code lookups.
        String mappedOpStr = AppOpsManager.permissionToOp(permission);
        assertEquals(mappedOpStr, opStr);
        int mappedOpCode = AppOpsManager.permissionToOpCode(permission);
        int mappedOpCode2 = AppOpsManager.strOpToOp(opStr);
        assertEquals(mappedOpCode, mappedOpCode2);

        // Do the internal op code => public value lookup (reverse lookup).
        String permissionMappedBack = AppOpsManager.opToPermission(mappedOpCode);
        assertEquals(permission, permissionMappedBack);
    }

    /**
     * Test that the app can not change the app op mode for itself.
     */
    @SmallTest
    public void testCantSetModeForSelf() {
        try {
            int writeSmsOp = AppOpsManager.permissionToOpCode("android.permission.WRITE_SMS");
            mAppOps.setMode(writeSmsOp, mMyUid, mOpPackageName, AppOpsManager.MODE_ALLOWED);
            fail("Was able to set mode for self");
        } catch (SecurityException expected) {
        }
    }

    @SmallTest
    public void testGetOpsForPackage_opsAreLogged() throws Exception {
        // This test checks if operations get logged by the system. It needs to start with a clean
        // slate, i.e. these ops can't have been logged previously for this test package. The reason
        // is that there's no API for clearing the app op logs before a test run. However, the op
        // logs are cleared when this test package is reinstalled between test runs. To make sure
        // that other test methods in this class don't affect this test method, here we use
        // operations that are not used by any other test cases.
        String mustNotBeLogged = "Operation mustn't be logged before the test runs";
        assertFalse(mustNotBeLogged, allowedOperationLogged(mOpPackageName, OPSTR_RECORD_AUDIO));
        assertFalse(mustNotBeLogged, allowedOperationLogged(mOpPackageName, OPSTR_READ_CALENDAR));

        setOpMode(mOpPackageName, OPSTR_RECORD_AUDIO, MODE_ALLOWED);
        setOpMode(mOpPackageName, OPSTR_READ_CALENDAR, MODE_ERRORED);

        // Note an op that's allowed.
        mAppOps.noteOp(OPSTR_RECORD_AUDIO, mMyUid, mOpPackageName);
        String mustBeLogged = "Operation must be logged";
        assertTrue(mustBeLogged, allowedOperationLogged(mOpPackageName, OPSTR_RECORD_AUDIO));

        // Note another op that's not allowed.
        mAppOps.noteOpNoThrow(OPSTR_READ_CALENDAR, mMyUid, mOpPackageName);
        assertTrue(mustBeLogged, allowedOperationLogged(mOpPackageName, OPSTR_RECORD_AUDIO));
        assertTrue(mustBeLogged, rejectedOperationLogged(mOpPackageName, OPSTR_READ_CALENDAR));
    }
}
