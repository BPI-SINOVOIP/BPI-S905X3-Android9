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
 * limitations under the License
 */

package android.telecom.cts;

import android.content.Context;
import android.telecom.TelecomManager;
import android.test.InstrumentationTestCase;
import android.text.TextUtils;

/**
 * Verifies correct operation of TelecomManager APIs when the correct permissions have not been
 * granted.
 */
public class TelecomManagerNoPermissionsTest extends InstrumentationTestCase {
    private Context mContext;
    private TelecomManager mTelecomManager;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = getInstrumentation().getContext();
        if (!TestUtils.shouldTestTelecom(mContext)) {
            return;
        }
        mTelecomManager = (TelecomManager) mContext.getSystemService(Context.TELECOM_SERVICE);
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
    }

    public void testCannotEndCall() throws Exception {
        if (!TestUtils.shouldTestTelecom(mContext)) {
            return;
        }
        try {
            mTelecomManager.endCall();
            fail("Shouldn't be able to call endCall without permission grant.");
        } catch (SecurityException se) {
        }
    }
}
