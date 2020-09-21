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
package com.android.cts.transferowner;

import static junit.framework.Assert.assertNotNull;
import static junit.framework.Assert.assertTrue;

import static org.junit.Assert.assertEquals;
import static org.testng.Assert.assertFalse;
import static org.testng.Assert.assertNull;
import static org.testng.Assert.assertThrows;

import android.app.admin.DeviceAdminReceiver;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.PersistableBundle;
import android.os.UserHandle;
import android.os.UserManager;
import android.support.test.InstrumentationRegistry;

import com.android.compatibility.common.util.BlockingBroadcastReceiver;

import org.junit.Before;
import org.junit.Test;

import java.util.Collections;
import java.util.Set;

public abstract class DeviceAndProfileOwnerTransferOutgoingTest {
    public static class BasicAdminReceiver extends DeviceAdminReceiver {
        public BasicAdminReceiver() {}

        @Override
        public void onTransferAffiliatedProfileOwnershipComplete(Context context, UserHandle user) {
            putBooleanPref(context, KEY_TRANSFER_AFFILIATED_PROFILE_COMPLETED_CALLED, true);
        }
    }

    private static final String TRANSFER_OWNER_INCOMING_PKG =
            "com.android.cts.transferownerincoming";
    private static final String TRANSFER_OWNER_INCOMING_TEST_RECEIVER_CLASS =
            "com.android.cts.transferowner.DeviceAndProfileOwnerTransferIncomingTest"
                    + "$BasicAdminReceiver";
    private static final String TRANSFER_OWNER_INCOMING_TEST_RECEIVER_NO_METADATA_CLASS =
            "com.android.cts.transferowner.DeviceAndProfileOwnerTransferIncomingTest"
                    + "$BasicAdminReceiverNoMetadata";
    private static final String ARE_PARAMETERS_SAVED = "ARE_PARAMETERS_SAVED";
    static final ComponentName INCOMING_COMPONENT_NAME =
            new ComponentName(
                    TRANSFER_OWNER_INCOMING_PKG, TRANSFER_OWNER_INCOMING_TEST_RECEIVER_CLASS);
    static final ComponentName INCOMING_NO_METADATA_COMPONENT_NAME =
            new ComponentName(TRANSFER_OWNER_INCOMING_PKG,
                    TRANSFER_OWNER_INCOMING_TEST_RECEIVER_NO_METADATA_CLASS);
    private static final ComponentName INVALID_TARGET_COMPONENT =
            new ComponentName("com.android.cts.intent.receiver", ".BroadcastIntentReceiver");
    private final static String SHARED_PREFERENCE_NAME = "shared-preference-name";
    static final String KEY_TRANSFER_AFFILIATED_PROFILE_COMPLETED_CALLED =
            "key-transfer-affiliated-completed-called";

    protected DevicePolicyManager mDevicePolicyManager;
    protected ComponentName mOutgoingComponentName;
    protected Context mContext;
    private String mOwnerChangedBroadcastAction;

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getTargetContext();
        mDevicePolicyManager = mContext.getSystemService(DevicePolicyManager.class);
        mOutgoingComponentName = new ComponentName(mContext, BasicAdminReceiver.class.getName());
    }

    protected final void setupTestParameters(String ownerChangedBroadcastAction) {
        mOwnerChangedBroadcastAction = ownerChangedBroadcastAction;
    }

    @Test
    public void testTransferSameAdmin() {
        PersistableBundle b = new PersistableBundle();
        assertThrows(
                IllegalArgumentException.class,
                () -> {
                    mDevicePolicyManager.transferOwnership(
                            mOutgoingComponentName, mOutgoingComponentName, b);
                });
    }

    @Test
    public void testTransferInvalidTarget() {
        PersistableBundle b = new PersistableBundle();
        assertThrows(
                IllegalArgumentException.class,
                () -> {
                    mDevicePolicyManager.transferOwnership(mOutgoingComponentName,
                            INVALID_TARGET_COMPONENT, b);
                });
    }

    @Test
    public void testTransferOwnershipChangedBroadcast() throws Throwable {
        BlockingBroadcastReceiver receiver = new BlockingBroadcastReceiver(mContext,
                mOwnerChangedBroadcastAction);
        try {
            receiver.register();
            PersistableBundle b = new PersistableBundle();
            mDevicePolicyManager.transferOwnership(mOutgoingComponentName,
                    INCOMING_COMPONENT_NAME, b);
            Intent intent = receiver.awaitForBroadcast();
            assertNotNull(intent);
        } finally {
            receiver.unregisterQuietly();
        }
    }

    abstract public void testTransferOwnership() throws Throwable;

    @Test
    public void testTransferOwnershipNullBundle() throws Throwable {
        mDevicePolicyManager.transferOwnership(mOutgoingComponentName,
                INCOMING_COMPONENT_NAME, null);
    }

    @Test
    public void testTransferOwnershipNoMetadata() throws Throwable {
        PersistableBundle b = new PersistableBundle();
        assertThrows(
                IllegalArgumentException.class,
                () -> {
                    mDevicePolicyManager.transferOwnership(mOutgoingComponentName,
                            INCOMING_NO_METADATA_COMPONENT_NAME, b);
                });
    }

    @Test
    public void testClearDisallowAddManagedProfileRestriction() {
        setUserRestriction(UserManager.DISALLOW_ADD_MANAGED_PROFILE, false);
    }

    @Test
    public void testAddDisallowAddManagedProfileRestriction() {
        setUserRestriction(UserManager.DISALLOW_REMOVE_MANAGED_PROFILE, true);
    }

    @Test
    public void testSetAffiliationId1() {
        setAffiliationId("id.number.1");
    }

    @Test
    public void testTransferOwnershipBundleSaved() throws Throwable {
        PersistableBundle b = new PersistableBundle();
        b.putBoolean(ARE_PARAMETERS_SAVED, true);
        mDevicePolicyManager.transferOwnership(mOutgoingComponentName, INCOMING_COMPONENT_NAME, b);
    }

    @Test
    public void testGetTransferOwnershipBundleOnlyCalledFromAdmin() throws Throwable {
        PersistableBundle b = new PersistableBundle();
        b.putBoolean(ARE_PARAMETERS_SAVED, true);
        mDevicePolicyManager.transferOwnership(mOutgoingComponentName, INCOMING_COMPONENT_NAME, b);
        assertThrows(SecurityException.class, mDevicePolicyManager::getTransferOwnershipBundle);
    }

    @Test
    public void testIsBundleNullNoTransfer() throws Throwable {
        assertNull(mDevicePolicyManager.getTransferOwnershipBundle());
    }

    private void setUserRestriction(String restriction, boolean add) {
        DevicePolicyManager dpm = mContext.getSystemService(DevicePolicyManager.class);
        if (add) {
            dpm.addUserRestriction(mOutgoingComponentName, restriction);
        } else {
            dpm.clearUserRestriction(mOutgoingComponentName, restriction);
        }
    }

    private void setAffiliationId(String id) {
        ComponentName admin = mOutgoingComponentName;
        DevicePolicyManager dpm = (DevicePolicyManager)
                mContext.getSystemService(Context.DEVICE_POLICY_SERVICE);
        Set<String> ids = Collections.singleton(id);
        dpm.setAffiliationIds(admin, ids);
        assertEquals(ids, dpm.getAffiliationIds(admin));
    }

    private static SharedPreferences getPrefs(Context context) {
        return context.getSharedPreferences(SHARED_PREFERENCE_NAME, Context.MODE_PRIVATE);
    }

    private static void putBooleanPref(Context context, String key, boolean value) {
        getPrefs(context).edit().putBoolean(key, value).apply();
    }

    protected static boolean getBooleanPref(Context context, String key) {
        return getPrefs(context).getBoolean(key, false);
    }
}
