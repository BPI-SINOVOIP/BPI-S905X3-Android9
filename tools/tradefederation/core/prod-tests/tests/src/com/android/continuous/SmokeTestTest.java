/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.continuous;

import com.android.continuous.SmokeTest.TrimListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;

import junit.framework.TestCase;

import org.easymock.EasyMock;

/**
 * Unit tests for {@link SmokeTest}.
 */
public class SmokeTestTest extends TestCase {
    private ITestInvocationListener mListener = null;

    @Override
    public void setUp() {
        mListener = EasyMock.createMock(ITestInvocationListener.class);
    }

    public void testRewrite() {
        TrimListener trim = new TrimListener(mListener);
        TestDescription in =
                new TestDescription(
                        "com.android.smoketest.SmokeTestRunner$3",
                        "com.android.voicedialer.VoiceDialerActivity");
        TestDescription out =
                new TestDescription("SmokeFAST", "com.android.voicedialer.VoiceDialerActivity");
        mListener.testStarted(EasyMock.eq(out));

        EasyMock.replay(mListener);
        trim.testStarted(in);
        EasyMock.verify(mListener);
    }
}

