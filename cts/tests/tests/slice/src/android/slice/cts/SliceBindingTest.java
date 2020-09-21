/*
 * Copyright (C) 2017 The Android Open Source Project
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
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

import android.app.PendingIntent.CanceledException;
import android.app.slice.Slice;
import android.app.slice.SliceItem;
import android.app.slice.SliceManager;
import android.app.slice.SliceSpec;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.drawable.Icon;
import android.net.Uri;
import android.os.Bundle;
import android.slice.cts.SliceProvider.TestParcel;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

@RunWith(AndroidJUnit4.class)
public class SliceBindingTest {

    public static boolean sFlag = false;

    private static final Uri BASE_URI = Uri.parse("content://android.slice.cts/");
    private final Context mContext = InstrumentationRegistry.getContext();
    private final SliceManager mSliceManager = mContext.getSystemService(SliceManager.class);

    @Test
    public void testProcess() {
        sFlag = false;
        mSliceManager.bindSlice(BASE_URI.buildUpon().appendPath("set_flag").build(),
                Collections.emptySet());
        assertFalse(sFlag);
    }

    @Test
    public void testType() {
        assertEquals(SliceProvider.SLICE_TYPE,
                mContext.getContentResolver().getType(BASE_URI));
    }

    @Test
    public void testSliceUri() {
        Slice s = mSliceManager.bindSlice(BASE_URI,
                Collections.emptySet());
        assertEquals(BASE_URI, s.getUri());
    }

    @Test
    public void testSubSlice() {
        Uri uri = BASE_URI.buildUpon().appendPath("subslice").build();
        Slice s = mSliceManager.bindSlice(uri, Collections.emptySet());
        assertEquals(uri, s.getUri());
        assertEquals(1, s.getItems().size());

        SliceItem item = s.getItems().get(0);
        assertEquals(SliceItem.FORMAT_SLICE, item.getFormat());
        assertEquals("subslice", item.getSubType());
        // The item should start with the same Uri as the parent, but be different.
        assertTrue(item.getSlice().getUri().toString().startsWith(uri.toString()));
        assertNotEquals(uri, item.getSlice().getUri());
    }

    @Test
    public void testText() {
        Uri uri = BASE_URI.buildUpon().appendPath("text").build();
        Slice s = mSliceManager.bindSlice(uri,
                Collections.emptySet());
        assertEquals(uri, s.getUri());
        assertEquals(1, s.getItems().size());

        SliceItem item = s.getItems().get(0);
        assertEquals(SliceItem.FORMAT_TEXT, item.getFormat());
        // TODO: Test spannables here.
        assertEquals("Expected text", item.getText());
    }

    @Test
    public void testIcon() {
        Uri uri = BASE_URI.buildUpon().appendPath("icon").build();
        Slice s = mSliceManager.bindSlice(uri,
                Collections.emptySet());
        assertEquals(uri, s.getUri());
        assertEquals(1, s.getItems().size());

        SliceItem item = s.getItems().get(0);
        assertEquals(SliceItem.FORMAT_IMAGE, item.getFormat());
        assertEquals(Icon.createWithResource(mContext, R.drawable.size_48x48).toString(),
                item.getIcon().toString());
    }

    @Test
    public void testAction() {
        sFlag = false;
        CountDownLatch latch = new CountDownLatch(1);
        BroadcastReceiver receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                sFlag = true;
                latch.countDown();
            }
        };
        mContext.registerReceiver(receiver,
                new IntentFilter(mContext.getPackageName() + ".action"));
        Uri uri = BASE_URI.buildUpon().appendPath("action").build();
        Slice s = mSliceManager.bindSlice(uri, Collections.emptySet());
        assertEquals(uri, s.getUri());
        assertEquals(1, s.getItems().size());

        SliceItem item = s.getItems().get(0);
        assertEquals(SliceItem.FORMAT_ACTION, item.getFormat());
        try {
            item.getAction().send();
        } catch (CanceledException e) {
        }

        try {
            latch.await(100, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        assertTrue(sFlag);
        mContext.unregisterReceiver(receiver);
    }

    @Test
    public void testInt() {
        Uri uri = BASE_URI.buildUpon().appendPath("int").build();
        Slice s = mSliceManager.bindSlice(uri, Collections.emptySet());
        assertEquals(uri, s.getUri());
        assertEquals(1, s.getItems().size());

        SliceItem item = s.getItems().get(0);
        assertEquals(SliceItem.FORMAT_INT, item.getFormat());
        assertEquals(0xff121212, item.getInt());
    }

    @Test
    public void testTimestamp() {
        Uri uri = BASE_URI.buildUpon().appendPath("timestamp").build();
        Slice s = mSliceManager.bindSlice(uri,
                Collections.emptySet());
        assertEquals(uri, s.getUri());
        assertEquals(1, s.getItems().size());

        SliceItem item = s.getItems().get(0);
        assertEquals(SliceItem.FORMAT_LONG, item.getFormat());
        assertEquals(43, item.getLong());
    }

    @Test
    public void testHints() {
        // Note this tests that hints are propagated through to the client but not that any specific
        // hints have any effects.
        Uri uri = BASE_URI.buildUpon().appendPath("hints").build();
        Slice s = mSliceManager.bindSlice(uri, Collections.emptySet());
        assertEquals(uri, s.getUri());

        assertEquals(Arrays.asList(Slice.HINT_LIST), s.getHints());
        assertEquals(Arrays.asList(Slice.HINT_TITLE), s.getItems().get(0).getHints());
        assertEquals(Arrays.asList(Slice.HINT_NO_TINT, Slice.HINT_LARGE),
                s.getItems().get(1).getHints());
    }

    @Test
    public void testHasHints() {
        Uri uri = BASE_URI.buildUpon().appendPath("hints").build();
        Slice s = mSliceManager.bindSlice(uri, Collections.emptySet());

        assertTrue(s.getItems().get(0).hasHint(Slice.HINT_TITLE));
        assertFalse(s.getItems().get(0).hasHint(Slice.HINT_LIST));
    }

    @Test
    public void testBundle() {
        Uri uri = BASE_URI.buildUpon().appendPath("bundle").build();
        Slice s = mSliceManager.bindSlice(uri, Collections.emptySet());
        assertEquals(uri, s.getUri());
        assertEquals(1, s.getItems().size());

        SliceItem item = s.getItems().get(0);
        assertEquals(SliceItem.FORMAT_BUNDLE, item.getFormat());
        Bundle b = item.getBundle();
        b.setClassLoader(getClass().getClassLoader());
        assertEquals(new TestParcel(), b.getParcelable("a"));
    }

    @Test
    public void testGetDescendants() {
        Collection<Uri> allUris = mSliceManager.getSliceDescendants(BASE_URI);
        assertEquals(SliceProvider.PATHS.length, allUris.size());
        Iterator<Uri> it = allUris.iterator();
        for (int i = 0; i < SliceProvider.PATHS.length; i++) {
            assertEquals(SliceProvider.PATHS[i], it.next().getPath());
        }

        assertEquals(0, mSliceManager.getSliceDescendants(
                BASE_URI.buildUpon().appendPath("/nothing").build()).size());
    }

    @Test
    public void testGetSliceSpec() {
        Uri uri = BASE_URI.buildUpon().appendPath("spec").build();
        Slice s = mSliceManager.bindSlice(uri, Collections.emptySet());
        assertEquals(new SliceSpec(SliceProvider.SPEC_TYPE, SliceProvider.SPEC_REV), s.getSpec());
    }
}
