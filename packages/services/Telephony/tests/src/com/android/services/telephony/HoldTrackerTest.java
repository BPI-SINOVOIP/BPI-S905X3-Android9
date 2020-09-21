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

package com.android.services.telephony;

import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;

import android.content.ComponentName;
import android.support.test.runner.AndroidJUnit4;
import android.telecom.PhoneAccountHandle;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class HoldTrackerTest {

    private HoldTracker mHoldTrackerUT;
    private PhoneAccountHandle mPhoneAccountHandle1;
    private PhoneAccountHandle mPhoneAccountHandle2;

    @Before
    public void setUp() throws Exception {
        mHoldTrackerUT = new HoldTracker();
        mPhoneAccountHandle1 =
                new PhoneAccountHandle(new ComponentName("pkg1", "cls1"), "0");
        mPhoneAccountHandle2 =
                new PhoneAccountHandle(new ComponentName("pkg2", "cls2"), "1");
    }

    @Test
    public void oneTopHoldableCanBeHeld() {
        FakeHoldable topHoldable = createHoldable(false);
        mHoldTrackerUT.addHoldable(mPhoneAccountHandle1, topHoldable);

        assertTrue(topHoldable.canBeHeld());
    }

    @Test
    public void childHoldableCanNotBeHeld() {
        FakeHoldable topHoldable = createHoldable(false);
        FakeHoldable childHoldable = createHoldable(true);
        mHoldTrackerUT.addHoldable(mPhoneAccountHandle1, topHoldable);
        mHoldTrackerUT.addHoldable(mPhoneAccountHandle1, childHoldable);

        assertTrue(topHoldable.canBeHeld());
        assertFalse(childHoldable.canBeHeld());
    }

    @Test
    public void twoTopHoldableWithTheSamePhoneAccountCanNotBeHeld() {
        FakeHoldable topHoldable1 = createHoldable(false);
        FakeHoldable topHoldable2 = createHoldable(false);
        mHoldTrackerUT.addHoldable(mPhoneAccountHandle1, topHoldable1);
        mHoldTrackerUT.addHoldable(mPhoneAccountHandle1, topHoldable2);

        mHoldTrackerUT.updateHoldCapability(mPhoneAccountHandle1);
        assertFalse(topHoldable1.canBeHeld());
        assertFalse(topHoldable2.canBeHeld());
    }

    @Test
    public void holdableWithDifferentPhoneAccountDoesNotAffectEachOther() {
        FakeHoldable topHoldable1 = createHoldable(false);
        FakeHoldable topHoldable2 = createHoldable(false);
        mHoldTrackerUT.addHoldable(mPhoneAccountHandle1, topHoldable1);
        mHoldTrackerUT.addHoldable(mPhoneAccountHandle2, topHoldable2);

        // Both phones account have only one top holdable, so the holdable of each phone account can
        // be held.
        assertTrue(topHoldable1.canBeHeld());
        assertTrue(topHoldable2.canBeHeld());
    }

    @Test
    public void removeOneTopHoldableAndUpdateHoldCapabilityCorrectly() {
        FakeHoldable topHoldable1 = createHoldable(false);
        FakeHoldable topHoldable2 = createHoldable(false);
        mHoldTrackerUT.addHoldable(mPhoneAccountHandle1, topHoldable1);
        mHoldTrackerUT.addHoldable(mPhoneAccountHandle1, topHoldable2);
        assertFalse(topHoldable1.canBeHeld());
        assertFalse(topHoldable2.canBeHeld());

        mHoldTrackerUT.removeHoldable(mPhoneAccountHandle1, topHoldable1);
        assertTrue(topHoldable2.canBeHeld());
    }

    public FakeHoldable createHoldable(boolean isChildHoldable) {
        return new FakeHoldable(isChildHoldable);
    }

    private class FakeHoldable implements Holdable {
        private boolean mIsChildHoldable;
        private boolean mIsHoldable;

        FakeHoldable(boolean isChildHoldable) {
            mIsChildHoldable = isChildHoldable;
        }

        @Override
        public boolean isChildHoldable() {
            return mIsChildHoldable;
        }

        @Override
        public void setHoldable(boolean isHoldable) {
            mIsHoldable = isHoldable;
        }

        public boolean canBeHeld() {
            return mIsHoldable;
        }
    }
}
