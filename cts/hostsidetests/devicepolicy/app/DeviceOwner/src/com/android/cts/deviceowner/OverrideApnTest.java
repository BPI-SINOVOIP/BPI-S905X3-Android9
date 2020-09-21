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

package com.android.cts.deviceowner;

import android.net.Uri;
import android.telephony.TelephonyManager;
import android.telephony.data.ApnSetting;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.List;

/**
 * Test override APN APIs.
 */
public class OverrideApnTest extends BaseDeviceOwnerTest {
    private static final String TEST_APN_NAME = "testApnName";
    private static final String UPDATE_APN_NAME = "updateApnName";
    private static final String TEST_ENTRY_NAME = "testEntryName";
    private static final String UPDATE_ETNRY_NAME = "updateEntryName";
    private static final String TEST_OPERATOR_NUMERIC = "123456789";
    private static final int TEST_PROXY_PORT = 123;
    private static final String TEST_PROXY_ADDRESS = "123.123.123.123";
    private static final Uri TEST_MMSC = Uri.parse("http://www.google.com");
    private static final String TEST_USER_NAME = "testUser";
    private static final String TEST_PASSWORD = "testPassword";
    private static final int TEST_AUTH_TYPE = ApnSetting.AUTH_TYPE_CHAP;
    private static final int TEST_APN_TYPE_BITMASK
        = ApnSetting.TYPE_DEFAULT | ApnSetting.TYPE_EMERGENCY;
    private static final int TEST_PROTOCOL = ApnSetting.PROTOCOL_IPV4V6;
    private static final int TEST_NETWORK_TYPE_BITMASK = TelephonyManager.NETWORK_TYPE_CDMA;
    private static final int TEST_MVNO_TYPE = ApnSetting.MVNO_TYPE_GID;
    private static final boolean TEST_ENABLED = true;

    private static final ApnSetting testApnFull;
    static {
        testApnFull = new ApnSetting.Builder()
            .setApnName(TEST_APN_NAME)
            .setEntryName(TEST_ENTRY_NAME)
            .setOperatorNumeric(TEST_OPERATOR_NUMERIC)
            .setProxyAddress(getProxyInetAddress(TEST_PROXY_ADDRESS))
            .setProxyPort(TEST_PROXY_PORT)
            .setMmsc(TEST_MMSC)
            .setMmsProxyAddress(getProxyInetAddress(TEST_PROXY_ADDRESS))
            .setMmsProxyPort(TEST_PROXY_PORT)
            .setUser(TEST_USER_NAME)
            .setPassword(TEST_PASSWORD)
            .setAuthType(TEST_AUTH_TYPE)
            .setApnTypeBitmask(TEST_APN_TYPE_BITMASK)
            .setProtocol(TEST_PROTOCOL)
            .setRoamingProtocol(TEST_PROTOCOL)
            .setNetworkTypeBitmask(TEST_NETWORK_TYPE_BITMASK)
            .setMvnoType(TEST_MVNO_TYPE)
            .setCarrierEnabled(TEST_ENABLED)
            .build();
    }

    private static InetAddress getProxyInetAddress(String proxyAddress) {
        try {
            return InetAddress.getByName(proxyAddress);
        } catch (UnknownHostException e) {
            return null;
        }
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        List <ApnSetting> apnList = mDevicePolicyManager.getOverrideApns(getWho());
        for (ApnSetting apn : apnList) {
            boolean deleted = mDevicePolicyManager.removeOverrideApn(getWho(), apn.getId());
            assertTrue("Failed to clean up override APNs.", deleted);
        }
        mDevicePolicyManager.setOverrideApnsEnabled(getWho(), false);
    }

    public void testAddGetRemoveOverrideApn() throws Exception {
        int insertedId = mDevicePolicyManager.addOverrideApn(getWho(), testApnFull);
        assertTrue(insertedId != 0);
        List <ApnSetting> apnList = mDevicePolicyManager.getOverrideApns(getWho());

        assertEquals(1, apnList.size());
        assertEquals(TEST_OPERATOR_NUMERIC, apnList.get(0).getOperatorNumeric());
        assertEquals(TEST_ENTRY_NAME, apnList.get(0).getEntryName());
        assertEquals(TEST_APN_NAME, apnList.get(0).getApnName());
        assertEquals(getProxyInetAddress(TEST_PROXY_ADDRESS), apnList.get(0).getProxyAddress());
        assertEquals(TEST_PROXY_PORT, apnList.get(0).getProxyPort());
        assertEquals(TEST_MMSC, apnList.get(0).getMmsc());
        assertEquals(getProxyInetAddress(TEST_PROXY_ADDRESS), apnList.get(0).getMmsProxyAddress());
        assertEquals(TEST_PROXY_PORT, apnList.get(0).getMmsProxyPort());
        assertEquals(TEST_USER_NAME, apnList.get(0).getUser());
        assertEquals(TEST_PASSWORD, apnList.get(0).getPassword());
        assertEquals(TEST_AUTH_TYPE, apnList.get(0).getAuthType());
        assertEquals(TEST_APN_TYPE_BITMASK, apnList.get(0).getApnTypeBitmask());
        assertEquals(TEST_PROTOCOL, apnList.get(0).getProtocol());
        assertEquals(TEST_PROTOCOL, apnList.get(0).getRoamingProtocol());
        assertEquals(TEST_ENABLED, apnList.get(0).isEnabled());
        assertEquals(TEST_MVNO_TYPE, apnList.get(0).getMvnoType());
        assertEquals(TEST_NETWORK_TYPE_BITMASK, apnList.get(0).getNetworkTypeBitmask());

        assertTrue(mDevicePolicyManager.removeOverrideApn(getWho(), insertedId));
        apnList = mDevicePolicyManager.getOverrideApns(getWho());
        assertEquals(0, apnList.size());
    }

    public void testRemoveOverrideApnFailsForIncorrectId() throws Exception {
        assertFalse(mDevicePolicyManager.removeOverrideApn(getWho(), -1));
    }

    public void testInsertConflictingApn() {
        mDevicePolicyManager.addOverrideApn(getWho(), testApnFull);
        int insertConflictingId = mDevicePolicyManager.addOverrideApn(getWho(), testApnFull);
        assertEquals(-1, insertConflictingId);
    }

    public void testUpdateConflictingApn() {
        mDevicePolicyManager.addOverrideApn(getWho(), testApnFull);

        final ApnSetting apnToBeUpdated = (new ApnSetting.Builder())
            .setEntryName(UPDATE_ETNRY_NAME)
            .setApnName(UPDATE_APN_NAME)
            .setApnTypeBitmask(ApnSetting.TYPE_DEFAULT)
            .build();
        int apnIdToUpdate = mDevicePolicyManager.addOverrideApn(getWho(), apnToBeUpdated);
        assertNotSame(-1, apnIdToUpdate);
        assertFalse(mDevicePolicyManager.updateOverrideApn(getWho(), apnIdToUpdate, testApnFull));
        assertTrue(mDevicePolicyManager.removeOverrideApn(getWho(), apnIdToUpdate));
    }

    public void testUpdateOverrideApn() throws Exception {
        int insertedId = mDevicePolicyManager.addOverrideApn(getWho(), testApnFull);
        assertNotSame(-1, insertedId);

        final ApnSetting updateApn = new ApnSetting.Builder()
            .setApnName(UPDATE_APN_NAME)
            .setEntryName(UPDATE_ETNRY_NAME)
            .setOperatorNumeric(TEST_OPERATOR_NUMERIC)
            .setProxyAddress(getProxyInetAddress(TEST_PROXY_ADDRESS))
            .setProxyPort(TEST_PROXY_PORT)
            .setMmsc(TEST_MMSC)
            .setMmsProxyAddress(getProxyInetAddress(TEST_PROXY_ADDRESS))
            .setMmsProxyPort(TEST_PROXY_PORT)
            .setUser(TEST_USER_NAME)
            .setPassword(TEST_PASSWORD)
            .setAuthType(TEST_AUTH_TYPE)
            .setApnTypeBitmask(TEST_APN_TYPE_BITMASK)
            .setProtocol(TEST_PROTOCOL)
            .setRoamingProtocol(TEST_PROTOCOL)
            .setNetworkTypeBitmask(TEST_NETWORK_TYPE_BITMASK)
            .setMvnoType(TEST_MVNO_TYPE)
            .setCarrierEnabled(TEST_ENABLED)
            .build();
        assertTrue(mDevicePolicyManager.updateOverrideApn(getWho(), insertedId, updateApn));

        List <ApnSetting> apnList = mDevicePolicyManager.getOverrideApns(getWho());

        assertEquals(1, apnList.size());
        assertEquals(TEST_OPERATOR_NUMERIC, apnList.get(0).getOperatorNumeric());
        assertEquals(UPDATE_ETNRY_NAME, apnList.get(0).getEntryName());
        assertEquals(UPDATE_APN_NAME, apnList.get(0).getApnName());
        assertEquals(getProxyInetAddress(TEST_PROXY_ADDRESS), apnList.get(0).getProxyAddress());
        assertEquals(TEST_PROXY_PORT, apnList.get(0).getProxyPort());
        assertEquals(TEST_MMSC, apnList.get(0).getMmsc());
        assertEquals(getProxyInetAddress(TEST_PROXY_ADDRESS), apnList.get(0).getMmsProxyAddress());
        assertEquals(TEST_PROXY_PORT, apnList.get(0).getMmsProxyPort());
        assertEquals(TEST_USER_NAME, apnList.get(0).getUser());
        assertEquals(TEST_PASSWORD, apnList.get(0).getPassword());
        assertEquals(TEST_AUTH_TYPE, apnList.get(0).getAuthType());
        assertEquals(TEST_APN_TYPE_BITMASK, apnList.get(0).getApnTypeBitmask());
        assertEquals(TEST_PROTOCOL, apnList.get(0).getProtocol());
        assertEquals(TEST_PROTOCOL, apnList.get(0).getRoamingProtocol());
        assertEquals(TEST_ENABLED, apnList.get(0).isEnabled());
        assertEquals(TEST_MVNO_TYPE, apnList.get(0).getMvnoType());

        assertTrue(mDevicePolicyManager.removeOverrideApn(getWho(), insertedId));
    }

    public void testOverrideApnDefaultValues() {
        final ApnSetting emptyApn = new ApnSetting.Builder()
            .setApnName(TEST_APN_NAME)
            .setEntryName(TEST_ENTRY_NAME)
            .setApnTypeBitmask(TEST_APN_TYPE_BITMASK)
            .build();

        int insertedId = mDevicePolicyManager.addOverrideApn(getWho(), emptyApn);
        assertNotSame(-1, insertedId);

        List <ApnSetting> apnList = mDevicePolicyManager.getOverrideApns(getWho());
        assertEquals(1, apnList.size());
        assertEquals("", apnList.get(0).getOperatorNumeric());
        assertEquals(TEST_ENTRY_NAME, apnList.get(0).getEntryName());
        assertEquals(TEST_APN_NAME, apnList.get(0).getApnName());
        assertEquals(null, apnList.get(0).getProxyAddress());
        assertEquals(-1, apnList.get(0).getProxyPort());
        assertEquals(null, apnList.get(0).getMmsc());
        assertEquals(null, apnList.get(0).getMmsProxyAddress());
        assertEquals(-1, apnList.get(0).getMmsProxyPort());
        assertEquals("", apnList.get(0).getUser());
        assertEquals("", apnList.get(0).getPassword());
        assertEquals(0, apnList.get(0).getAuthType());
        assertEquals(TEST_APN_TYPE_BITMASK, apnList.get(0).getApnTypeBitmask());
        assertEquals(-1, apnList.get(0).getProtocol());
        assertEquals(-1, apnList.get(0).getRoamingProtocol());
        assertEquals(false, apnList.get(0).isEnabled());
        assertEquals(-1, apnList.get(0).getMvnoType());

        assertTrue(mDevicePolicyManager.removeOverrideApn(getWho(), insertedId));
    }

    public void testBuildApnSettingFailsWithoutRequiredField() {
        assertNull(new ApnSetting.Builder()
            .setEntryName(UPDATE_ETNRY_NAME)
            .setApnName(UPDATE_APN_NAME)
            .build());

        assertNull(new ApnSetting.Builder()
            .setEntryName(UPDATE_ETNRY_NAME)
            .setApnName(UPDATE_APN_NAME)
            .setApnTypeBitmask(1 << 20)
            .build());

        assertNull(new ApnSetting.Builder()
            .setApnName(UPDATE_APN_NAME)
            .setApnTypeBitmask(TEST_APN_TYPE_BITMASK)
            .build());

        assertNull(new ApnSetting.Builder()
            .setEntryName(UPDATE_ETNRY_NAME)
            .setApnTypeBitmask(TEST_APN_TYPE_BITMASK)
            .build());
    }

    public void testSetAndGetOverrideApnEnabled() throws Exception {
        mDevicePolicyManager.setOverrideApnsEnabled(getWho(), true);
        assertTrue(mDevicePolicyManager.isOverrideApnEnabled(getWho()));
        mDevicePolicyManager.setOverrideApnsEnabled(getWho(), false);
        assertFalse(mDevicePolicyManager.isOverrideApnEnabled(getWho()));
    }
}
