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

package android.telephony.euicc.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import android.telephony.euicc.EuiccInfo;

@RunWith(AndroidJUnit4.class)
public class EuiccInfoTest {

    private static final String OS_VERSION = "1.23";

    private EuiccInfo mEuiccInfo;

    @Before
    public void setUp() throws Exception {
        mEuiccInfo = new EuiccInfo(OS_VERSION);
    }

    @Test
    public void testGetOsVersion() {
        assertNotNull(mEuiccInfo);
        assertEquals(OS_VERSION, mEuiccInfo.getOsVersion());
    }

    @Test
    public void testDescribeContents() {
        assertNotNull(mEuiccInfo);
        int bitmask = mEuiccInfo.describeContents();
        assertTrue(bitmask == 0 || bitmask == Parcelable.CONTENTS_FILE_DESCRIPTOR);
    }

    @Test
    public void testWriteToParcel() {
        assertNotNull(mEuiccInfo);

        // write object to parcel
        Parcel parcel = Parcel.obtain();
        mEuiccInfo.writeToParcel(parcel, mEuiccInfo.describeContents());

        // extract object from parcel
        parcel.setDataPosition(0 /* pos */);
        EuiccInfo euiccInfoFromParcel = EuiccInfo.CREATOR.createFromParcel(parcel);
        assertEquals(OS_VERSION, euiccInfoFromParcel.getOsVersion());
    }
}
