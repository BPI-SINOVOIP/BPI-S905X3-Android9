/*
 * Copyright (C) 2015 The Android Open Source Project
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

package android.media.cts;

import android.media.MediaDescription;
import android.media.browse.MediaBrowser.MediaItem;
import android.media.session.MediaSession;
import android.media.session.MediaSessionManager;
import android.os.Bundle;
import android.service.media.MediaBrowserService;

import junit.framework.Assert;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Stub implementation of (@link android.service.media.MediaBrowserService}.
 */
public class StubMediaBrowserService extends MediaBrowserService {
    static final String EXTRAS_KEY = "test_extras_key";
    static final String EXTRAS_VALUE = "test_extras_value";

    static final String MEDIA_ID_INVALID = "test_media_id_invalid";
    static final String MEDIA_ID_ROOT = "test_media_id_root";
    static final String MEDIA_ID_CHILDREN_DELAYED = "test_media_id_children_delayed";

    static final String[] MEDIA_ID_CHILDREN = new String[] {
        "test_media_id_children_0", "test_media_id_children_1",
        "test_media_id_children_2", "test_media_id_children_3",
        MEDIA_ID_CHILDREN_DELAYED
    };

    static StubMediaBrowserService sInstance;

    static MediaSession sSession;
    static MediaSessionManager.RemoteUserInfo sBrowserInfo;

    private Bundle mExtras;
    private Result<List<MediaItem>> mPendingLoadChildrenResult;
    private Result<MediaItem> mPendingLoadItemResult;
    private Bundle mPendingRootHints;

    public static void clearBrowserInfo() {
        sBrowserInfo = null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        sInstance = this;
        sSession = new MediaSession(this, "StubMediaBrowserService");
        setSessionToken(sSession.getSessionToken());
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        sSession.release();
        sInstance = null;
        sSession = null;
    }

    @Override
    public BrowserRoot onGetRoot(String clientPackageName, int clientUid, Bundle rootHints) {
        sBrowserInfo = getCurrentBrowserInfo();
        mExtras = new Bundle();
        mExtras.putString(EXTRAS_KEY, EXTRAS_VALUE);
        return new BrowserRoot(MEDIA_ID_ROOT, mExtras);
    }

    @Override
    public void onLoadChildren(final String parentMediaId, final Result<List<MediaItem>> result) {
        List<MediaItem> mediaItems = new ArrayList<>();
        if (MEDIA_ID_ROOT.equals(parentMediaId)) {
            Bundle rootHints = getBrowserRootHints();
            for (String id : MEDIA_ID_CHILDREN) {
                mediaItems.add(new MediaItem(new MediaDescription.Builder()
                        .setMediaId(id).setExtras(rootHints).build(), MediaItem.FLAG_BROWSABLE));
            }
            sBrowserInfo = getCurrentBrowserInfo();
            result.sendResult(mediaItems);
        } else if (MEDIA_ID_CHILDREN_DELAYED.equals(parentMediaId)) {
            Assert.assertNull(mPendingLoadChildrenResult);
            mPendingLoadChildrenResult = result;
            mPendingRootHints = getBrowserRootHints();
            result.detach();
        } else if (MEDIA_ID_INVALID.equals(parentMediaId)) {
            result.sendResult(null);
        }
    }

    @Override
    public void onLoadItem(String itemId, Result<MediaItem> result) {
        if (MEDIA_ID_CHILDREN_DELAYED.equals(itemId)) {
            mPendingLoadItemResult = result;
            mPendingRootHints = getBrowserRootHints();
            result.detach();
            return;
        }

        for (String id : MEDIA_ID_CHILDREN) {
            if (id.equals(itemId)) {
                result.sendResult(new MediaItem(new MediaDescription.Builder()
                        .setMediaId(id).setExtras(getBrowserRootHints()).build(),
                                MediaItem.FLAG_BROWSABLE));
                sBrowserInfo = getCurrentBrowserInfo();
                return;
            }
        }

        super.onLoadItem(itemId, result);
    }

    public void sendDelayedNotifyChildrenChanged() {
        if (mPendingLoadChildrenResult != null) {
            mPendingLoadChildrenResult.sendResult(Collections.<MediaItem>emptyList());
            mPendingRootHints = null;
            mPendingLoadChildrenResult = null;
        }
    }

    public void sendDelayedItemLoaded() {
        if (mPendingLoadItemResult != null) {
            mPendingLoadItemResult.sendResult(new MediaItem(new MediaDescription.Builder()
                    .setMediaId(MEDIA_ID_CHILDREN_DELAYED).setExtras(mPendingRootHints).build(),
                            MediaItem.FLAG_BROWSABLE));
            mPendingRootHints = null;
            mPendingLoadItemResult = null;
        }
    }
}
