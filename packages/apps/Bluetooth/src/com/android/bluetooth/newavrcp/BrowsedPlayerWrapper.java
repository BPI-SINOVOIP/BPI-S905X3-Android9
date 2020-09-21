/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.bluetooth.avrcp;

import android.content.ComponentName;
import android.content.Context;
import android.media.browse.MediaBrowser.MediaItem;
import android.util.Log;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/*
 * Helper class to create an abstraction layer for the MediaBrowser service that AVRCP can use.
 *
 * TODO (apanicke): Add timeouts in case a browser takes forever to connect or gets stuck.
 * Right now this is ok because the BrowsablePlayerConnector will handle timeouts.
 */
class BrowsedPlayerWrapper {
    private static final String TAG = "NewAvrcpBrowsedPlayerWrapper";
    private static final boolean DEBUG = true;

    enum ConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
    }

    interface ConnectionCallback {
        void run(int status, BrowsedPlayerWrapper wrapper);
    }

    interface BrowseCallback {
        void run(int status, String mediaId, List<ListItem> results);
    }

    public static final int STATUS_SUCCESS = 0;
    public static final int STATUS_CONN_ERROR = 1;
    public static final int STATUS_LOOKUP_ERROR = 2;

    private MediaBrowser mWrappedBrowser;

    // TODO (apanicke): Store the context in the factories so that we don't need to save this.
    // As long as the service is alive those factories will have a valid context.
    private Context mContext;
    private String mPackageName;
    private ConnectionCallback mCallback;

    // TODO(apanicke): We cache this because normally you can only grab the root
    // while connected. We shouldn't cache this since theres nothing in the framework documentation
    // that says this can't change between connections. Instead always treat empty string as root.
    private String mRoot = "";

    // A linked hash map that keeps the contents of the last X browsed folders.
    //
    // NOTE: This is needed since some carkits will repeatedly request each item in a folder
    // individually, incrementing the index of the requested item by one at a time. Going through
    // the subscription process for each individual item is incredibly slow so we cache the items
    // in the folder in order to speed up the process. We still run the risk of one device pushing
    // out a cached folder that another device was using, but this is highly unlikely since for
    // this to happen you would need to be connected to two carkits at the same time.
    //
    // TODO (apanicke): Dynamically set the number of cached folders equal to the max number
    // of connected devices because that is the maximum number of folders that can be browsed at
    // a single time.
    static final int NUM_CACHED_FOLDERS = 5;
    LinkedHashMap<String, List<ListItem>> mCachedFolders =
            new LinkedHashMap<String, List<ListItem>>(NUM_CACHED_FOLDERS) {
                @Override
                protected boolean removeEldestEntry(Map.Entry<String, List<ListItem>> eldest) {
                    return size() > NUM_CACHED_FOLDERS;
                }
            };

    // TODO (apanicke): Investigate if there is a way to create this just by passing in the
    // MediaBrowser. Right now there is no obvious way to create the browser then update the
    // connection callback without being forced to re-create the object every time.
    private BrowsedPlayerWrapper(Context context, String packageName, String className) {
        mContext = context;
        mPackageName = packageName;

        mWrappedBrowser = MediaBrowserFactory.make(
                context,
                new ComponentName(packageName, className),
                new MediaConnectionCallback(),
                null);
    }

    static BrowsedPlayerWrapper wrap(Context context, String packageName, String className) {
        Log.i(TAG, "Wrapping Media Browser " + packageName);
        BrowsedPlayerWrapper wrapper =
                new BrowsedPlayerWrapper(context, packageName, className);
        return wrapper;
    }

    void connect(ConnectionCallback cb) {
        if (cb == null) {
            Log.wtfStack(TAG, "connect: Trying to connect to " + mPackageName
                    + "with null callback");
        }
        if (mCallback != null) {
            Log.w(TAG, "connect: Already trying to connect to " + mPackageName);
            return;
        }

        if (DEBUG) Log.d(TAG, "connect: Connecting to browsable player: " + mPackageName);
        mCallback = (int status, BrowsedPlayerWrapper wrapper) -> {
            cb.run(status, wrapper);
            wrapper.disconnect();
        };
        mWrappedBrowser.connect();
    }

    void disconnect() {
        if (DEBUG) Log.d(TAG, "disconnect: Disconnecting from " + mPackageName);
        mWrappedBrowser.disconnect();
        mCallback = null;
    }

    public String getPackageName() {
        return mPackageName;
    }

    public String getRootId() {
        return mRoot;
    }

    public void playItem(String mediaId) {
        if (DEBUG) Log.d(TAG, "playItem: Play Item from media ID: " + mediaId);
        connect((int status, BrowsedPlayerWrapper wrapper) -> {
            if (DEBUG) Log.d(TAG, "playItem: Connected to browsable player " + mPackageName);

            MediaController controller = MediaControllerFactory.make(mContext,
                    wrapper.mWrappedBrowser.getSessionToken());
            MediaController.TransportControls ctrl = controller.getTransportControls();
            Log.i(TAG, "playItem: Playing " + mediaId);
            ctrl.playFromMediaId(mediaId, null);
        });
        return;
    }

    // Returns false if the player is in the connecting state. Wait for it to either be
    // connected or disconnected.
    //
    // TODO (apanicke): Determine what happens when we subscribe to the same item while a
    // callback is in flight.
    //
    // TODO (apanicke): Currently we do a full folder lookup even if the remote device requests
    // info for only one item. Add a lookup function that can handle getting info for a single
    // item.
    public boolean getFolderItems(String mediaId, BrowseCallback cb) {
        if (mCachedFolders.containsKey(mediaId)) {
            Log.i(TAG, "getFolderItems: Grabbing cached data for mediaId: " + mediaId);
            cb.run(STATUS_SUCCESS, mediaId, Util.cloneList(mCachedFolders.get(mediaId)));
            return true;
        }

        if (cb == null) {
            Log.wtfStack(TAG, "connect: Trying to connect to " + mPackageName
                    + "with null callback");
        }
        if (mCallback != null) {
            Log.w(TAG, "connect: Already trying to connect to " + mPackageName);
            return false;
        }

        if (DEBUG) Log.d(TAG, "connect: Connecting to browsable player: " + mPackageName);
        mCallback = (int status, BrowsedPlayerWrapper wrapper) -> {
            Log.i(TAG, "getFolderItems: Connected to browsable player: " + mPackageName);
            if (status != STATUS_SUCCESS) {
                cb.run(status, "", new ArrayList<ListItem>());
            }

            // This will disconnect when the callback is called
            getFolderItemsInternal(mediaId, cb);
        };
        mWrappedBrowser.connect();

        return true;
    }

    // Internal function to call once the Browser is connected
    private boolean getFolderItemsInternal(String mediaId, BrowseCallback cb) {
        mWrappedBrowser.subscribe(mediaId, new BrowserSubscriptionCallback(cb));
        return true;
    }

    class MediaConnectionCallback extends MediaBrowser.ConnectionCallback {
        @Override
        public void onConnected() {
            Log.i(TAG, "onConnected: " + mPackageName + " is connected");
            // Get the root while connected because we may need to use it when disconnected.
            mRoot = mWrappedBrowser.getRoot();

            if (mCallback == null) return;

            if (mRoot == null || mRoot.isEmpty()) {
                mCallback.run(STATUS_CONN_ERROR, BrowsedPlayerWrapper.this);
                return;
            }

            mCallback.run(STATUS_SUCCESS, BrowsedPlayerWrapper.this);
            mCallback = null;
        }


        @Override
        public void onConnectionFailed() {
            Log.w(TAG, "onConnectionFailed: Connection Failed with " + mPackageName);
            if (mCallback != null) mCallback.run(STATUS_CONN_ERROR, BrowsedPlayerWrapper.this);
            mCallback = null;
        }

        // TODO (apanicke): Add a check to list a player as unbrowsable if it suspends immediately
        // after connection.
        @Override
        public void onConnectionSuspended() {
            mWrappedBrowser.disconnect();
            Log.i(TAG, "onConnectionSuspended: Connection Suspended with " + mPackageName);
        }
    }

    /**
     * Subscription callback handler. Subscribe to a folder to get its contents. We generate a new
     * instance for this class for each subscribe call to make it easier to differentiate between
     * the callers.
     */
    private class BrowserSubscriptionCallback extends MediaBrowser.SubscriptionCallback {
        BrowseCallback mCallback = null;

        BrowserSubscriptionCallback(BrowseCallback cb) {
            mCallback = cb;
        }

        @Override
        public void onChildrenLoaded(String parentId, List<MediaItem> children) {
            if (DEBUG) {
                Log.d(TAG, "onChildrenLoaded: mediaId=" + parentId + " size= " + children.size());
            }

            if (mCallback == null) {
                Log.w(TAG, "onChildrenLoaded: " + mPackageName
                        + " children loaded while callback is null");
            }

            // TODO (apanicke): Instead of always unsubscribing, only unsubscribe from folders
            // that aren't cached. This will let us update what is cached on the fly and prevent
            // us from serving stale data.
            mWrappedBrowser.unsubscribe(parentId);

            ArrayList<ListItem> return_list = new ArrayList<ListItem>();

            for (MediaItem item : children) {
                if (DEBUG) {
                    Log.d(TAG, "onChildrenLoaded: Child=\"" + item.toString()
                            + "\",  ID=\"" + item.getMediaId() + "\"");
                }

                if (item.isBrowsable()) {
                    CharSequence titleCharSequence = item.getDescription().getTitle();
                    String title = "Not Provided";
                    if (titleCharSequence != null) {
                        title = titleCharSequence.toString();
                    }
                    Folder f = new Folder(item.getMediaId(), false, title);
                    return_list.add(new ListItem(f));
                } else {
                    return_list.add(new ListItem(Util.toMetadata(item)));
                }
            }

            mCachedFolders.put(parentId, return_list);

            // Clone the list so that the callee can mutate it without affecting the cached data
            mCallback.run(STATUS_SUCCESS, parentId, Util.cloneList(return_list));
            mCallback = null;
            disconnect();
        }

        /* mediaId is invalid */
        @Override
        public void onError(String id) {
            Log.e(TAG, "BrowserSubscriptionCallback: Could not get folder items");
            mCallback.run(STATUS_LOOKUP_ERROR, id, new ArrayList<ListItem>());
            disconnect();
        }
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("Browsable Package Name: " + mPackageName + "\n");
        sb.append("   Cached Media ID's: ");
        for (String id : mCachedFolders.keySet()) {
            sb.append("\"" + id + "\", ");
        }
        sb.append("\n");
        return sb.toString();
    }
}
