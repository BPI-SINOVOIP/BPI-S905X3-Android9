/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.framework.tests;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.RunUtil;

public class DataIdleTestHelper {
    private final static String[] PING_SERVER_LIST = {"www.google.com", "www.facebook.com",
        "www.bing.com", "www.ask.com", "www.yahoo.com"};
    private final static String PING_FAIL_STRING = "ping: unknown host";
    private ITestDevice mDevice;
    private final static int RETRY_ATTEMPTS = 5;
    private final static int PING_WAITING_TIME = 30 * 10000;  // 30 seconds

    DataIdleTestHelper(ITestDevice device) {
        mDevice = device;
    }

    /**
     * Ping a series of popular servers to check for Internet connection.
     *
     * @return true if the device is able to ping the test servers.
     * @throws DeviceNotAvailableException
     */
    public boolean pingTest() throws DeviceNotAvailableException {
        for (int i = 0; i < RETRY_ATTEMPTS; ++i) {
            for (int j = 0; j < PING_SERVER_LIST.length; ++j) {
                String host = PING_SERVER_LIST[j];
                CLog.d("Start ping test, ping %s", host);
                String res = mDevice.executeShellCommand("ping -c 10 -w 100 " + host);
                CLog.d("res: %s", res);
                if (!res.contains(PING_FAIL_STRING)) {
                    return true;
                }
            }
            RunUtil.getDefault().sleep(PING_WAITING_TIME);
        }
        return false;
    }


}
