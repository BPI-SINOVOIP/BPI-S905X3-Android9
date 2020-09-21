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

import android.app.PendingIntent;
import android.app.RemoteInput;
import android.app.slice.Slice;
import android.app.slice.SliceItem;
import android.app.slice.SliceSpec;
import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.Icon;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcel;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

@RunWith(AndroidJUnit4.class)
public class SliceBuilderTest {

    private static final Uri BASE_URI = Uri.parse("content://android.slice.cts/");
    private static final SliceSpec SPEC = new SliceSpec("", 1);
    private final Context mContext = InstrumentationRegistry.getContext();

    @Test
    public void testInt() {
        Slice s = new Slice.Builder(BASE_URI, SPEC)
                .addInt(0xff121212, "subtype", Arrays.asList(Slice.HINT_TITLE))
                .build();
        assertEquals(BASE_URI, s.getUri());
        assertEquals(1, s.getItems().size());

        SliceItem item = s.getItems().get(0);
        assertEquals(SliceItem.FORMAT_INT, item.getFormat());
        assertEquals(0xff121212, item.getInt());
        assertEquals("subtype", item.getSubType());
        assertTrue(item.hasHint(Slice.HINT_TITLE));

        verifySlice(s);
    }

    @Test
    public void testIcon() {
        Icon i = Icon.createWithResource(mContext, R.drawable.size_48x48);
        Slice s = new Slice.Builder(BASE_URI, SPEC)
                .addIcon(i, "subtype", Arrays.asList(Slice.HINT_TITLE))
                .build();
        assertEquals(BASE_URI, s.getUri());
        assertEquals(1, s.getItems().size());

        SliceItem item = s.getItems().get(0);
        assertEquals(SliceItem.FORMAT_IMAGE, item.getFormat());
        assertEquals(i, item.getIcon());
        assertEquals("subtype", item.getSubType());
        assertTrue(item.hasHint(Slice.HINT_TITLE));

        verifySlice(s);
    }

    @Test
    public void testText() {
        CharSequence i = "Some text";
        Slice s = new Slice.Builder(BASE_URI, SPEC)
                .addText(i, "subtype", Arrays.asList(Slice.HINT_TITLE))
                .build();
        assertEquals(BASE_URI, s.getUri());
        assertEquals(1, s.getItems().size());

        SliceItem item = s.getItems().get(0);
        assertEquals(SliceItem.FORMAT_TEXT, item.getFormat());
        assertEquals(i, item.getText());
        assertEquals("subtype", item.getSubType());
        assertTrue(item.hasHint(Slice.HINT_TITLE));

        verifySlice(s);
    }

    @Test
    public void testLong() {
        long i = 43L;
        Slice s = new Slice.Builder(BASE_URI, SPEC)
                .addLong(i, "subtype", Arrays.asList(Slice.HINT_TITLE))
                .build();
        assertEquals(BASE_URI, s.getUri());
        assertEquals(1, s.getItems().size());

        SliceItem item = s.getItems().get(0);
        assertEquals(SliceItem.FORMAT_LONG, item.getFormat());
        assertEquals(i, item.getLong());
        assertEquals("subtype", item.getSubType());
        assertTrue(item.hasHint(Slice.HINT_TITLE));

        verifySlice(s);
    }

    @Test
    public void testRemoteInput() {
        RemoteInput i = new RemoteInput.Builder("key").build();
        Slice s = new Slice.Builder(BASE_URI, SPEC)
                .addRemoteInput(i, "subtype", Arrays.asList(Slice.HINT_TITLE))
                .build();
        assertEquals(BASE_URI, s.getUri());
        assertEquals(1, s.getItems().size());

        SliceItem item = s.getItems().get(0);
        assertEquals(SliceItem.FORMAT_REMOTE_INPUT, item.getFormat());
        assertEquals(i, item.getRemoteInput());
        assertEquals("subtype", item.getSubType());
        assertTrue(item.hasHint(Slice.HINT_TITLE));

        verifySlice(s);
    }

    @Test
    public void testBundle() {
        Bundle i = new Bundle();
        Slice s = new Slice.Builder(BASE_URI, SPEC)
                .addBundle(i, "subtype", Arrays.asList(Slice.HINT_TITLE))
                .build();
        assertEquals(BASE_URI, s.getUri());
        assertEquals(1, s.getItems().size());

        SliceItem item = s.getItems().get(0);
        assertEquals(SliceItem.FORMAT_BUNDLE, item.getFormat());
        assertEquals(i, item.getBundle());
        assertEquals("subtype", item.getSubType());
        assertTrue(item.hasHint(Slice.HINT_TITLE));

        verifySlice(s);
    }

    @Test
    public void testActionSubtype() {
        PendingIntent i = PendingIntent.getActivity(mContext, 0, new Intent(), 0);
        Slice subSlice = new Slice.Builder(BASE_URI.buildUpon().appendPath("s").build(), SPEC)
                .build();
        Slice s = new Slice.Builder(BASE_URI, SPEC)
                .addAction(i, subSlice, "subtype")
                .build();
        assertEquals(BASE_URI, s.getUri());
        assertEquals(1, s.getItems().size());

        SliceItem item = s.getItems().get(0);
        assertEquals(SliceItem.FORMAT_ACTION, item.getFormat());
        assertEquals(i, item.getAction());
        assertEquals("subtype", item.getSubType());
        assertEquals(subSlice, item.getSlice());

        verifySlice(s);
    }

    @Test
    public void testSubsliceSubtype() {
        Slice subSlice = new Slice.Builder(BASE_URI.buildUpon().appendPath("s").build(), SPEC)
                .build();
        Slice s = new Slice.Builder(BASE_URI, SPEC)
                .addSubSlice(subSlice, "subtype")
                .build();
        assertEquals(BASE_URI, s.getUri());
        assertEquals(1, s.getItems().size());

        SliceItem item = s.getItems().get(0);
        assertEquals(SliceItem.FORMAT_SLICE, item.getFormat());
        assertEquals("subtype", item.getSubType());
        assertEquals(subSlice, item.getSlice());

        verifySlice(s);
    }

    @Test
    public void testSpec() {
        Slice s = new Slice.Builder(BASE_URI, new SliceSpec("spec", 3))
                .build();
        assertEquals(BASE_URI, s.getUri());
        assertEquals(0, s.getItems().size());
        assertEquals(new SliceSpec("spec", 3), s.getSpec());

        verifySlice(s);
    }

    @Test
    public void testCallerNeeded() {
        Slice s = new Slice.Builder(BASE_URI, SPEC)
                .setCallerNeeded(true)
                .build();
        assertEquals(BASE_URI, s.getUri());
        assertEquals(0, s.getItems().size());
        assertTrue(s.isCallerNeeded());

        s = new Slice.Builder(BASE_URI, SPEC)
                .build();
        assertEquals(BASE_URI, s.getUri());
        assertEquals(0, s.getItems().size());
        assertFalse(s.isCallerNeeded());

        verifySlice(s);
    }

    private void verifySlice(Slice s) {
        Parcel p = Parcel.obtain();
        s.writeToParcel(p, 0);

        p.setDataPosition(0);
        Slice other = Slice.CREATOR.createFromParcel(p);
        assertEquivalent(s, other);
        p.recycle();
    }

    private void assertEquivalent(Slice s, Slice other) {
        assertEquals(s.getUri(), other.getUri());
        assertEquivalentHints(s.getHints(), other.getHints());
        assertEquivalent(s.getItems(), other.getItems());
        assertEquals(s.getSpec(), other.getSpec());
    }

    private void assertEquivalent(List<SliceItem> i1, List<SliceItem> i2) {
        assertEquals(i1.size(), i2.size());
        for (int i = 0; i < i1.size(); i++) {
            assertEquivalent(i1.get(i), i2.get(i));
        }
    }

    private void assertEquivalent(SliceItem s1, SliceItem s2) {
        assertEquals(s1.getFormat(), s2.getFormat());
        assertEquals(s1.getSubType(), s2.getSubType());
        assertEquivalentHints(s1.getHints(), s2.getHints());
        // TODO: Consider comparing content.
    }

    private void assertEquivalentHints(List<String> h1, List<String> h2) {
        assertEquals(h1.size(), h2.size());
        Collections.sort(h1);
        Collections.sort(h2);
        assertEquals(h1, h2);
    }
}
