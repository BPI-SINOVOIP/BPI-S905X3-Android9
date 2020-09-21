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

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.IEmail;
import com.android.tradefed.util.IEmail.Message;

import junit.framework.TestCase;

import org.easymock.Capture;
import org.easymock.EasyMock;

import java.util.HashMap;

/** Unit tests for {@link SmokeTestFailureReporter} */
public class SmokeTestFailureReporterTest extends TestCase {
    private SmokeTestFailureReporter mReporter = null;
    private IEmail mMailer = null;

    private static final String TAG = "DeviceSmokeTests";
    private static final String BID = "123456";
    private static final String TARGET = "target?";
    private static final String FLAVOR = "generic-userdebug";
    private static final String BRANCH = "git_master";

    @Override
    public void setUp() {
        mMailer = EasyMock.createMock(IEmail.class);
        mReporter = new SmokeTestFailureReporter(mMailer);
    }

    public void testSingleFail() throws Exception {
        final String expSubject =
                "DeviceSmokeTests SmokeFAST failed on: BuildInfo{bid=123456, "
                        + "target=target?, build_flavor=generic-userdebug, branch=git_master}";
        final String expBodyStart = "FooTest#testFoo failed\nStack trace:\nthis is a trace\n";

        final HashMap<String, Metric> emptyMap = new HashMap<>();
        final TestDescription testId = new TestDescription("FooTest", "testFoo");
        final String trace = "this is a trace";

        final Capture<Message> msgCapture = new Capture<Message>();
        mMailer.send(EasyMock.capture(msgCapture));
        EasyMock.replay(mMailer);

        final IBuildInfo build = new BuildInfo(BID, TARGET);
        build.setBuildFlavor(FLAVOR);
        build.setBuildBranch(BRANCH);
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("serial", build);
        context.setTestTag(TAG);

        mReporter.addDestination("dest.ination@email.com");
        mReporter.invocationStarted(context);
        mReporter.testRunStarted("testrun", 1);
        mReporter.testStarted(testId);
        mReporter.testFailed(testId, trace);
        mReporter.testEnded(testId, emptyMap);
        mReporter.testRunEnded(2, emptyMap);
        mReporter.invocationEnded(1);

        EasyMock.verify(mMailer);

        assertTrue(msgCapture.hasCaptured());
        final Message msg = msgCapture.getValue();
        final String subj = msg.getSubject();
        final String body = msg.getBody();
        CLog.i("subject: %s", subj);
        CLog.i("body:\n%s", body);
        assertEquals(expSubject, subj);
        assertTrue(String.format(
                "Expected body to start with \"\"\"%s\"\"\".  Body was actually: %s\n",
                expBodyStart, body), body.startsWith(expBodyStart));

    }

    public void testTwoPassOneFail() throws Exception {
        final String expSubject =
                "DeviceSmokeTests SmokeFAST failed on: BuildInfo{bid=123456, "
                        + "target=target?, build_flavor=generic-userdebug, branch=git_master}";
        final String expBodyStart = "FooTest#testFail failed\nStack trace:\nthis is a trace\n";

        final HashMap<String, Metric> emptyMap = new HashMap<>();
        final String trace = "this is a trace";
        final TestDescription testFail = new TestDescription("FooTest", "testFail");
        final TestDescription testPass1 = new TestDescription("FooTest", "testPass1");
        final TestDescription testPass2 = new TestDescription("FooTest", "testPass2");

        final Capture<Message> msgCapture = new Capture<Message>();
        mMailer.send(EasyMock.capture(msgCapture));
        EasyMock.replay(mMailer);

        IBuildInfo build = new BuildInfo(BID, TARGET);
        build.setBuildFlavor(FLAVOR);
        build.setBuildBranch(BRANCH);
        IInvocationContext context = new InvocationContext();
        context.addDeviceBuildInfo("serial", build);
        context.setTestTag(TAG);

        mReporter.addDestination("dest.ination@email.com");
        mReporter.invocationStarted(context);
        mReporter.testRunStarted("testrun", 1);
        mReporter.testStarted(testPass1);
        mReporter.testEnded(testPass1, emptyMap);

        mReporter.testStarted(testFail);
        mReporter.testFailed(testFail, trace);
        mReporter.testEnded(testFail, emptyMap);

        mReporter.testStarted(testPass2);
        mReporter.testEnded(testPass2, emptyMap);
        mReporter.testRunEnded(2, emptyMap);
        mReporter.invocationEnded(1);

        EasyMock.verify(mMailer);

        assertTrue(msgCapture.hasCaptured());
        final Message msg = msgCapture.getValue();
        final String subj = msg.getSubject();
        final String body = msg.getBody();
        CLog.i("subject: %s", subj);
        CLog.i("body:\n%s", body);
        assertEquals(expSubject, subj);
        assertTrue(String.format(
                "Expected body to start with \"\"\"%s\"\"\".  Body was actually: %s\n",
                expBodyStart, body), body.startsWith(expBodyStart));
    }
}
