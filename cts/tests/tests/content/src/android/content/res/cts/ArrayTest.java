/*
 * Copyright (C) 2009 The Android Open Source Project
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

package android.content.res.cts;

import android.content.res.Resources;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import android.content.cts.R;

public class ArrayTest extends AndroidTestCase {
    private Resources mResources;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mResources = mContext.getResources();
    }

    private void checkEntry(final int resid, final int index, final Object res,
            final Object expected) {
        assertEquals("in resource 0x" + Integer.toHexString(resid)
                + " at index " + index, expected, res);
    }

    private void checkStringArray(final int resid, final String[] expected) {
        final String[] res = mResources.getStringArray(resid);
        assertEquals(res.length, expected.length);
        for (int i = 0; i < expected.length; i++) {
            checkEntry(resid, i, res[i], expected[i]);
        }
    }

    private void checkTextArray(final int resid, final String[] expected) {
        final CharSequence[] res = mResources.getTextArray(resid);
        assertEquals(res.length, expected.length);
        for (int i = 0; i < expected.length; i++) {
            checkEntry(resid, i, res[i], expected[i]);
        }
    }

    private void checkIntArray(final int resid, final int[] expected) {
        final int[] res = mResources.getIntArray(resid);
        assertEquals(res.length, expected.length);
        for (int i = 0; i < expected.length; i++) {
            assertEquals("in resource 0x" + Integer.toHexString(resid)
                    + " at index " + i, expected[i], res[i]);
        }
    }

    @SmallTest
    public void testStrings() throws Exception {
        checkStringArray(R.array.strings, new String[] {"zero", "1", "here"});
        checkTextArray(R.array.strings, new String[] {"zero", "1", "here"});
        checkStringArray(R.array.integers, new String[] {null, null, null});
        checkTextArray(R.array.integers, new String[] {null, null, null});
    }

    @SmallTest
    public void testIntegers() throws Exception {
        checkIntArray(R.array.strings, new int[] {0, 0, 0});
        checkIntArray(R.array.integers, new int[] {0, 1, 101});
    }
}
