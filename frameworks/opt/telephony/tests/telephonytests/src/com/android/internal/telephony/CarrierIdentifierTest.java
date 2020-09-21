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

package com.android.internal.telephony;

import static com.android.internal.telephony.TelephonyTestUtils.waitForMs;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doReturn;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.HandlerThread;
import android.provider.Telephony.CarrierId;
import android.provider.Telephony.Carriers;
import android.test.mock.MockContentProvider;
import android.test.mock.MockContentResolver;
import android.test.suitebuilder.annotation.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.Arrays;

public class CarrierIdentifierTest extends TelephonyTest {
    private static final String MCCMNC = "311480";
    private static final String NAME = "VZW";
    private static final int CID_VZW = 1;

    private static final String SPN_FI = "PROJECT FI";
    private static final String NAME_FI = "FI";
    private static final int CID_FI = 2;

    private static final String NAME_DOCOMO = "DOCOMO";
    private static final String APN_DOCOMO = "mopera.net";
    private static final int CID_DOCOMO = 3;

    private static final String NAME_TMO = "TMO";
    private static final String GID1 = "ae";
    private static final int CID_TMO = 4;

    private static final int CID_UNKNOWN = -1;

    // events to trigger carrier identification
    private static final int SIM_LOAD_EVENT       = 1;
    private static final int SIM_ABSENT_EVENT     = 2;
    private static final int SPN_OVERRIDE_EVENT   = 3;
    private static final int PREFER_APN_SET_EVENT = 5;

    private CarrierIdentifier mCarrierIdentifier;
    private CarrierIdentifierHandler mCarrierIdentifierHandler;

    private class CarrierIdentifierHandler extends HandlerThread {
        private CarrierIdentifierHandler(String name) {
            super(name);
        }

        @Override
        public void onLooperPrepared() {
            mCarrierIdentifier = new CarrierIdentifier(mPhone);
            setReady(true);
        }
    }

    @Before
    public void setUp() throws Exception {
        logd("CarrierIdentifierTest +Setup!");
        super.setUp(getClass().getSimpleName());
        ((MockContentResolver) mContext.getContentResolver()).addProvider(
                CarrierId.AUTHORITY, new CarrierIdContentProvider());
        // start handler thread
        mCarrierIdentifierHandler = new CarrierIdentifierHandler(getClass().getSimpleName());
        mCarrierIdentifierHandler.start();
        waitUntilReady();
        logd("CarrierIdentifierTest -Setup!");
    }

    @After
    public void tearDown() throws Exception {
        logd("CarrierIdentifier -tearDown");
        mCarrierIdentifier.removeCallbacksAndMessages(null);
        mCarrierIdentifier = null;
        mCarrierIdentifierHandler.quit();
        super.tearDown();
    }

    @Test
    @SmallTest
    public void testCarrierMatch() {
        int phoneId = mPhone.getPhoneId();
        doReturn(MCCMNC).when(mTelephonyManager).getSimOperatorNumericForPhone(eq(phoneId));
        // trigger sim loading event
        mCarrierIdentifier.sendEmptyMessage(SIM_LOAD_EVENT);
        waitForMs(200);
        assertEquals(CID_VZW, mCarrierIdentifier.getCarrierId());
        assertEquals(NAME, mCarrierIdentifier.getCarrierName());

        doReturn(SPN_FI).when(mTelephonyManager).getSimOperatorNameForPhone(eq(phoneId));
        mCarrierIdentifier.sendEmptyMessage(SIM_LOAD_EVENT);
        waitForMs(200);
        assertEquals(CID_FI, mCarrierIdentifier.getCarrierId());
        assertEquals(NAME_FI, mCarrierIdentifier.getCarrierName());

        doReturn(GID1).when(mPhone).getGroupIdLevel1();
        mCarrierIdentifier.sendEmptyMessage(SIM_LOAD_EVENT);
        waitForMs(200);
        assertEquals(CID_TMO, mCarrierIdentifier.getCarrierId());
        assertEquals(NAME_TMO, mCarrierIdentifier.getCarrierName());
    }

    @Test
    @SmallTest
    public void testCarrierMatchSpnOverride() {
        int phoneId = mPhone.getPhoneId();
        doReturn(MCCMNC).when(mTelephonyManager).getSimOperatorNumericForPhone(eq(phoneId));
        // trigger sim loading event
        mCarrierIdentifier.sendEmptyMessage(SIM_LOAD_EVENT);
        waitForMs(200);
        assertEquals(CID_VZW, mCarrierIdentifier.getCarrierId());
        assertEquals(NAME, mCarrierIdentifier.getCarrierName());
        // spn override
        doReturn(SPN_FI).when(mTelephonyManager).getSimOperatorNameForPhone(eq(phoneId));
        mCarrierIdentifier.sendEmptyMessage(SPN_OVERRIDE_EVENT);
        waitForMs(200);
        assertEquals(CID_FI, mCarrierIdentifier.getCarrierId());
        assertEquals(NAME_FI, mCarrierIdentifier.getCarrierName());
    }

    @Test
    @SmallTest
    public void testCarrierMatchSimAbsent() {
        int phoneId = mPhone.getPhoneId();
        doReturn(MCCMNC).when(mTelephonyManager).getSimOperatorNumericForPhone(eq(phoneId));
        // trigger sim loading event
        mCarrierIdentifier.sendEmptyMessage(SIM_LOAD_EVENT);
        waitForMs(200);
        assertEquals(CID_VZW, mCarrierIdentifier.getCarrierId());
        assertEquals(NAME, mCarrierIdentifier.getCarrierName());
        // trigger sim absent event
        mCarrierIdentifier.sendEmptyMessage(SIM_ABSENT_EVENT);
        waitForMs(200);
        assertEquals(CID_UNKNOWN, mCarrierIdentifier.getCarrierId());
        assertNull(mCarrierIdentifier.getCarrierName());
    }

    @Test
    @SmallTest
    public void testCarrierNoMatch() {
        // un-configured MCCMNC
        int phoneId = mPhone.getPhoneId();
        doReturn("12345").when(mTelephonyManager).getSimOperatorNumericForPhone(eq(phoneId));
        // trigger sim loading event
        mCarrierIdentifier.sendEmptyMessage(SIM_LOAD_EVENT);
        waitForMs(200);
        assertEquals(CID_UNKNOWN, mCarrierIdentifier.getCarrierId());
        assertNull(mCarrierIdentifier.getCarrierName());
    }

    @Test
    @SmallTest
    public void testCarrierMatchPreferApnChange() {
        int phoneId = mPhone.getPhoneId();
        doReturn(MCCMNC).when(mTelephonyManager).getSimOperatorNumericForPhone(eq(phoneId));
        // trigger sim loading event
        mCarrierIdentifier.sendEmptyMessage(SIM_LOAD_EVENT);
        waitForMs(200);
        assertEquals(CID_VZW, mCarrierIdentifier.getCarrierId());
        assertEquals(NAME, mCarrierIdentifier.getCarrierName());
        // mock apn
        ((MockContentResolver) mContext.getContentResolver()).addProvider(
                Carriers.CONTENT_URI.getAuthority(), new CarrierIdContentProvider());
        mCarrierIdentifier.sendEmptyMessage(PREFER_APN_SET_EVENT);
        waitForMs(200);
        assertEquals(CID_DOCOMO, mCarrierIdentifier.getCarrierId());
        assertEquals(NAME_DOCOMO, mCarrierIdentifier.getCarrierName());
    }

    private class CarrierIdContentProvider extends MockContentProvider {
        @Override
        public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
                String sortOrder) {
            logd("CarrierIdContentProvider: query");
            logd("   uri = " + uri);
            logd("   projection = " + Arrays.toString(projection));
            logd("   selection = " + selection);
            logd("   selectionArgs = " + Arrays.toString(selectionArgs));
            logd("   sortOrder = " + sortOrder);

            if (CarrierId.All.CONTENT_URI.getAuthority().equals(
                    uri.getAuthority())) {
                MatrixCursor mc = new MatrixCursor(
                        new String[]{CarrierId._ID,
                                CarrierId.All.MCCMNC,
                                CarrierId.All.GID1,
                                CarrierId.All.GID2,
                                CarrierId.All.PLMN,
                                CarrierId.All.IMSI_PREFIX_XPATTERN,
                                CarrierId.All.ICCID_PREFIX,
                                CarrierId.All.SPN,
                                CarrierId.All.APN,
                                CarrierId.CARRIER_NAME,
                                CarrierId.CARRIER_ID});

                mc.addRow(new Object[] {
                        1,                      // id
                        MCCMNC,                 // mccmnc
                        null,                   // gid1
                        null,                   // gid2
                        null,                   // plmn
                        null,                   // imsi_prefix
                        null,                   // iccid_prefix
                        null,                   // spn
                        null,                   // apn
                        NAME,                   // carrier name
                        CID_VZW,                // cid
                });
                mc.addRow(new Object[] {
                        2,                      // id
                        MCCMNC,                 // mccmnc
                        GID1,                   // gid1
                        null,                   // gid2
                        null,                   // plmn
                        null,                   // imsi_prefix
                        null,                   // iccid_prefix
                        null,                   // spn
                        null,                   // apn
                        NAME_TMO,               // carrier name
                        CID_TMO,                // cid
                });
                mc.addRow(new Object[] {
                        3,                      // id
                        MCCMNC,                 // mccmnc
                        null,                   // gid1
                        null,                   // gid2
                        null,                   // plmn
                        null,                   // imsi_prefix
                        null,                   // iccid_prefix
                        SPN_FI,                 // spn
                        null,                   // apn
                        NAME_FI,                // carrier name
                        CID_FI,                 // cid
                });
                mc.addRow(new Object[] {
                        4,                      // id
                        MCCMNC,                 // mccmnc
                        null,                   // gid1
                        null,                   // gid2
                        null,                   // plmn
                        null,                   // imsi_prefix
                        null,                   // iccid_prefix
                        null,                   // spn
                        APN_DOCOMO,             // apn
                        NAME_DOCOMO,            // carrier name
                        CID_DOCOMO,             // cid
                });
                return mc;
            } else if (Carriers.CONTENT_URI.getAuthority().equals(uri.getAuthority())) {
                MatrixCursor mc = new MatrixCursor(new String[]{Carriers._ID, Carriers.APN});
                mc.addRow(new Object[] {
                        1,                      // id
                        APN_DOCOMO              // apn
                });
                return mc;
            }
            return null;
        }
        @Override
        public int update(android.net.Uri uri, android.content.ContentValues values,
                java.lang.String selection, java.lang.String[] selectionArgs) {
            return 0;
        }
    }
}
