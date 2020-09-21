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

import static java.util.stream.Collectors.toList;

import android.app.PendingIntent;
import android.app.slice.Slice;
import android.app.slice.Slice.Builder;
import android.app.slice.SliceSpec;
import android.content.ContentResolver;
import android.content.Intent;
import android.graphics.drawable.Icon;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcel;
import android.os.Parcelable;

import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Set;

public class SliceProvider extends android.app.slice.SliceProvider {

    static final String[] PATHS = new String[]{
            "/set_flag",
            "/subslice",
            "/text",
            "/icon",
            "/action",
            "/int",
            "/timestamp",
            "/hints",
            "/bundle",
            "/spec",
    };

    public static final String SPEC_TYPE = "android.cts.SliceType";
    public static final int SPEC_REV = 4;

    public static final SliceSpec SPEC = new SliceSpec(SPEC_TYPE, SPEC_REV);

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public Collection<Uri> onGetSliceDescendants(Uri uri) {
        if (uri.getPath().equals("/")) {
            Uri.Builder builder = new Uri.Builder()
                    .scheme(ContentResolver.SCHEME_CONTENT)
                    .authority("android.slice.cts");
            return Arrays.asList(PATHS).stream().map(s ->
                    builder.path(s).build()).collect(toList());
        }
        return Collections.emptyList();
    }

    @Override
    public Slice onBindSlice(Uri sliceUri, Set<SliceSpec> supportedSpecs) {
        switch (sliceUri.getPath()) {
            case "/set_flag":
                SliceBindingTest.sFlag = true;
                break;
            case "/subslice":
                Builder b = new Builder(sliceUri, SPEC);
                return b.addSubSlice(new Slice.Builder(b).build(), "subslice").build();
            case "/text":
                return new Slice.Builder(sliceUri, SPEC).addText("Expected text", "text",
                        Collections.emptyList()).build();
            case "/icon":
                return new Slice.Builder(sliceUri, SPEC).addIcon(
                        Icon.createWithResource(getContext(), R.drawable.size_48x48), "icon",
                        Collections.emptyList()).build();
            case "/action":
                Builder builder = new Builder(sliceUri, SPEC);
                Slice subSlice = new Slice.Builder(builder).build();
                PendingIntent broadcast = PendingIntent.getBroadcast(getContext(), 0,
                        new Intent(getContext().getPackageName() + ".action"), 0);
                return builder.addAction(broadcast, subSlice, "action").build();
            case "/int":
                return new Slice.Builder(sliceUri, SPEC).addInt(0xff121212, "int",
                        Collections.emptyList()).build();
            case "/timestamp":
                return new Slice.Builder(sliceUri, SPEC).addLong(43, "timestamp",
                        Collections.emptyList()).build();
            case "/hints":
                return new Slice.Builder(sliceUri, SPEC)
                        .addHints(Arrays.asList(Slice.HINT_LIST))
                        .addText("Text", null, Arrays.asList(Slice.HINT_TITLE))
                        .addIcon(Icon.createWithResource(getContext(), R.drawable.size_48x48),
                                null, Arrays.asList(Slice.HINT_NO_TINT, Slice.HINT_LARGE))
                        .build();
            case "/bundle":
                Bundle b1 = new Bundle();
                b1.putParcelable("a", new TestParcel());
                return new Slice.Builder(sliceUri, SPEC).addBundle(b1, "bundle",
                        Collections.emptyList()).build();
            case "/spec":
                return new Slice.Builder(sliceUri, SPEC)
                        .build();
        }
        return new Slice.Builder(sliceUri, SPEC).build();
    }

    public static class TestParcel implements Parcelable {

        private final int mValue;

        public TestParcel() {
            mValue = 42;
        }

        protected TestParcel(Parcel in) {
            mValue = in.readInt();
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeInt(mValue);
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public boolean equals(Object obj) {
            try {
                TestParcel p = (TestParcel) obj;
                return p.mValue == mValue;
            } catch (ClassCastException e) {
                return false;
            }
        }

        public static final Creator<TestParcel> CREATOR = new Creator<TestParcel>() {
            @Override
            public TestParcel createFromParcel(Parcel in) {
                return new TestParcel(in);
            }

            @Override
            public TestParcel[] newArray(int size) {
                return new TestParcel[size];
            }
        };
    }
}
