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
import static org.junit.Assert.assertTrue;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Parcel;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.Html;
import android.text.Spanned;
import android.text.style.BulletSpan;

import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class BulletSpanTest {

    @Test
    public void testDefaultConstructor() {
        BulletSpan bulletSpan = new BulletSpan();
        assertEquals(calculateLeadingMargin(bulletSpan.getGapWidth(), bulletSpan.getBulletRadius()),
                bulletSpan.getLeadingMargin(true));
        assertEquals(0, bulletSpan.getColor());
        assertEquals(BulletSpan.STANDARD_GAP_WIDTH, bulletSpan.getGapWidth());
        assertTrue(bulletSpan.getBulletRadius() > 0);
    }

    @Test
    public void testConstructorFromGapWidth() {
        BulletSpan bulletSpan = new BulletSpan(2);
        assertEquals(calculateLeadingMargin(2, bulletSpan.getBulletRadius()),
                bulletSpan.getLeadingMargin(true));
        assertEquals(0, bulletSpan.getColor());
        assertEquals(2, bulletSpan.getGapWidth());
        assertTrue(bulletSpan.getBulletRadius() > 0);
    }

    @Test
    public void testConstructorFromGapWidthColor() {
        BulletSpan bulletSpan = new BulletSpan(2, Color.RED);
        assertEquals(bulletSpan.getBulletRadius() * 2 + bulletSpan.getGapWidth(),
                bulletSpan.getLeadingMargin(true));
        assertEquals(Color.RED, bulletSpan.getColor());
        assertEquals(2, bulletSpan.getGapWidth());
        assertTrue(bulletSpan.getBulletRadius() > 0);
    }

    @Test
    public void testConstructorFromGapWidthColorBulletRadius() {
        BulletSpan bulletSpan = new BulletSpan(2, Color.RED, 10);
        assertEquals(calculateLeadingMargin(2, 10), bulletSpan.getLeadingMargin(true));
        assertEquals(Color.RED, bulletSpan.getColor());
        assertEquals(2, bulletSpan.getGapWidth());
        assertEquals(10, bulletSpan.getBulletRadius());
    }

    @Test
    public void testGetLeadingMargin() {
        BulletSpan bulletSpan = new BulletSpan(1);
        int leadingMargin1 = bulletSpan.getLeadingMargin(true);

        bulletSpan = new BulletSpan(4);
        int leadingMargin2 = bulletSpan.getLeadingMargin(false);

        assertTrue(leadingMargin2 > leadingMargin1);
    }

    @Test
    public void testDrawLeadingMargin() {
        BulletSpan bulletSpan = new BulletSpan(10, 20);

        Canvas canvas = new Canvas();
        Paint paint = new Paint();
        Spanned text = Html.fromHtml("<b>hello</b>");

        bulletSpan.drawLeadingMargin(canvas, paint, 10, 0, 10, 0, 20, text, 0, 0, true, null);
    }

    @Test(expected = ClassCastException.class)
    public void testDrawLeadingMarginString() {
        BulletSpan bulletSpan = new BulletSpan(10, 20);

        String text = "cts test.";
        // Should throw ClassCastException when use a String as text
        bulletSpan.drawLeadingMargin(null, null, 0, 0, 0, 0, 0, text, 0, 0, true, null);
    }

    @Test(expected = NullPointerException.class)
    public void testDrawLeadingMarginNull() {
        BulletSpan bulletSpan = new BulletSpan(10, 20);

        // Should throw NullPointerException when text is null
        bulletSpan.drawLeadingMargin(null, null, 0, 0, 0, 0, 0, null, 0, 0, false, null);
    }

    @Test
    public void testDescribeContents() {
        BulletSpan bulletSpan = new BulletSpan();
        bulletSpan.describeContents();
    }

    @Test
    public void testGetSpanTypeId() {
        BulletSpan bulletSpan = new BulletSpan();
        bulletSpan.getSpanTypeId();
    }

    @Test
    public void testWriteToParcel() {
        int leadingMargin1 = 0;
        int leadingMargin2 = 0;

        Parcel p = Parcel.obtain();
        try {
            BulletSpan bulletSpan = new BulletSpan(2, Color.RED, 5);
            bulletSpan.writeToParcel(p, 0);
            p.setDataPosition(0);
            BulletSpan b = new BulletSpan(p);
            leadingMargin1 = b.getLeadingMargin(true);
            assertEquals(calculateLeadingMargin(2, 5), leadingMargin1);
            assertEquals(Color.RED, b.getColor());
            assertEquals(2, b.getGapWidth());
        } finally {
            p.recycle();
        }

        p = Parcel.obtain();
        try {
            BulletSpan bulletSpan = new BulletSpan();
            bulletSpan.writeToParcel(p, 0);
            p.setDataPosition(0);
            BulletSpan b = new BulletSpan(p);
            leadingMargin2 = b.getLeadingMargin(true);
            assertEquals(calculateLeadingMargin(b.getGapWidth(), b.getBulletRadius()),
                    leadingMargin2);
            assertEquals(0, bulletSpan.getColor());
            assertEquals(BulletSpan.STANDARD_GAP_WIDTH, bulletSpan.getGapWidth());
        } finally {
            p.recycle();
        }
    }

    private int calculateLeadingMargin(int gapWidth, int bulletRadius) {
        return 2 * bulletRadius + gapWidth;
    }
}
