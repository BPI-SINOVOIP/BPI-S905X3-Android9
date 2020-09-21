/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.tradefed.targetprep.multi;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.fail;

import com.android.tradefed.build.DeviceBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.targetprep.TargetSetupError;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;

/** Unit tests for {@link MergeMultiBuildTargetPreparer}. */
@RunWith(JUnit4.class)
public class MergeMultiBuildTargetPreparerTest {

    private static final String EXAMPLE_KEY = "testsdir";
    private MergeMultiBuildTargetPreparer mPreparer;
    private IInvocationContext mContext;
    private IDeviceBuildInfo mMockInfo1;
    private IDeviceBuildInfo mMockInfo2;
    private ITestDevice mMockDevice1;

    @Before
    public void setUp() {
        mMockDevice1 = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockDevice1.getDeviceDescriptor()).andStubReturn(null);
        mMockInfo1 = new DeviceBuildInfo("id1", "target1");
        mMockInfo2 = new DeviceBuildInfo("id2", "target2");
        mContext = new InvocationContext();
        mContext.addDeviceBuildInfo("device1", mMockInfo1);
        mContext.addDeviceBuildInfo("device2", mMockInfo2);
        mContext.addAllocatedDevice("device1", mMockDevice1);
        mPreparer = new MergeMultiBuildTargetPreparer();
    }

    /** Test that a file was moved from one build to another if the key exists. */
    @Test
    public void testMergeFiles() throws Exception {
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("src-device", "device1");
        setter.setOptionValue("dest-device", "device2");
        setter.setOptionValue("key-to-copy", EXAMPLE_KEY);

        mMockInfo1.setFile(EXAMPLE_KEY, new File("fake"), "some version");
        assertEquals("some version", mMockInfo1.getTestsDirVersion());
        assertNotNull(mMockInfo1.getFile(EXAMPLE_KEY));
        assertNull(mMockInfo2.getFile(EXAMPLE_KEY));

        mPreparer.setUp(mContext);
        // Now mock info 2 has the file.
        assertNotNull(mMockInfo2.getFile(EXAMPLE_KEY));
        // The version of the provider build is preserved.
        assertEquals("some version", mMockInfo2.getTestsDirVersion());
    }

    /**
     * Test that in case of collision the files are not replaced. This is due to our BuildInfo
     * implementation that prevents replacing files.
     */
    @Test
    public void testMergeFiles_collision() throws Exception {
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("src-device", "device1");
        setter.setOptionValue("dest-device", "device2");
        setter.setOptionValue("key-to-copy", EXAMPLE_KEY);

        mMockInfo1.setFile(EXAMPLE_KEY, new File("/fake"), "version1");
        mMockInfo2.setFile(EXAMPLE_KEY, new File("/orig"), "version0");

        assertNotNull(mMockInfo1.getFile(EXAMPLE_KEY));
        assertNotNull(mMockInfo2.getFile(EXAMPLE_KEY));

        mPreparer.setUp(mContext);
        // Now mock info 2 still has the original file
        assertNotNull(mMockInfo2.getFile(EXAMPLE_KEY));
        assertEquals("/orig", mMockInfo2.getFile(EXAMPLE_KEY).getAbsolutePath());
    }

    /** Test that unfound keys are ignored. */
    @Test
    public void testMergeFiles_keyNotFound() throws Exception {
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("src-device", "device1");
        setter.setOptionValue("dest-device", "device2");
        setter.setOptionValue("key-to-copy", EXAMPLE_KEY);

        mPreparer.setUp(mContext);
        // receiver build info is still null because the key did not have an entry.
        assertNull(mMockInfo2.getFile(EXAMPLE_KEY));
    }

    /** Test if the receiver device name is not found in the list of device. */
    @Test
    public void testReceiverNotFound() throws Exception {
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("src-device", "device1");
        setter.setOptionValue("dest-device", "doesnotexists");
        try {
            EasyMock.replay(mMockDevice1);
            mPreparer.setUp(mContext);
            fail("Should have thrown an exception.");
        } catch (TargetSetupError expected) {
            assertEquals(
                    "Could not find a build associated with 'doesnotexists' null",
                    expected.getMessage());
        }
        EasyMock.verify(mMockDevice1);
    }

    /** Test if the provider device name is not found in the list. */
    @Test
    public void testProviderNotFound() throws Exception {
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("src-device", "doesnotexists1");
        setter.setOptionValue("dest-device", "device2");
        try {
            EasyMock.replay(mMockDevice1);
            mPreparer.setUp(mContext);
            fail("Should have thrown an exception.");
        } catch (TargetSetupError expected) {
            assertEquals(
                    "Could not find a build associated with 'doesnotexists1' null",
                    expected.getMessage());
        }
        EasyMock.verify(mMockDevice1);
    }
}
