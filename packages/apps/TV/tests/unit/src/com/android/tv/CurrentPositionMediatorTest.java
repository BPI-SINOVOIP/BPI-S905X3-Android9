/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.tv;

import static com.android.tv.TimeShiftManager.INVALID_TIME;
import static com.android.tv.TimeShiftManager.REQUEST_TIMEOUT_MS;
import static com.google.common.truth.Truth.assertWithMessage;

import android.support.test.annotation.UiThreadTest;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import com.android.tv.testing.activities.BaseMainActivityTestCase;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class CurrentPositionMediatorTest extends BaseMainActivityTestCase {
    private TimeShiftManager.CurrentPositionMediator mMediator;

    @Override
    @Before
    public void setUp() {
        super.setUp();
        mMediator = mActivity.getTimeShiftManager().mCurrentPositionMediator;
    }

    @UiThreadTest
    @Test
    public void testInitialize() {
        long currentTimeMs = System.currentTimeMillis();
        mMediator.initialize(currentTimeMs);
        assertCurrentPositionMediator(INVALID_TIME, currentTimeMs);
    }

    @UiThreadTest
    @Test
    public void testOnSeekRequested() {
        long seekToTimeMs = System.currentTimeMillis() - REQUEST_TIMEOUT_MS * 3;
        mMediator.onSeekRequested(seekToTimeMs);
        assertWithMessage("Seek request time")
                .that(mMediator.mSeekRequestTimeMs)
                .isNotSameAs(INVALID_TIME);
        assertWithMessage("Current position")
                .that(mMediator.mCurrentPositionMs)
                .isEqualTo(seekToTimeMs);
    }

    @UiThreadTest
    @Test
    public void testOnCurrentPositionChangedInvalidInput() {
        long seekToTimeMs = System.currentTimeMillis() - REQUEST_TIMEOUT_MS * 3;
        long newCurrentTimeMs = seekToTimeMs + REQUEST_TIMEOUT_MS;
        mMediator.onSeekRequested(seekToTimeMs);
        mMediator.onCurrentPositionChanged(newCurrentTimeMs);
        assertWithMessage("Seek request time")
                .that(mMediator.mSeekRequestTimeMs)
                .isNotSameAs(INVALID_TIME);
        assertWithMessage("Current position")
                .that(mMediator.mCurrentPositionMs)
                .isNotSameAs(seekToTimeMs);
        assertWithMessage("Current position")
                .that(mMediator.mCurrentPositionMs)
                .isNotSameAs(newCurrentTimeMs);
    }

    @UiThreadTest
    @Test
    public void testOnCurrentPositionChangedValidInput() {
        long seekToTimeMs = System.currentTimeMillis() - REQUEST_TIMEOUT_MS * 3;
        long newCurrentTimeMs = seekToTimeMs + REQUEST_TIMEOUT_MS - 1;
        mMediator.onSeekRequested(seekToTimeMs);
        mMediator.onCurrentPositionChanged(newCurrentTimeMs);
        assertCurrentPositionMediator(INVALID_TIME, newCurrentTimeMs);
    }

    private void assertCurrentPositionMediator(
            long expectedSeekRequestTimeMs, long expectedCurrentPositionMs) {
        assertWithMessage("Seek request time")
                .that(mMediator.mSeekRequestTimeMs)
                .isEqualTo(expectedSeekRequestTimeMs);
        assertWithMessage("Current position")
                .that(mMediator.mCurrentPositionMs)
                .isEqualTo(expectedCurrentPositionMs);
    }
}
