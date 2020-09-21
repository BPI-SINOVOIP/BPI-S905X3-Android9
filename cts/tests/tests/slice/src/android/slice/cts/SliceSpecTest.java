/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package android.slice.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.app.slice.SliceSpec;
import android.os.Parcel;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class SliceSpecTest {

    @Test
    public void testDifferentTypes() {
        SliceSpec first = new SliceSpec("first", 1);
        SliceSpec second = new SliceSpec("second", 1);

        assertFalse(first.canRender(second));
        assertFalse(second.canRender(first));

        verify(first);
        verify(second);
    }

    @Test
    public void testRenderDifference() {
        SliceSpec first = new SliceSpec("namespace", 2);
        SliceSpec second = new SliceSpec("namespace", 1);

        assertTrue(first.canRender(second));
        assertFalse(second.canRender(first));
    }

    private void verify(SliceSpec s) {
        Parcel p = Parcel.obtain();
        s.writeToParcel(p, 0);

        p.setDataPosition(0);
        SliceSpec s2 = SliceSpec.CREATOR.createFromParcel(p);

        assertEquals(s.getType(), s2.getType());
        assertEquals(s.getRevision(), s2.getRevision());
        assertEquals(s, s2);

        p.recycle();
    }
}
