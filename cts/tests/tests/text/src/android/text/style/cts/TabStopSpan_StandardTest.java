/*
 * Copyright (C) 2008 The Android Open Source Project
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

package android.text.style.cts;

import static org.junit.Assert.assertEquals;

import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.style.TabStopSpan;
import android.text.style.TabStopSpan.Standard;

import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class TabStopSpan_StandardTest {
    @Test
    public void testConstructor() {
        new TabStopSpan.Standard(3);

        new TabStopSpan.Standard(-3);
    }

    @Test
    public void testGetTabStop() {
        Standard standard = new Standard(3);
        assertEquals(3, standard.getTabStop());

        standard = new Standard(-4);
        assertEquals(-4, standard.getTabStop());
    }
}
