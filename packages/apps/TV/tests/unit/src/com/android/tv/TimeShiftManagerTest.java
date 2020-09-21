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

import static com.android.tv.TimeShiftManager.TIME_SHIFT_ACTION_ID_FAST_FORWARD;
import static com.android.tv.TimeShiftManager.TIME_SHIFT_ACTION_ID_JUMP_TO_NEXT;
import static com.android.tv.TimeShiftManager.TIME_SHIFT_ACTION_ID_JUMP_TO_PREVIOUS;
import static com.android.tv.TimeShiftManager.TIME_SHIFT_ACTION_ID_PAUSE;
import static com.android.tv.TimeShiftManager.TIME_SHIFT_ACTION_ID_PLAY;
import static com.android.tv.TimeShiftManager.TIME_SHIFT_ACTION_ID_REWIND;
import static com.google.common.truth.Truth.assertWithMessage;

import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import com.android.tv.testing.activities.BaseMainActivityTestCase;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class TimeShiftManagerTest extends BaseMainActivityTestCase {
    private TimeShiftManager mTimeShiftManager;

    @Override
    @Before
    public void setUp() {
        super.setUp();
        mTimeShiftManager = mActivity.getTimeShiftManager();
    }

    @Test
    public void testDisableActions() {
        enableAllActions(true);
        assertActionState(true, true, true, true, true, true);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_PLAY, false);
        assertActionState(false, true, true, true, true, true);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_PAUSE, false);
        assertActionState(false, false, true, true, true, true);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_REWIND, false);
        assertActionState(false, false, false, true, true, true);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_FAST_FORWARD, false);
        assertActionState(false, false, false, false, true, true);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_JUMP_TO_PREVIOUS, false);
        assertActionState(false, false, false, false, false, true);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_JUMP_TO_NEXT, false);
        assertActionState(false, false, false, false, false, false);
    }

    @Test
    public void testEnableActions() {
        enableAllActions(false);
        assertActionState(false, false, false, false, false, false);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_PLAY, true);
        assertActionState(true, false, false, false, false, false);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_PAUSE, true);
        assertActionState(true, true, false, false, false, false);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_REWIND, true);
        assertActionState(true, true, true, false, false, false);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_FAST_FORWARD, true);
        assertActionState(true, true, true, true, false, false);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_JUMP_TO_PREVIOUS, true);
        assertActionState(true, true, true, true, true, false);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_JUMP_TO_NEXT, true);
        assertActionState(true, true, true, true, true, true);
    }

    private void enableAllActions(boolean enabled) {
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_PLAY, enabled);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_PAUSE, enabled);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_REWIND, enabled);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_FAST_FORWARD, enabled);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_JUMP_TO_PREVIOUS, enabled);
        mTimeShiftManager.enableAction(TIME_SHIFT_ACTION_ID_JUMP_TO_NEXT, enabled);
    }

    private void assertActionState(
            boolean playEnabled,
            boolean pauseEnabled,
            boolean rewindEnabled,
            boolean fastForwardEnabled,
            boolean jumpToPreviousEnabled,
            boolean jumpToNextEnabled) {
        assertWithMessage("Play Action")
                .that(mTimeShiftManager.isActionEnabled(TIME_SHIFT_ACTION_ID_PLAY))
                .isEqualTo(playEnabled);
        assertWithMessage("Pause Action")
                .that(mTimeShiftManager.isActionEnabled(TIME_SHIFT_ACTION_ID_PAUSE))
                .isEqualTo(pauseEnabled);
        assertWithMessage("Rewind Action")
                .that(mTimeShiftManager.isActionEnabled(TIME_SHIFT_ACTION_ID_REWIND))
                .isEqualTo(rewindEnabled);
        assertWithMessage("Fast Forward Action")
                .that(mTimeShiftManager.isActionEnabled(TIME_SHIFT_ACTION_ID_FAST_FORWARD))
                .isEqualTo(fastForwardEnabled);
        assertWithMessage("Jump To Previous Action")
                .that(mTimeShiftManager.isActionEnabled(TIME_SHIFT_ACTION_ID_JUMP_TO_PREVIOUS))
                .isEqualTo(jumpToPreviousEnabled);
        assertWithMessage("Jump To Next Action")
                .that(mTimeShiftManager.isActionEnabled(TIME_SHIFT_ACTION_ID_JUMP_TO_NEXT))
                .isEqualTo(jumpToNextEnabled);
    }
}
