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

package com.android.tradefed.device;

import com.android.ddmlib.IShellOutputReceiver;

import org.easymock.EasyMock;
import org.easymock.IAnswer;

import java.util.concurrent.TimeUnit;

class MockTestDeviceHelper {

    /**
     * Helper method to build a response to a
     * {@link ITestDevice#executeShellCommand(String, IShellOutputReceiver)} call.
     *
     * @param mockDevice the EasyMock created {@link ITestDevice}
     * @param expectedCommand the shell command to expect or null to skip verification of command
     * @param response the response to simulate
     * @param asStub whether to set a single expectation or a stub expectation. A 'stub' expectation
     *            will return the same result for multiple calls to the method
     */
    @SuppressWarnings("unchecked")
    static void injectShellResponse(ITestDevice mockDevice, final String expectedCommand,
            final String response, boolean asStub) throws Exception {
        IAnswer<Object> shellAnswer = new IAnswer<Object>() {
            @Override
            public Object answer() throws Throwable {
                IShellOutputReceiver receiver = (IShellOutputReceiver)EasyMock
                        .getCurrentArguments()[1];
                byte[] inputData = response.getBytes();
                receiver.addOutput(inputData, 0, inputData.length);
                receiver.flush();
                return null;
            }
        };
        if (expectedCommand != null) {
            mockDevice.executeShellCommand(EasyMock.eq(expectedCommand),
                    EasyMock.<IShellOutputReceiver>anyObject());
        } else {
            mockDevice.executeShellCommand(EasyMock.<String>anyObject(),
                    EasyMock.<IShellOutputReceiver>anyObject());

        }
        if (asStub) {
            EasyMock.expectLastCall().andStubAnswer(shellAnswer);
        } else {
            EasyMock.expectLastCall().andAnswer(shellAnswer);
        }
    }

    /**
     * Helper method to build a response to a
     * {@link ITestDevice#executeShellCommand(String, IShellOutputReceiver, long, TimeUnit, int)}
     * call.
     *
     * @param mockDevice the EasyMock created {@link ITestDevice}
     * @param expectedCommand the shell command to expect or null to skip verification of command
     * @param expectedTimeout the shell timeout to expect
     * @param expectedTimeUnit the shell timeout unit to expect
     * @param expectedRetryAttempts the retry attempts to expect
     * @param response the response to simulate
     * @param asStub whether to set a single expectation or a stub expectation. A 'stub' expectation
     *            will return the same result for multiple calls to the method
     */
    @SuppressWarnings("unchecked")
    static void injectShellResponse(ITestDevice mockDevice, final String expectedCommand,
            final long expectedTimeout, final TimeUnit expectedTimeUnit,
            final int expectedRetryAttempts, final String response, boolean asStub)
                    throws Exception {
        IAnswer<Object> shellAnswer = new IAnswer<Object>() {
            @Override
            public Object answer() throws Throwable {
                IShellOutputReceiver receiver = (IShellOutputReceiver)EasyMock
                        .getCurrentArguments()[1];
                byte[] inputData = response.getBytes();
                receiver.addOutput(inputData, 0, inputData.length);
                receiver.flush();
                return null;
            }
        };
        if (expectedCommand != null) {
            mockDevice.executeShellCommand(EasyMock.eq(expectedCommand),
                    EasyMock.<IShellOutputReceiver> anyObject(), EasyMock.eq(expectedTimeout),
                    EasyMock.eq(expectedTimeUnit), EasyMock.eq(expectedRetryAttempts));
        } else {
            mockDevice.executeShellCommand(EasyMock.<String> anyObject(),
                    EasyMock.<IShellOutputReceiver> anyObject(), EasyMock.eq(expectedTimeout),
                    EasyMock.eq(expectedTimeUnit), EasyMock.eq(expectedRetryAttempts));
        }
        if (asStub) {
            EasyMock.expectLastCall().andStubAnswer(shellAnswer);
        } else {
            EasyMock.expectLastCall().andAnswer(shellAnswer);
        }
    }
}
