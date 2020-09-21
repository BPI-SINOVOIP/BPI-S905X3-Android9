/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.internal.telephony.dataconnection;

import android.telephony.ServiceState;
import android.telephony.data.DataProfile;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.internal.telephony.RILConstants;

import junit.framework.TestCase;

public class DataProfileTest extends TestCase {

    private ApnSetting mApn1 = new ApnSetting(
            2163,                   // id
            "44010",                // numeric
            "sp-mode",              // name
            "fake_apn",             // apn
            "",                     // proxy
            "",                     // port
            "",                     // mmsc
            "",                     // mmsproxy
            "",                     // mmsport
            "user",                 // user
            "passwd",               // password
            -1,                     // authtype
            new String[]{"default", "supl"},     // types
            "IPV6",                 // protocol
            "IP",                   // roaming_protocol
            true,                   // carrier_enabled
            0,                      // bearer
            0,                      // bearer_bitmask
            1234,                   // profile_id
            false,                  // modem_cognitive
            321,                    // max_conns
            456,                    // wait_time
            789,                    // max_conns_time
            0,                      // mtu
            "",                     // mvno_type
            "");                    // mnvo_match_data

    private ApnSetting mApn2 = new ApnSetting(
            2163,                   // id
            "44010",                // numeric
            "sp-mode",              // name
            "fake_apn",             // apn
            "",                     // proxy
            "",                     // port
            "",                     // mmsc
            "",                     // mmsproxy
            "",                     // mmsport
            "user",                 // user
            "passwd",               // password
            -1,                     // authtype
            new String[]{"default", "supl"},     // types
            "IP",                   // protocol
            "IP",                   // roaming_protocol
            true,                   // carrier_enabled
            0,                      // bearer
            0,                      // bearer_bitmask
            1234,                   // profile_id
            false,                  // modem_cognitive
            111,                    // max_conns
            456,                    // wait_time
            789,                    // max_conns_time
            0,                      // mtu
            "",                     // mvno_type
            "");                    // mnvo_match_data

    private ApnSetting mApn3 = new ApnSetting(
            2163,                   // id
            "44010",                // numeric
            "sp-mode",              // name
            "fake_apn",             // apn
            "",                     // proxy
            "",                     // port
            "",                     // mmsc
            "",                     // mmsproxy
            "",                     // mmsport
            "user",                 // user
            "passwd",               // password
            -1,                     // authtype
            new String[]{"default", "supl"},     // types
            "IP",                   // protocol
            "IP",                   // roaming_protocol
            true,                   // carrier_enabled
            276600,                    // network_type_bitmask
            1234,                   // profile_id
            false,                  // modem_cognitive
            111,                    // max_conns
            456,                    // wait_time
            789,                    // max_conns_time
            0,                      // mtu
            "",                     // mvno_type
            ""                   // mnvo_match_data
            );

    @SmallTest
    public void testCreateFromApnSetting() throws Exception {
        DataProfile dp = DcTracker.createDataProfile(mApn1, mApn1.profileId);
        assertEquals(mApn1.profileId, dp.getProfileId());
        assertEquals(mApn1.apn, dp.getApn());
        assertEquals(mApn1.protocol, dp.getProtocol());
        assertEquals(RILConstants.SETUP_DATA_AUTH_PAP_CHAP, dp.getAuthType());
        assertEquals(mApn1.user, dp.getUserName());
        assertEquals(mApn1.password, dp.getPassword());
        assertEquals(0, dp.getType());  // TYPE_COMMON
        assertEquals(mApn1.maxConnsTime, dp.getMaxConnsTime());
        assertEquals(mApn1.maxConns, dp.getMaxConns());
        assertEquals(mApn1.waitTime, dp.getWaitTime());
        assertEquals(mApn1.carrierEnabled, dp.isEnabled());
    }

    @SmallTest
    public void testCreateFromApnSettingWithNetworkTypeBitmask() throws Exception {
        DataProfile dp = DcTracker.createDataProfile(mApn3, mApn3.profileId);
        assertEquals(mApn3.profileId, dp.getProfileId());
        assertEquals(mApn3.apn, dp.getApn());
        assertEquals(mApn3.protocol, dp.getProtocol());
        assertEquals(RILConstants.SETUP_DATA_AUTH_PAP_CHAP, dp.getAuthType());
        assertEquals(mApn3.user, dp.getUserName());
        assertEquals(mApn3.password, dp.getPassword());
        assertEquals(2, dp.getType());  // TYPE_3GPP2
        assertEquals(mApn3.maxConnsTime, dp.getMaxConnsTime());
        assertEquals(mApn3.maxConns, dp.getMaxConns());
        assertEquals(mApn3.waitTime, dp.getWaitTime());
        assertEquals(mApn3.carrierEnabled, dp.isEnabled());
        int expectedBearerBitmap =
                ServiceState.convertNetworkTypeBitmaskToBearerBitmask(mApn3.networkTypeBitmask);
        assertEquals(expectedBearerBitmap, dp.getBearerBitmap());
    }

    @SmallTest
    public void testEquals() throws Exception {
        DataProfile dp1 = DcTracker.createDataProfile(mApn1, mApn1.profileId);
        DataProfile dp2 = DcTracker.createDataProfile(mApn1, mApn1.profileId);
        assertEquals(dp1, dp2);

        dp2 = DcTracker.createDataProfile(mApn2, mApn2.profileId);
        assertFalse(dp1.equals(dp2));
    }
}
