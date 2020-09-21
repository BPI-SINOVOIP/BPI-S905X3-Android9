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

import android.graphics.Color;
import android.os.Parcel;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.TextPaint;
import android.text.style.BackgroundColorSpan;

import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class BackgroundColorSpanTest {
    @Test
    public void testConstructor() {
        BackgroundColorSpan b = new BackgroundColorSpan(Color.GREEN);

        final Parcel p = Parcel.obtain();
        try {
            b.writeToParcel(p, 0);
            p.setDataPosition(0);
            new BackgroundColorSpan(p);
        } finally {
            p.recycle();
        }
    }

    @Test
    public void testUpdateDrawState() {
        BackgroundColorSpan backgroundColorSpan = new BackgroundColorSpan(Color.BLACK);

        TextPaint tp = new TextPaint();
        backgroundColorSpan.updateDrawState(tp);
        assertEquals(Color.BLACK, tp.bgColor);

        backgroundColorSpan = new BackgroundColorSpan(Color.BLUE);
        backgroundColorSpan.updateDrawState(tp);
        assertEquals(Color.BLUE, tp.bgColor);
    }

    @Test(expected=NullPointerException.class)
    public void testUpdateDrawStateNull() {
        BackgroundColorSpan backgroundColorSpan = new BackgroundColorSpan(Color.BLACK);

        backgroundColorSpan.updateDrawState(null);
    }

    @Test
    public void testGetBackgroundColor() {
        BackgroundColorSpan backgroundColorSpan = new BackgroundColorSpan(Color.CYAN);
        assertEquals(Color.CYAN, backgroundColorSpan.getBackgroundColor());

        backgroundColorSpan = new BackgroundColorSpan(Color.GRAY);
        assertEquals(Color.GRAY, backgroundColorSpan.getBackgroundColor());
    }

    @Test
    public void testDescribeContents() {
        BackgroundColorSpan backgroundColorSpan = new BackgroundColorSpan(Color.RED);
        backgroundColorSpan.describeContents();
    }

    @Test
    public void testGetSpanTypeId() {
        BackgroundColorSpan backgroundColorSpan = new BackgroundColorSpan(Color.RED);
        backgroundColorSpan.getSpanTypeId();
    }

    @Test
    public void testWriteToParcel() {
        Parcel p = Parcel.obtain();
        try {
            BackgroundColorSpan backgroundColorSpan = new BackgroundColorSpan(Color.RED);
            backgroundColorSpan.writeToParcel(p, 0);
            p.setDataPosition(0);
            BackgroundColorSpan b = new BackgroundColorSpan(p);
            assertEquals(Color.RED, b.getBackgroundColor());
        } finally {
            p.recycle();
        }

        p = Parcel.obtain();
        try {
            BackgroundColorSpan backgroundColorSpan = new BackgroundColorSpan(Color.MAGENTA);
            backgroundColorSpan.writeToParcel(p, 0);
            p.setDataPosition(0);
            BackgroundColorSpan b = new BackgroundColorSpan(p);
            assertEquals(Color.MAGENTA, b.getBackgroundColor());
        } finally {
            p.recycle();
        }
    }
}
