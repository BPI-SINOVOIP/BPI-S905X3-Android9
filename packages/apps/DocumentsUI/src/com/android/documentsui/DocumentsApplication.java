/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.documentsui;

import android.app.ActivityManager;
import android.app.Application;
import android.content.BroadcastReceiver;
import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.RemoteException;
import android.text.format.DateUtils;

import com.android.documentsui.base.Lookup;
import com.android.documentsui.clipping.ClipStorage;
import com.android.documentsui.clipping.ClipStore;
import com.android.documentsui.clipping.DocumentClipper;
import com.android.documentsui.roots.ProvidersCache;

public class DocumentsApplication extends Application {
    private static final long PROVIDER_ANR_TIMEOUT = 20 * DateUtils.SECOND_IN_MILLIS;

    private ProvidersCache mProviders;
    private ThumbnailCache mThumbnailCache;
    private ClipStorage mClipStore;
    private DocumentClipper mClipper;
    private DragAndDropManager mDragAndDropManager;
    private Lookup<String, String> mFileTypeLookup;

    public static ProvidersCache getProvidersCache(Context context) {
        return ((DocumentsApplication) context.getApplicationContext()).mProviders;
    }

    public static ThumbnailCache getThumbnailCache(Context context) {
        final DocumentsApplication app = (DocumentsApplication) context.getApplicationContext();
        return app.mThumbnailCache;
    }

    public static ContentProviderClient acquireUnstableProviderOrThrow(
            ContentResolver resolver, String authority) throws RemoteException {
        final ContentProviderClient client = resolver.acquireUnstableContentProviderClient(
                authority);
        if (client == null) {
            throw new RemoteException("Failed to acquire provider for " + authority);
        }
        client.setDetectNotResponding(PROVIDER_ANR_TIMEOUT);
        return client;
    }

    public static DocumentClipper getDocumentClipper(Context context) {
        return ((DocumentsApplication) context.getApplicationContext()).mClipper;
    }

    public static ClipStore getClipStore(Context context) {
        return ((DocumentsApplication) context.getApplicationContext()).mClipStore;
    }

    public static DragAndDropManager getDragAndDropManager(Context context) {
        return ((DocumentsApplication) context.getApplicationContext()).mDragAndDropManager;
    }

    public static Lookup<String, String> getFileTypeLookup(Context context) {
        return ((DocumentsApplication) context.getApplicationContext()).mFileTypeLookup;
    }

    @Override
    public void onCreate() {
        super.onCreate();

        final ActivityManager am = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        final int memoryClassBytes = am.getMemoryClass() * 1024 * 1024;

        mProviders = new ProvidersCache(this);
        mProviders.updateAsync(false);

        mThumbnailCache = new ThumbnailCache(memoryClassBytes / 4);

        mClipStore = new ClipStorage(
                ClipStorage.prepareStorage(getCacheDir()),
                getSharedPreferences(ClipStorage.PREF_NAME, 0));
        mClipper = DocumentClipper.create(this, mClipStore);

        mDragAndDropManager = DragAndDropManager.create(this, mClipper);

        mFileTypeLookup = new FileTypeMap(this);

        final IntentFilter packageFilter = new IntentFilter();
        packageFilter.addAction(Intent.ACTION_PACKAGE_ADDED);
        packageFilter.addAction(Intent.ACTION_PACKAGE_CHANGED);
        packageFilter.addAction(Intent.ACTION_PACKAGE_REMOVED);
        packageFilter.addAction(Intent.ACTION_PACKAGE_DATA_CLEARED);
        packageFilter.addDataScheme("package");
        registerReceiver(mCacheReceiver, packageFilter);

        final IntentFilter localeFilter = new IntentFilter();
        localeFilter.addAction(Intent.ACTION_LOCALE_CHANGED);
        registerReceiver(mCacheReceiver, localeFilter);
    }

    @Override
    public void onTrimMemory(int level) {
        super.onTrimMemory(level);

        mThumbnailCache.onTrimMemory(level);
    }

    private BroadcastReceiver mCacheReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final Uri data = intent.getData();
            if (data != null) {
                final String packageName = data.getSchemeSpecificPart();
                mProviders.updatePackageAsync(packageName);
            } else {
                mProviders.updateAsync(true);
            }
        }
    };
}
