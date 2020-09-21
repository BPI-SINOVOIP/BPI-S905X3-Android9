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
package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.ITestDevice;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit Tests for {@link RootTargetPreparer}. */
@RunWith(JUnit4.class)
public class RootTargetPreparerTest {

    private RootTargetPreparer mRootTargetPreparer;
    private ITestDevice mMockDevice;
    private IBuildInfo mMockBuildInfo;

    @Before
    public void setUp() {
        mRootTargetPreparer = new RootTargetPreparer();
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mMockBuildInfo = EasyMock.createMock(IBuildInfo.class);
    }

    @Test
    public void testSetUpSuccess_rootBefore() throws Exception {
        EasyMock.expect(mMockDevice.isAdbRoot()).andReturn(true).once();
        EasyMock.replay(mMockDevice, mMockBuildInfo);

        mRootTargetPreparer.setUp(mMockDevice, mMockBuildInfo);
        mRootTargetPreparer.tearDown(mMockDevice, mMockBuildInfo, null);
        EasyMock.verify(mMockDevice, mMockBuildInfo);
    }

    @Test
    public void testSetUpSuccess_notRootBefore() throws Exception {
        EasyMock.expect(mMockDevice.isAdbRoot()).andReturn(false).once();
        EasyMock.expect(mMockDevice.enableAdbRoot()).andReturn(true).once();
        EasyMock.expect(mMockDevice.disableAdbRoot()).andReturn(true).once();
        EasyMock.replay(mMockDevice, mMockBuildInfo);

        mRootTargetPreparer.setUp(mMockDevice, mMockBuildInfo);
        mRootTargetPreparer.tearDown(mMockDevice, mMockBuildInfo, null);
        EasyMock.verify(mMockDevice, mMockBuildInfo);
    }

    @Test(expected = TargetSetupError.class)
    public void testSetUpFail() throws Exception {
        EasyMock.expect(mMockDevice.isAdbRoot()).andReturn(false).once();
        EasyMock.expect(mMockDevice.enableAdbRoot()).andReturn(false).once();
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andReturn(null).once();
        EasyMock.replay(mMockDevice, mMockBuildInfo);

        mRootTargetPreparer.setUp(mMockDevice, mMockBuildInfo);
    }
}
