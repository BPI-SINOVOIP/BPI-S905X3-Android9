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
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.test.runner.AndroidJUnit4;
import android.telephony.euicc.DownloadableSubscription;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class DownloadableSubscriptionTest {

    private static final String ACTIVATION_CODE =
            "1$SMDP.GSMA.COM$04386-AGYFT-A74Y8-3F815$1.3.6.1.4.1.31746";

    private DownloadableSubscription mDownloadableSubscription;

    @Before
    public void setUp() throws Exception {
        mDownloadableSubscription = DownloadableSubscription.forActivationCode(ACTIVATION_CODE);
    }

    @Test
    public void testGetEncodedActivationCode() {
        assertNotNull(mDownloadableSubscription);
        assertEquals(ACTIVATION_CODE, mDownloadableSubscription.getEncodedActivationCode());
    }

    @Test
    public void testDescribeContents() {
        assertNotNull(mDownloadableSubscription);
        int bitmask = mDownloadableSubscription.describeContents();
        assertTrue(bitmask == 0 || bitmask == Parcelable.CONTENTS_FILE_DESCRIPTOR);
    }

    @Test
    public void testGetConfirmationCode() throws NoSuchFieldException, IllegalAccessException {
        // There is no way to set DownloadableSubscription#confirmationCode from here because
        // Android P doesn't allow accessing a platform class's @hide methods or private fields
        // through reflection.. so let's just verify confirmationCode is null.
        assertNotNull(mDownloadableSubscription);
        assertNull(mDownloadableSubscription.getConfirmationCode());
    }

    @Test
    public void testWriteToParcel() {
        assertNotNull(mDownloadableSubscription);

        // write object to parcel
        Parcel parcel = Parcel.obtain();
        mDownloadableSubscription.writeToParcel(parcel,
                mDownloadableSubscription.describeContents());

        // extract object from parcel
        parcel.setDataPosition(0 /* pos */);
        DownloadableSubscription downloadableSubscriptionFromParcel =
                DownloadableSubscription.CREATOR.createFromParcel(parcel);
        assertEquals(ACTIVATION_CODE,
                downloadableSubscriptionFromParcel.getEncodedActivationCode());

        // There is no way to set DownloadableSubscription#confirmationCode from here because
        // Android P doesn't allow accessing a platform class's @hide methods or private fields
        // through reflection.. so let's just verify confirmationCode is null.
        assertNull(downloadableSubscriptionFromParcel.getConfirmationCode());

        // Similarly there is no way to get DownloadableSubscription#carrierName or
        // DownloadableSubscription#accessRules on Android P so we can't verify them here.
    }
}
