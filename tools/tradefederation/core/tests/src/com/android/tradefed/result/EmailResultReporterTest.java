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
package com.android.tradefed.result;

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.util.IEmail;
import com.android.tradefed.util.IEmail.Message;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.io.IOException;

/**
 * Unit tests for {@link EmailResultReporter}.
 */
public class EmailResultReporterTest extends TestCase {
    private IEmail mMockMailer;
    private EmailResultReporter mEmailReporter;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMockMailer = EasyMock.createMock(IEmail.class);
        mEmailReporter = new EmailResultReporter(mMockMailer);
    }

    /**
     * Test normal success case for {@link EmailResultReporter#invocationEnded(long)}.
     * @throws IOException
     */
    public void testInvocationEnded() throws IllegalArgumentException, IOException {
        mMockMailer.send(EasyMock.<Message>anyObject());
        EasyMock.replay(mMockMailer);
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("fakeDevice", new BuildInfo("888", "mybuild"));
        context.setTestTag("mytest");
        mEmailReporter.invocationStarted(context);
        mEmailReporter.addDestination("foo");
        mEmailReporter.invocationEnded(0);
        EasyMock.verify(mMockMailer);

        assertEquals("Tradefed result for mytest  on BuildInfo{bid=888, target=mybuild}: SUCCESS",
                mEmailReporter.generateEmailSubject());
    }

    /**
     * Test normal success case for {@link EmailResultReporter#invocationEnded(long)} with multiple
     * builds.
     */
    public void testInvocationEnded_multiBuild() throws IllegalArgumentException, IOException {
        mMockMailer.send(EasyMock.<Message>anyObject());
        EasyMock.replay(mMockMailer);
        IInvocationContext context = new InvocationContext();
        context.addAllocatedDevice("fakeDevice", EasyMock.createMock(ITestDevice.class));
        context.addDeviceBuildInfo("fakeDevice", new BuildInfo("888", "mybuild"));
        context.addAllocatedDevice("fakeDevice2", EasyMock.createMock(ITestDevice.class));
        context.addDeviceBuildInfo("fakeDevice2", new BuildInfo("999", "mybuild2"));
        context.setTestTag("mytest");
        mEmailReporter.invocationStarted(context);
        mEmailReporter.addDestination("foo");
        mEmailReporter.invocationEnded(0);
        EasyMock.verify(mMockMailer);

        assertEquals("Tradefed result for mytest  on BuildInfo{bid=888, target=mybuild}"
                + "BuildInfo{bid=999, target=mybuild2}: SUCCESS",
                mEmailReporter.generateEmailSubject());
    }

    /**
     * Make sure that we don't include the string "null" in a generated email subject
     */
    public void testNullFlavorAndBranch() throws Exception {
        mMockMailer.send(EasyMock.<Message>anyObject());
        EasyMock.replay(mMockMailer);
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("fakeDevice", new BuildInfo("888", null));
        mEmailReporter.invocationStarted(context);
        mEmailReporter.addDestination("foo");
        mEmailReporter.invocationEnded(0);

        EasyMock.verify(mMockMailer);

        assertEquals("Tradefed result for (unknown suite) on BuildInfo{bid=888}: SUCCESS",
                mEmailReporter.generateEmailSubject());
    }
}
