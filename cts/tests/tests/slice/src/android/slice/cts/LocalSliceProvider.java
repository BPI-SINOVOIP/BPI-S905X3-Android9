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

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.withSettings;

import android.app.PendingIntent;
import android.app.slice.Slice;
import android.app.slice.SliceManager;
import android.app.slice.SliceSpec;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.pm.ProviderInfo;
import android.net.Uri;

import org.mockito.Answers;
import org.mockito.stubbing.Answer;

import java.util.Collection;
import java.util.Set;

public class LocalSliceProvider extends SliceProvider {
    public static SliceProvider sProxy;
    public static Answer sAnswer;

    private SliceManager mSliceService;

    @Override
    public boolean onCreate() {
        return sProxy == null || sProxy.onCreate();
    }

    @Override
    public void attachInfo(Context context, ProviderInfo info) {
        mSliceService = mock(SliceManager.class, withSettings()
                .spiedInstance(context.getSystemService(Context.SLICE_SERVICE))
                .defaultAnswer(invocation -> {
                    Answer s = sAnswer != null ? sAnswer : Answers.CALLS_REAL_METHODS;
                    return s.answer(invocation);
                }));
        Context wrapped = new ContextWrapper(context) {
            @Override
            public Object getSystemService(String name) {
                if (Context.SLICE_SERVICE.equals(name)) {
                    return mSliceService;
                }
                return super.getSystemService(name);
            }
        };
        super.attachInfo(wrapped, info);
    }

    @Override
    public Slice onBindSlice(Uri sliceUri, Set<SliceSpec> specs) {
        if (sProxy != null) return sProxy.onBindSlice(sliceUri, specs);
        return super.onBindSlice(sliceUri, specs);
    }

    @Override
    public Uri onMapIntentToUri(Intent intent) {
        if (sProxy != null) return sProxy.onMapIntentToUri(intent);
        return super.onMapIntentToUri(intent);
    }

    @Override
    public Collection<Uri> onGetSliceDescendants(Uri uri) {
        if (sProxy != null) return sProxy.onGetSliceDescendants(uri);
        return super.onGetSliceDescendants(uri);
    }

    @Override
    public void onSlicePinned(Uri sliceUri) {
        if (sProxy != null) sProxy.onSlicePinned(sliceUri);
        super.onSlicePinned(sliceUri);
    }

    @Override
    public void onSliceUnpinned(Uri sliceUri) {
        if (sProxy != null) sProxy.onSliceUnpinned(sliceUri);
        super.onSliceUnpinned(sliceUri);
    }

    @Override
    public PendingIntent onCreatePermissionRequest(Uri sliceUri) {
        if (sProxy != null) return sProxy.onCreatePermissionRequest(sliceUri);
        return super.onCreatePermissionRequest(sliceUri);
    }
}
