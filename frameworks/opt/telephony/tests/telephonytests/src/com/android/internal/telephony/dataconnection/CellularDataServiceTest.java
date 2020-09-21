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


import static junit.framework.Assert.assertEquals;

import android.hardware.radio.V1_0.SetupDataCallResult;
import android.net.LinkAddress;
import android.net.NetworkUtils;
import android.telephony.data.DataCallResponse;

import com.android.internal.telephony.dataconnection.CellularDataService;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.Arrays;

public class CellularDataServiceTest extends TelephonyTest {

    private CellularDataService mCellularDataService;

    @Before
    public void setUp() throws Exception {
        super.setUp(getClass().getSimpleName());
        mCellularDataService = new CellularDataService();
    }

    @After
    public void tearDown() throws Exception {
        super.tearDown();
    }

    @Test
    public void testConvertDataCallResult() throws Exception {

        SetupDataCallResult result = new SetupDataCallResult();
        result.status = 0;
        result.suggestedRetryTime = -1;
        result.cid = 1;
        result.active = 1;
        result.type = "IP";
        result.ifname = "eth0";
        result.addresses = "10.0.2.15";
        result.dnses = "10.0.2.3";
        result.gateways = "10.0.2.15 fe80::2";
        result.pcscf = "";
        result.mtu = 1500;

        DataCallResponse response = new DataCallResponse(0, -1, 1, 1, "IP",
                "eth0",
                Arrays.asList(new LinkAddress(NetworkUtils.numericToInetAddress("10.0.2.15"), 32)),
                Arrays.asList(NetworkUtils.numericToInetAddress("10.0.2.3")),
                Arrays.asList(NetworkUtils.numericToInetAddress("10.0.2.15"),
                        NetworkUtils.numericToInetAddress("fe80::2")),
                Arrays.asList(""),
                1500);

        assertEquals(response, mCellularDataService.convertDataCallResult(result));

        result.status = 0;
        result.suggestedRetryTime = -1;
        result.cid = 0;
        result.active = 2;
        result.type = "IPV4V6";
        result.ifname = "ifname";
        result.addresses = "2607:fb90:a620:651d:eabe:f8da:c107:44be/64";
        result.dnses = "fd00:976a::9      fd00:976a::10";
        result.gateways = "fe80::4c61:1832:7b28:d36c    1.2.3.4";
        result.pcscf = "fd00:976a:c206:20::6   fd00:976a:c206:20::9    fd00:976a:c202:1d::9";
        result.mtu = 1500;

        response = new DataCallResponse(0, -1, 0, 2, "IPV4V6",
                "ifname",
                Arrays.asList(new LinkAddress("2607:fb90:a620:651d:eabe:f8da:c107:44be/64")),
                Arrays.asList(NetworkUtils.numericToInetAddress("fd00:976a::9"),
                        NetworkUtils.numericToInetAddress("fd00:976a::10")),
                Arrays.asList(NetworkUtils.numericToInetAddress("fe80::4c61:1832:7b28:d36c"),
                        NetworkUtils.numericToInetAddress("1.2.3.4")),
                Arrays.asList("fd00:976a:c206:20::6", "fd00:976a:c206:20::9",
                        "fd00:976a:c202:1d::9"),
                1500);

        assertEquals(response, mCellularDataService.convertDataCallResult(result));
    }
}
