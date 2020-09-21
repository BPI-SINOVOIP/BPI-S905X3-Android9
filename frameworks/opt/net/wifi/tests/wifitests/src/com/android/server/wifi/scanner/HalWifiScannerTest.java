/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.server.wifi.scanner;

import static com.android.server.wifi.ScanTestUtil.setupMockChannels;

import android.support.test.filters.SmallTest;

import org.junit.Before;

/**
 * Unit tests for {@link com.android.server.wifi.scanner.HalWifiScannerImpl}.
 */
@SmallTest
public class HalWifiScannerTest extends BaseWifiScannerImplTest {

    @Before
    public void setUp() throws Exception {
        setupMockChannels(mWifiNative,
                new int[]{2400, 2450},
                new int[]{5150, 5175},
                new int[]{5600, 5650});
        mScanner = new HalWifiScannerImpl(mContext, BaseWifiScannerImplTest.IFACE_NAME,
                mWifiNative, mWifiMonitor, mLooper.getLooper(), mClock);
    }

    // Subtle: tests are inherited from base class.
}
