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

package android.security.cts;

import org.junit.runner.RunWith;
import org.junit.Test;

import android.graphics.Bitmap;
import android.platform.test.annotations.SecurityTest;
import android.support.test.runner.AndroidJUnit4;

@SecurityTest
@RunWith(AndroidJUnit4.class)
public class BitmapTest {
    /**
     * Test Bitmap.createBitmap properly throws OOME on large inputs.
     *
     * A prior change in behavior resulted in throwing an NPE instead.
     * OOME is more appropriate.
     */
    @Test(expected=OutOfMemoryError.class)
    public void test_33846679() {
        // This size is based on the max size possible in a GIF file,
        // which might be passed to createBitmap from a Java decoder.
        Bitmap.createBitmap(65535, 65535, Bitmap.Config.ARGB_8888);
    }
}
