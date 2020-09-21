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

package com.android.internal.telephony;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.os.Parcel;
import android.support.test.filters.SmallTest;
import android.telephony.SignalStrength;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link IpSecConfig}. */
@SmallTest
@RunWith(JUnit4.class)
public class SignalStrengthTest {

    @Test
    public void testDefaults() throws Exception {
        SignalStrength s = new SignalStrength();
        assertEquals(-1, s.getCdmaDbm());
        assertEquals(-1, s.getCdmaEcio());
        assertEquals(-1, s.getEvdoDbm());
        assertEquals(-1, s.getEvdoEcio());
        assertEquals(-1, s.getEvdoSnr());
        assertEquals(-1, s.getGsmBitErrorRate());
        assertEquals(99, s.getGsmSignalStrength());
        assertEquals(true, s.isGsm());
    }

    @Test
    public void testParcelUnparcel() throws Exception {
        assertParcelingIsLossless(new SignalStrength());

        SignalStrength s = new SignalStrength(
                20,      // gsmSignalStrength
                5,       // gsmBitErrorRate
                -95,     // cdmaDbm
                10,      // cdmaEcio
                -98,     // evdoDbm
                -5,      // evdoEcio
                -2,      // evdoSnr
                45,      // lteSignalStrength
                -105,    // lteRsrp
                -110,    // lteRsrq
                -115,    // lteRssnr
                13,      // lteCqi
                -90,     // tdscdmaRscp
                45,      // wcdmaSignalStrength
                20,      // wcdmaRscpAsu
                2,       // lteRsrpBoost
                false,   // gsmFlag
                true,    // lteLevelBaseOnRsrp
                "rscp"); // wcdmaDefaultMeasurement
        assertParcelingIsLossless(s);
    }

    private void assertParcelingIsLossless(SignalStrength ssi) throws Exception {
        Parcel p = Parcel.obtain();
        ssi.writeToParcel(p, 0);
        p.setDataPosition(0);
        SignalStrength sso = SignalStrength.CREATOR.createFromParcel(p);
        assertTrue(sso.equals(ssi));
    }
}

