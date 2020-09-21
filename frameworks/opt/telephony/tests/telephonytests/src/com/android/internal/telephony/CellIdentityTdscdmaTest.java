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

import android.os.Parcel;
import android.telephony.CellIdentity;
import android.telephony.CellIdentityTdscdma;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

/** Unit tests for {@link CellIdentityTdscdma}. */

public class CellIdentityTdscdmaTest extends AndroidTestCase {

    // Cell identity ranges from 0 to 268435456.
    private static final int CI = 268435456;
    // Physical cell id ranges from 0 to 503.
    private static final int PCI = 503;
    // Tracking area code ranges from 0 to 65535.
    private static final int TAC = 65535;
    // Absolute RF Channel Number ranges from 0 to 262140.
    private static final int EARFCN = 262140;
    private static final int MCC = 120;
    private static final int MNC = 260;
    private static final String MCC_STR = "120";
    private static final String MNC_STR = "260";
    private static final String ALPHA_LONG = "long";
    private static final String ALPHA_SHORT = "short";

    // Location Area Code ranges from 0 to 65535.
    private static final int LAC = 65535;
    // UMTS Cell Identity ranges from 0 to 268435455.
    private static final int CID = 268435455;

    private static final int CPID = 12345;


    @SmallTest
    public void testDefaultConstructor() {
        CellIdentityTdscdma ci =
                new CellIdentityTdscdma(MCC_STR, MNC_STR, LAC, CID, CPID);

        assertEquals(MCC_STR, ci.getMccString());
        assertEquals(MNC_STR, ci.getMncString());
        assertEquals(LAC, ci.getLac());
        assertEquals(CID, ci.getCid());
        assertEquals(CPID, ci.getCpid());
    }

    @SmallTest
    public void testConstructorWithEmptyMccMnc() {
        CellIdentityTdscdma ci = new CellIdentityTdscdma(null, null, LAC, CID, CPID);

        assertNull(ci.getMccString());
        assertNull(ci.getMncString());

        ci = new CellIdentityTdscdma(MCC_STR, null, LAC, CID, CPID);

        assertEquals(MCC_STR, ci.getMccString());
        assertNull(ci.getMncString());

        ci = new CellIdentityTdscdma(null, MNC_STR, LAC, CID, CPID);

        assertEquals(MNC_STR, ci.getMncString());
        assertNull(ci.getMccString());

        ci = new CellIdentityTdscdma("", "", LAC, CID, CPID);

        assertNull(ci.getMccString());
        assertNull(ci.getMncString());
    }

    @SmallTest
    public void testParcel() {
        CellIdentityTdscdma ci = new CellIdentityTdscdma(MCC_STR, MNC_STR, LAC, CID, CPID);

        Parcel p = Parcel.obtain();
        ci.writeToParcel(p, 0);
        p.setDataPosition(0);

        CellIdentityTdscdma newCi = CellIdentityTdscdma.CREATOR.createFromParcel(p);
        assertEquals(ci, newCi);
    }

    @SmallTest
    public void testParcelWithUnknowMccMnc() {
        CellIdentityTdscdma ci =
                new CellIdentityTdscdma(null, null, LAC, CID, CPID, ALPHA_LONG, ALPHA_SHORT);

        Parcel p = Parcel.obtain();
        p.writeInt(CellIdentity.TYPE_TDSCDMA);
        p.writeString(String.valueOf(Integer.MAX_VALUE));
        p.writeString(String.valueOf(Integer.MAX_VALUE));
        p.writeString(ALPHA_LONG);
        p.writeString(ALPHA_SHORT);
        p.writeInt(LAC);
        p.writeInt(CID);
        p.writeInt(CPID);
        p.setDataPosition(0);

        CellIdentityTdscdma newCi = CellIdentityTdscdma.CREATOR.createFromParcel(p);
        assertEquals(ci, newCi);
    }

    @SmallTest
    public void testParcelWithInvalidMccMnc() {
        final String invalidMcc = "randomStuff";
        final String invalidMnc = "randomStuff";
        CellIdentityTdscdma ci =
                new CellIdentityTdscdma(null, null, LAC, CID, CPID, ALPHA_LONG, ALPHA_SHORT);

        Parcel p = Parcel.obtain();
        p.writeInt(CellIdentity.TYPE_TDSCDMA);
        p.writeString(invalidMcc);
        p.writeString(invalidMnc);
        p.writeString(ALPHA_LONG);
        p.writeString(ALPHA_SHORT);
        p.writeInt(LAC);
        p.writeInt(CID);
        p.writeInt(CPID);
        p.setDataPosition(0);

        CellIdentityTdscdma newCi = CellIdentityTdscdma.CREATOR.createFromParcel(p);
        assertEquals(ci, newCi);
    }
}
