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
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Date;
import java.util.concurrent.TimeUnit;

/** Unit Tests for {@link TimeSetterTargetPreparer}. */
@RunWith(JUnit4.class)
public class TimeSetterTargetPreparerTest {
    private TimeSetterTargetPreparer mTimeSetterTargetPreparer;
    private ITestDevice mMockDevice;
    private IBuildInfo mMockBuildInfo;
    private long mMockNanoTime;

    @Before
    public void setUp() {
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mMockBuildInfo = EasyMock.createMock(IBuildInfo.class);
        mTimeSetterTargetPreparer =
                new TimeSetterTargetPreparer() {
                    @Override
                    long getNanoTime() {
                        return mMockNanoTime;
                    }
                };
    }

    @Test
    public void testSaveTime() throws Exception {
        OptionSetter optionSetter = new OptionSetter(mTimeSetterTargetPreparer);
        optionSetter.setOptionValue("time", "555");

        EasyMock.expect(mMockDevice.getDeviceDate()).andReturn(123L).once();
        mMockDevice.setDate(new Date(555L));
        EasyMock.expectLastCall().once();
        mMockDevice.setDate(new Date(128L));
        EasyMock.expectLastCall().once();

        EasyMock.replay(mMockDevice, mMockBuildInfo);

        mMockNanoTime = TimeUnit.MILLISECONDS.toNanos(2);
        mTimeSetterTargetPreparer.setUp(mMockDevice, mMockBuildInfo);
        mMockNanoTime = TimeUnit.MILLISECONDS.toNanos(7);
        mTimeSetterTargetPreparer.tearDown(mMockDevice, mMockBuildInfo, null);
        EasyMock.verify(mMockBuildInfo, mMockDevice);
    }
}
