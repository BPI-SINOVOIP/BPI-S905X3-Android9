/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.graphics.tests;

import com.android.tradefed.log.LogUtil.CLog;

import junit.framework.TestCase;

import org.junit.Assert;

import java.util.Map;

public class FlatlandTestFuncTest extends TestCase {

    private final String output = " cmdline: /data/local/tmp/flatland\n" +
            "               Scenario               | Resolution  | Time (ms)\n" +
            " 16:10 Single Static Window           | 1280 x  800 |   fast\n" +
            " 16:10 Single Static Window           | 1920 x 1200 |  3.136\n" +
            " 16:10 Single Static Window           | 2560 x 1600 |  5.524\n" +
            " 16:10 Single Static Window           | 3840 x 2400 | 11.841\n" +
            " 4:3 Single Static Window             | 2048 x 1536 |  4.292\n" +
            " 16:10 App -> Home Transition         | 1280 x  800 |  varies\n" +
            " 16:10 App -> Home Transition         | 1920 x 1200 |  5.724\n" +
            " 16:10 App -> Home Transition         | 2560 x 1600 | 10.033\n" +
            " 16:10 App -> Home Transition         | 3840 x 2400 | 22.034\n" +
            " 4:3 App -> Home Transition           | 2048 x 1536 |  8.003\n" +
            " 16:10 SurfaceView -> Home Transition | 1280 x  800 |   slow\n" +
            " 16:10 SurfaceView -> Home Transition | 1920 x 1200 |  7.023\n" +
            " 16:10 SurfaceView -> Home Transition | 2560 x 1600 | 12.337\n" +
            " 16:10 SurfaceView -> Home Transition | 3840 x 2400 | 27.283\n" +
            " 4:3 SurfaceView -> Home Transition   | 2048 x 1536 |  9.918\n";

    public void testPraseResults() throws Exception {
        FlatlandTest ft = new FlatlandTest();
        ft.parseResult(output);
        // "0" represents a "fast" result
        String t = ft.mResultMap.get("16:10 Single Static Window 1280 x  800");
        Assert.assertTrue(t.equals("0"));

        t = ft.mResultMap.get("16:10 Single Static Window 1920 x 1200");
        Assert.assertTrue(t.equals("3.136"));

        t = ft.mResultMap.get("16:10 App -> Home Transition 1280 x  800");
        Assert.assertTrue(t.equals("-1"));

        t = ft.mResultMap.get("16:10 SurfaceView -> Home Transition 1280 x  800");
        Assert.assertTrue(t.equals("1000"));

        for(Map.Entry<String, String> entry: ft.mResultMap.entrySet()) {
            CLog.e("key:%s, value:%s", entry.getKey(), entry.getValue());
        }
    }
}
