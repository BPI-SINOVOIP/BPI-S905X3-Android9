/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.internal.os;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;

import android.os.BatteryStats;
import android.os.Parcel;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Log;

import junit.framework.Assert;
import junit.framework.TestCase;

import com.android.internal.os.BatteryStatsImpl;

import org.mockito.Mockito;

/**
 * Provides test cases for android.os.BatteryStats.
 */
public class BatteryStatsUidTest extends TestCase {
    private static final String TAG = "BatteryStatsTimeBaseTest";

    static class TestBsi extends BatteryStatsImpl {
        TestBsi(MockClocks clocks) {
            super(clocks);
        }
    }

    /**
     * Test the observers and the setRunning call.
     */
    @SmallTest
    public void testParceling() throws Exception  {
    }
}

