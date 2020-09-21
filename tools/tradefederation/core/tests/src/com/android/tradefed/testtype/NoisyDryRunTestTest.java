/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.tradefed.testtype;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.util.FileUtil;

import org.easymock.EasyMock;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;

/**
 * Unit test for {@link NoisyDryRunTest}.
 */
@RunWith(JUnit4.class)
public class NoisyDryRunTestTest {

    private File mFile;
    private ITestInvocationListener mMockListener;

    @Before
    public void setUp() throws Exception {
        mFile = FileUtil.createTempFile("NoisyDryRunTestTest", "tmpFile");
        mMockListener = EasyMock.createMock(ITestInvocationListener.class);
    }

    @After
    public void tearDown() {
        FileUtil.deleteFile(mFile);
    }

    @Test
    public void testRun() throws Exception {
        FileUtil.writeToFile("tf/fake\n"
                + "tf/fake", mFile);
        mMockListener.testRunStarted("com.android.tradefed.testtype.NoisyDryRunTest_parseFile", 1);
        mMockListener.testStarted(EasyMock.anyObject());
        mMockListener.testEnded(
                EasyMock.anyObject(), (HashMap<String, Metric>) EasyMock.anyObject());
        mMockListener.testRunEnded(EasyMock.eq(0l), (HashMap<String, Metric>) EasyMock.anyObject());

        mMockListener.testRunStarted("com.android.tradefed.testtype.NoisyDryRunTest_parseCommands",
                2);
        mMockListener.testStarted(EasyMock.anyObject());
        mMockListener.testEnded(
                EasyMock.anyObject(), (HashMap<String, Metric>) EasyMock.anyObject());
        mMockListener.testStarted(EasyMock.anyObject());
        mMockListener.testEnded(
                EasyMock.anyObject(), (HashMap<String, Metric>) EasyMock.anyObject());
        mMockListener.testRunEnded(EasyMock.eq(0l), (HashMap<String, Metric>) EasyMock.anyObject());
        replayMocks();

        NoisyDryRunTest noisyDryRunTest = new NoisyDryRunTest();
        OptionSetter setter = new OptionSetter(noisyDryRunTest);
        setter.setOptionValue("cmdfile", mFile.getAbsolutePath());
        noisyDryRunTest.run(mMockListener);
        verifyMocks();
    }

    /**
     * Test loading a configuration with a USE_KEYSTORE option specified. It should still load and
     * we fake the keystore to simply validate the overall structure.
     */
    @Test
    public void testRun_withKeystore() throws Exception {
        FileUtil.writeToFile("tf/fake --fail-invocation-with-cause USE_KEYSTORE@fake\n", mFile);
        mMockListener.testRunStarted("com.android.tradefed.testtype.NoisyDryRunTest_parseFile", 1);
        mMockListener.testStarted(EasyMock.anyObject());
        mMockListener.testEnded(
                EasyMock.anyObject(), (HashMap<String, Metric>) EasyMock.anyObject());
        mMockListener.testRunEnded(EasyMock.eq(0l), (HashMap<String, Metric>) EasyMock.anyObject());

        mMockListener.testRunStarted(
                "com.android.tradefed.testtype.NoisyDryRunTest_parseCommands", 1);
        mMockListener.testStarted(EasyMock.anyObject());
        mMockListener.testEnded(
                EasyMock.anyObject(), (HashMap<String, Metric>) EasyMock.anyObject());

        mMockListener.testRunEnded(EasyMock.eq(0l), (HashMap<String, Metric>) EasyMock.anyObject());
        replayMocks();

        NoisyDryRunTest noisyDryRunTest = new NoisyDryRunTest();
        OptionSetter setter = new OptionSetter(noisyDryRunTest);
        setter.setOptionValue("cmdfile", mFile.getAbsolutePath());
        noisyDryRunTest.run(mMockListener);
        verifyMocks();
    }

    @Test
    public void testRun_invalidCmdFile() throws Exception {
        FileUtil.deleteFile(mFile);
        mMockListener.testRunStarted("com.android.tradefed.testtype.NoisyDryRunTest_parseFile", 1);
        mMockListener.testStarted(EasyMock.anyObject());
        mMockListener.testEnded(
                EasyMock.anyObject(), (HashMap<String, Metric>) EasyMock.anyObject());
        mMockListener.testFailed(EasyMock.anyObject(), EasyMock.anyObject());
        mMockListener.testRunEnded(EasyMock.eq(0l), (HashMap<String, Metric>) EasyMock.anyObject());
        replayMocks();

        NoisyDryRunTest noisyDryRunTest = new NoisyDryRunTest();
        OptionSetter setter = new OptionSetter(noisyDryRunTest);
        setter.setOptionValue("cmdfile", mFile.getAbsolutePath());
        noisyDryRunTest.run(mMockListener);
        verifyMocks();
    }

    @Test
    public void testRun_invalidCmdLine() throws Exception {
        FileUtil.writeToFile("tf/fake\n"
                + "invalid --option value2", mFile);
        mMockListener.testRunStarted("com.android.tradefed.testtype.NoisyDryRunTest_parseFile", 1);
        mMockListener.testStarted(EasyMock.anyObject());
        mMockListener.testEnded(
                EasyMock.anyObject(), (HashMap<String, Metric>) EasyMock.anyObject());
        mMockListener.testRunEnded(EasyMock.eq(0l), (HashMap<String, Metric>) EasyMock.anyObject());

        mMockListener.testRunStarted("com.android.tradefed.testtype.NoisyDryRunTest_parseCommands",
                2);
        mMockListener.testStarted(EasyMock.anyObject());
        mMockListener.testEnded(
                EasyMock.anyObject(), (HashMap<String, Metric>) EasyMock.anyObject());
        mMockListener.testStarted(EasyMock.anyObject());
        mMockListener.testFailed(EasyMock.anyObject(), EasyMock.anyObject());
        mMockListener.testEnded(
                EasyMock.anyObject(), (HashMap<String, Metric>) EasyMock.anyObject());
        mMockListener.testRunEnded(EasyMock.eq(0l), (HashMap<String, Metric>) EasyMock.anyObject());
        replayMocks();

        NoisyDryRunTest noisyDryRunTest = new NoisyDryRunTest();
        OptionSetter setter = new OptionSetter(noisyDryRunTest);
        setter.setOptionValue("cmdfile", mFile.getAbsolutePath());
        noisyDryRunTest.run(mMockListener);
        verifyMocks();
    }

    @Test
    public void testCheckFileWithTimeout() throws Exception {
        NoisyDryRunTest noisyDryRunTest = new NoisyDryRunTest() {
            long mCurrentTime = 0;
            @Override
            void sleep() {
            }

            @Override
            long currentTimeMillis() {
                mCurrentTime += 5 * 1000;
                return mCurrentTime;
            }
        };
        OptionSetter setter = new OptionSetter(noisyDryRunTest);
        setter.setOptionValue("timeout", "100");
        noisyDryRunTest.checkFileWithTimeout(mFile);
    }

    @Test
    public void testCheckFileWithTimeout_missingFile() throws Exception {
        NoisyDryRunTest noisyDryRunTest = new NoisyDryRunTest() {
            long mCurrentTime = 0;

            @Override
            void sleep() {
            }

            @Override
            long currentTimeMillis() {
                mCurrentTime += 5 * 1000;
                return mCurrentTime;
            }
        };
        OptionSetter setter = new OptionSetter(noisyDryRunTest);
        setter.setOptionValue("timeout", "100");
        File missingFile = new File("missing_file");
        try {
            noisyDryRunTest.checkFileWithTimeout(missingFile);
            fail("Should have thrown IOException");
        } catch (IOException e) {
            assertEquals(missingFile.getAbsoluteFile() + " doesn't exist.", e.getMessage());
            assertTrue(true);
        }
    }

    @Test
    public void testCheckFileWithTimeout_delayFile() throws Exception {
        FileUtil.deleteFile(mFile);
        NoisyDryRunTest noisyDryRunTest = new NoisyDryRunTest() {
            long mCurrentTime = 0;

            @Override
            void sleep() {
            }

            @Override
            long currentTimeMillis() {
                mCurrentTime += 5 * 1000;
                if (mCurrentTime > 10 * 1000) {
                    try {
                        mFile.createNewFile();
                    } catch (IOException e) {
                    }
                }
                return mCurrentTime;
            }
        };
        OptionSetter setter = new OptionSetter(noisyDryRunTest);
        setter.setOptionValue("timeout", "100000");
        noisyDryRunTest.checkFileWithTimeout(mFile);
    }

    private void replayMocks() {
        EasyMock.replay(mMockListener);
    }

    private void verifyMocks() {
        EasyMock.verify(mMockListener);
    }
}
