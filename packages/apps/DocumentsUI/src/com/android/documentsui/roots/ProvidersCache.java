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

package com.android.documentsui.roots;

import static com.android.documentsui.base.SharedMinimal.DEBUG;
import static com.android.documentsui.base.SharedMinimal.VERBOSE;

import android.content.BroadcastReceiver.PendingResult;
import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ProviderInfo;
import android.content.pm.ResolveInfo;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemClock;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Root;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import com.android.documentsui.DocumentsApplication;
import com.android.documentsui.R;
import com.android.documentsui.archives.ArchivesProvider;
import com.android.documentsui.base.Providers;
import com.android.documentsui.base.RootInfo;
import com.android.documentsui.base.State;
import com.android.internal.annotations.GuardedBy;

import com.google.common.collect.ArrayListMultimap;
import com.google.common.collect.Multimap;

import libcore.io.IoUtils;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Cache of known storage backends and their roots.
 */
public class ProvidersCache implements ProvidersAccess {
    private static final String TAG = "ProvidersCache";

    // Not all providers are equally well written. If a provider returns
    // empty results we don't cache them...unless they're in this magical list
    // of beloved providers.
    private static final List<String> PERMIT_EMPTY_CACHE = new ArrayList<String>() {{
        // MTP provider commonly returns no roots (if no devices are attached).
        add(Providers.AUTHORITY_MTP);
        // ArchivesProvider doesn't support any roots.
        add(ArchivesProvider.AUTHORITY);
    }};

    private final Context mContext;
    private final ContentObserver mObserver;

    private final RootInfo mRecentsRoot;

    private final Object mLock = new Object();
    private final CountDownLatch mFirstLoad = new CountDownLatch(1);

    @GuardedBy("mLock")
    private boolean mFirstLoadDone;
    @GuardedBy("mLock")
    private PendingResult mBootCompletedResult;

    @GuardedBy("mLock")
    private Multimap<String, RootInfo> mRoots = ArrayListMultimap.create();
    @GuardedBy("mLock")
    private HashSet<String> mStoppedAuthorities = new HashSet<>();

    @GuardedBy("mObservedAuthoritiesDetails")
    private final Map<String, PackageDetails> mObservedAuthoritiesDetails = new HashMap<>();

    public ProvidersCache(Context context) {
        mContext = context;
        mObserver = new RootsChangedObserver();

        // Create a new anonymous "Recents" RootInfo. It's a faker.
        mRecentsRoot = new RootInfo() {{
                // Special root for recents
                derivedIcon = R.drawable.ic_root_recent;
                derivedType = RootInfo.TYPE_RECENTS;
                flags = Root.FLAG_LOCAL_ONLY | Root.FLAG_SUPPORTS_IS_CHILD;
                title = mContext.getString(R.string.root_recent);
                availableBytes = -1;
            }};
    }

    private class RootsChangedObserver extends ContentObserver {
        public RootsChangedObserver() {
            super(new Handler());
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            if (uri == null) {
                Log.w(TAG, "Received onChange event for null uri. Skipping.");
                return;
            }
            if (DEBUG) Log.i(TAG, "Updating roots due to change at " + uri);
            updateAuthorityAsync(uri.getAuthority());
        }
    }

    @Override
    public String getApplicationName(String authority) {
        return mObservedAuthoritiesDetails.get(authority).applicationName;
    }

    @Override
    public String getPackageName(String authority) {
        return mObservedAuthoritiesDetails.get(authority).packageName;
    }

    public void updateAsync(boolean forceRefreshAll) {

        // NOTE: This method is called when the UI language changes.
        // For that reason we update our RecentsRoot to reflect
        // the current language.
        mRecentsRoot.title = mContext.getString(R.string.root_recent);

        // Nothing else about the root should ever change.
        assert(mRecentsRoot.authority == null);
        assert(mRecentsRoot.rootId == null);
        assert(mRecentsRoot.derivedIcon == R.drawable.ic_root_recent);
        assert(mRecentsRoot.derivedType == RootInfo.TYPE_RECENTS);
        assert(mRecentsRoot.flags == (Root.FLAG_LOCAL_ONLY | Root.FLAG_SUPPORTS_IS_CHILD));
        assert(mRecentsRoot.availableBytes == -1);

        new UpdateTask(forceRefreshAll, null)
                .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void updatePackageAsync(String packageName) {
        new UpdateTask(false, packageName).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void updateAuthorityAsync(String authority) {
        final ProviderInfo info = mContext.getPackageManager().resolveContentProvider(authority, 0);
        if (info != null) {
            updatePackageAsync(info.packageName);
        }
    }

    void setBootCompletedResult(PendingResult result) {
        synchronized (mLock) {
            // Quickly check if we've already finished loading, otherwise hang
            // out until first pass is finished.
            if (mFirstLoadDone) {
                result.finish();
            } else {
                mBootCompletedResult = result;
            }
        }
    }

    /**
     * Block until the first {@link UpdateTask} pass has finished.
     *
     * @return {@code true} if cached roots is ready to roll, otherwise
     *         {@code false} if we timed out while waiting.
     */
    private boolean waitForFirstLoad() {
        boolean success = false;
        try {
            success = mFirstLoad.await(15, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
        }
        if (!success) {
            Log.w(TAG, "Timeout waiting for first update");
        }
        return success;
    }

    /**
     * Load roots from authorities that are in stopped state. Normal
     * {@link UpdateTask} passes ignore stopped applications.
     */
    private void loadStoppedAuthorities() {
        final ContentResolver resolver = mContext.getContentResolver();
        synchronized (mLock) {
            for (String authority : mStoppedAuthorities) {
                mRoots.replaceValues(authority, loadRootsForAuthority(resolver, authority, true));
            }
            mStoppedAuthorities.clear();
        }
    }

    /**
     * Load roots from a stopped authority. Normal {@link UpdateTask} passes
     * ignore stopped applications.
     */
    private void loadStoppedAuthority(String authority) {
        final ContentResolver resolver = mContext.getContentResolver();
        synchronized (mLock) {
            if (!mStoppedAuthorities.contains(authority)) {
                return;
            }
            if (DEBUG) Log.d(TAG, "Loading stopped authority " + authority);
            mRoots.replaceValues(authority, loadRootsForAuthority(resolver, authority, true));
            mStoppedAuthorities.remove(authority);
        }
    }

    /**
     * Bring up requested provider and query for all active roots. Will consult cached
     * roots if not forceRefresh. Will query when cached roots is empty (which should never happen).
     */
    private Collection<RootInfo> loadRootsForAuthority(
            ContentResolver resolver, String authority, boolean forceRefresh) {
        if (VERBOSE) Log.v(TAG, "Loading roots for " + authority);

        final ArrayList<RootInfo> roots = new ArrayList<>();
        final PackageManager pm = mContext.getPackageManager();
        ProviderInfo provider = pm.resolveContentProvider(
                authority, PackageManager.GET_META_DATA);
        if (provider == null) {
            Log.w(TAG, "Failed to get provider info for " + authority);
            return roots;
        }
        if (!provider.exported) {
            Log.w(TAG, "Provider is not exported. Failed to load roots for " + authority);
            return roots;
        }
        if (!provider.grantUriPermissions) {
            Log.w(TAG, "Provider doesn't grantUriPermissions. Failed to load roots for "
                    + authority);
            return roots;
        }
        if (!android.Manifest.permission.MANAGE_DOCUMENTS.equals(provider.readPermission)
                || !android.Manifest.permission.MANAGE_DOCUMENTS.equals(provider.writePermission)) {
            Log.w(TAG, "Provider is not protected by MANAGE_DOCUMENTS. Failed to load roots for "
                    + authority);
            return roots;
        }

        synchronized (mObservedAuthoritiesDetails) {
            if (!mObservedAuthoritiesDetails.containsKey(authority)) {
                CharSequence appName = pm.getApplicationLabel(provider.applicationInfo);
                String packageName = provider.applicationInfo.packageName;

                mObservedAuthoritiesDetails.put(
                        authority, new PackageDetails(appName.toString(), packageName));

                // Watch for any future updates
                final Uri rootsUri = DocumentsContract.buildRootsUri(authority);
                mContext.getContentResolver().registerContentObserver(rootsUri, true, mObserver);
            }
        }

        final Uri rootsUri = DocumentsContract.buildRootsUri(authority);
        if (!forceRefresh) {
            // Look for roots data that we might have cached for ourselves in the
            // long-lived system process.
            final Bundle systemCache = resolver.getCache(rootsUri);
            if (systemCache != null) {
                ArrayList<RootInfo> cachedRoots = systemCache.getParcelableArrayList(TAG);
                assert(cachedRoots != null);
                if (!cachedRoots.isEmpty() || PERMIT_EMPTY_CACHE.contains(authority)) {
                    if (VERBOSE) Log.v(TAG, "System cache hit for " + authority);
                    return cachedRoots;
                } else {
                    Log.w(TAG, "Ignoring empty system cache hit for " + authority);
                }
            }
        }

        ContentProviderClient client = null;
        Cursor cursor = null;
        try {
            client = DocumentsApplication.acquireUnstableProviderOrThrow(resolver, authority);
            cursor = client.query(rootsUri, null, null, null, null);
            while (cursor.moveToNext()) {
                final RootInfo root = RootInfo.fromRootsCursor(authority, cursor);
                roots.add(root);
            }
        } catch (Exception e) {
            Log.w(TAG, "Failed to load some roots from " + authority, e);
            // We didn't load every root from the provider. Don't put it to
            // system cache so that we'll try loading them again next time even
            // if forceRefresh is false.
            return roots;
        } finally {
            IoUtils.closeQuietly(cursor);
            ContentProviderClient.releaseQuietly(client);
        }

        // Cache these freshly parsed roots over in the long-lived system
        // process, in case our process goes away. The system takes care of
        // invalidating the cache if the package or Uri changes.
        final Bundle systemCache = new Bundle();
        if (roots.isEmpty() && !PERMIT_EMPTY_CACHE.contains(authority)) {
            Log.i(TAG, "Provider returned no roots. Possibly naughty: " + authority);
        } else {
            systemCache.putParcelableArrayList(TAG, roots);
            resolver.putCache(rootsUri, systemCache);
        }

        return roots;
    }

    @Override
    public RootInfo getRootOneshot(String authority, String rootId) {
        return getRootOneshot(authority, rootId, false);
    }

    public RootInfo getRootOneshot(String authority, String rootId, boolean forceRefresh) {
        synchronized (mLock) {
            RootInfo root = forceRefresh ? null : getRootLocked(authority, rootId);
            if (root == null) {
                mRoots.replaceValues(authority, loadRootsForAuthority(
                                mContext.getContentResolver(), authority, forceRefresh));
                root = getRootLocked(authority, rootId);
            }
            return root;
        }
    }

    public RootInfo getRootBlocking(String authority, String rootId) {
        waitForFirstLoad();
        loadStoppedAuthorities();
        synchronized (mLock) {
            return getRootLocked(authority, rootId);
        }
    }

    private RootInfo getRootLocked(String authority, String rootId) {
        for (RootInfo root : mRoots.get(authority)) {
            if (Objects.equals(root.rootId, rootId)) {
                return root;
            }
        }
        return null;
    }

    @Override
    public RootInfo getRecentsRoot() {
        return mRecentsRoot;
    }

    public boolean isRecentsRoot(RootInfo root) {
        return mRecentsRoot.equals(root);
    }

    @Override
    public Collection<RootInfo> getRootsBlocking() {
        waitForFirstLoad();
        loadStoppedAuthorities();
        synchronized (mLock) {
            return mRoots.values();
        }
    }

    @Override
    public Collection<RootInfo> getMatchingRootsBlocking(State state) {
        waitForFirstLoad();
        loadStoppedAuthorities();
        synchronized (mLock) {
            return ProvidersAccess.getMatchingRoots(mRoots.values(), state);
        }
    }

    @Override
    public Collection<RootInfo> getRootsForAuthorityBlocking(String authority) {
        waitForFirstLoad();
        loadStoppedAuthority(authority);
        synchronized (mLock) {
            final Collection<RootInfo> roots = mRoots.get(authority);
            return roots != null ? roots : Collections.<RootInfo>emptyList();
        }
    }

    @Override
    public RootInfo getDefaultRootBlocking(State state) {
        for (RootInfo root : ProvidersAccess.getMatchingRoots(getRootsBlocking(), state)) {
            if (root.isDownloads()) {
                return root;
            }
        }
        return mRecentsRoot;
    }

    public void logCache() {
        ContentResolver resolver = mContext.getContentResolver();
        StringBuilder output = new StringBuilder();

        for (String authority : mObservedAuthoritiesDetails.keySet()) {
            List<String> roots = new ArrayList<>();
            Uri rootsUri = DocumentsContract.buildRootsUri(authority);
            Bundle systemCache = resolver.getCache(rootsUri);
            if (systemCache != null) {
                ArrayList<RootInfo> cachedRoots = systemCache.getParcelableArrayList(TAG);
                for (RootInfo root : cachedRoots) {
                    roots.add(root.toDebugString());
                }
            }

            output.append((output.length() == 0) ? "System cache: " : ", ");
            output.append(authority).append("=").append(roots);
        }

        Log.i(TAG, output.toString());
    }

    private class UpdateTask extends AsyncTask<Void, Void, Void> {
        private final boolean mForceRefreshAll;
        private final String mForceRefreshPackage;

        private final Multimap<String, RootInfo> mTaskRoots = ArrayListMultimap.create();
        private final HashSet<String> mTaskStoppedAuthorities = new HashSet<>();

        /**
         * Create task to update roots cache.
         *
         * @param forceRefreshAll when true, all previously cached values for
         *            all packages should be ignored.
         * @param forceRefreshPackage when non-null, all previously cached
         *            values for this specific package should be ignored.
         */
        public UpdateTask(boolean forceRefreshAll, String forceRefreshPackage) {
            mForceRefreshAll = forceRefreshAll;
            mForceRefreshPackage = forceRefreshPackage;
        }

        @Override
        protected Void doInBackground(Void... params) {
            final long start = SystemClock.elapsedRealtime();

            mTaskRoots.put(mRecentsRoot.authority, mRecentsRoot);

            final PackageManager pm = mContext.getPackageManager();

            // Pick up provider with action string
            final Intent intent = new Intent(DocumentsContract.PROVIDER_INTERFACE);
            final List<ResolveInfo> providers = pm.queryIntentContentProviders(intent, 0);
            for (ResolveInfo info : providers) {
                ProviderInfo providerInfo = info.providerInfo;
                if (providerInfo.authority != null) {
                    handleDocumentsProvider(providerInfo);
                }
            }

            final long delta = SystemClock.elapsedRealtime() - start;
            if (VERBOSE) Log.v(TAG,
                    "Update found " + mTaskRoots.size() + " roots in " + delta + "ms");
            synchronized (mLock) {
                mFirstLoadDone = true;
                if (mBootCompletedResult != null) {
                    mBootCompletedResult.finish();
                    mBootCompletedResult = null;
                }
                mRoots = mTaskRoots;
                mStoppedAuthorities = mTaskStoppedAuthorities;
            }
            mFirstLoad.countDown();
            LocalBroadcastManager.getInstance(mContext).sendBroadcast(new Intent(BROADCAST_ACTION));
            return null;
        }

        private void handleDocumentsProvider(ProviderInfo info) {
            // Ignore stopped packages for now; we might query them
            // later during UI interaction.
            if ((info.applicationInfo.flags & ApplicationInfo.FLAG_STOPPED) != 0) {
                if (VERBOSE) Log.v(TAG, "Ignoring stopped authority " + info.authority);
                mTaskStoppedAuthorities.add(info.authority);
                return;
            }

            final boolean forceRefresh = mForceRefreshAll
                    || Objects.equals(info.packageName, mForceRefreshPackage);
            mTaskRoots.putAll(info.authority, loadRootsForAuthority(mContext.getContentResolver(),
                    info.authority, forceRefresh));
        }

    }

    private static class PackageDetails {
        private String applicationName;
        private String packageName;

        public PackageDetails(String appName, String pckgName) {
            applicationName = appName;
            packageName = pckgName;
        }
    }
}
