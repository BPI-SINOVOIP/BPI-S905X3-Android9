/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.compatibility.common.util;

import android.webkit.ValueCallback;

import junit.framework.Assert;

public class EvaluateJsResultPollingCheck extends PollingCheck
        implements ValueCallback<String> {
    private String mActualResult;
    private String mExpectedResult;
    private boolean mGotResult;

    public EvaluateJsResultPollingCheck(String expected) {
        mExpectedResult = expected;
    }

    @Override
    public synchronized boolean check() {
        return mGotResult;
    }

    @Override
    public void run() {
        super.run();
        synchronized (this) {
            Assert.assertEquals(mExpectedResult, mActualResult);
        }
    }

    @Override
    public synchronized void onReceiveValue(String result) {
        mGotResult = true;
        mActualResult = result;
    }
}
