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

import android.content.Context;
import android.content.pm.ResolveInfo;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * This class provides a way to connect to multiple browsable players at a time.
 * It will attempt to simultaneously connect to a list of services that support
 * the MediaBrowserService. After a timeout, the list of connected players will
 * be returned via callback.
 *
 * The main use of this class is to check whether a player can be browsed despite
 * using the MediaBrowserService. This way we do not have to do the same checks
 * when constructing BrowsedPlayerWrappers by hand.
 */
public class BrowsablePlayerConnector {
    private static final String TAG = "NewAvrcpBrowsablePlayerConnector";
    private static final boolean DEBUG = true;
    private static final long CONNECT_TIMEOUT_MS = 10000; // Time in ms to wait for a connection

    private static final int MSG_GET_FOLDER_ITEMS_CB = 0;
    private static final int MSG_CONNECT_CB = 1;
    private static final int MSG_TIMEOUT = 2;

    private Handler mHandler;
    private Context mContext;
    private PlayerListCallback mCallback;

    private List<BrowsedPlayerWrapper> mResults = new ArrayList<BrowsedPlayerWrapper>();
    private Set<BrowsedPlayerWrapper> mPendingPlayers = new HashSet<BrowsedPlayerWrapper>();

    interface PlayerListCallback {
        void run(List<BrowsedPlayerWrapper> result);
    }

    static BrowsablePlayerConnector connectToPlayers(
            Context context,
            Looper looper,
            List<ResolveInfo> players,
            PlayerListCallback cb) {
        if (cb == null) {
            Log.wtfStack(TAG, "Null callback passed");
            return null;
        }

        BrowsablePlayerConnector newWrapper = new BrowsablePlayerConnector(context, looper, cb);

        // Try to start connecting all the browsed player wrappers
        for (ResolveInfo info : players) {
            BrowsedPlayerWrapper player = BrowsedPlayerWrapper.wrap(
                            context,
                            info.serviceInfo.packageName,
                            info.serviceInfo.name);
            newWrapper.mPendingPlayers.add(player);
            player.connect((int status, BrowsedPlayerWrapper wrapper) -> {
                // Use the handler to avoid concurrency issues
                if (DEBUG) {
                    Log.d(TAG, "Browse player callback called: package="
                            + info.serviceInfo.packageName
                            + " : status=" + status);
                }
                Message msg = newWrapper.mHandler.obtainMessage(MSG_CONNECT_CB);
                msg.arg1 = status;
                msg.obj = wrapper;
                newWrapper.mHandler.sendMessage(msg);
            });
        }

        Message msg = newWrapper.mHandler.obtainMessage(MSG_TIMEOUT);
        newWrapper.mHandler.sendMessageDelayed(msg, CONNECT_TIMEOUT_MS);
        return newWrapper;
    }

    private BrowsablePlayerConnector(Context context, Looper looper, PlayerListCallback cb) {
        mContext = context;
        mCallback = cb;
        mHandler = new Handler(looper) {
            public void handleMessage(Message msg) {
                if (DEBUG) Log.d(TAG, "Received a message: msg.what=" + msg.what);
                switch(msg.what) {
                    case MSG_GET_FOLDER_ITEMS_CB: {
                        BrowsedPlayerWrapper wrapper = (BrowsedPlayerWrapper) msg.obj;
                        // If we failed to remove the wrapper from the pending set, that
                        // means a timeout occurred and the callback was triggered afterwards
                        if (!mPendingPlayers.remove(wrapper)) {
                            return;
                        }

                        Log.i(TAG, "Successfully added package to results: "
                                + wrapper.getPackageName());
                        mResults.add(wrapper);
                    } break;

                    case MSG_CONNECT_CB: {
                        BrowsedPlayerWrapper wrapper = (BrowsedPlayerWrapper) msg.obj;

                        if (msg.arg1 != BrowsedPlayerWrapper.STATUS_SUCCESS) {
                            Log.i(TAG, wrapper.getPackageName() + " is not browsable");
                            mPendingPlayers.remove(wrapper);
                            return;
                        }

                        // Check to see if the root folder has any items
                        if (DEBUG) {
                            Log.i(TAG, "Checking root contents for " + wrapper.getPackageName());
                        }
                        wrapper.getFolderItems(wrapper.getRootId(),
                                (int status, String mediaId, List<ListItem> results) -> {
                                    if (status != BrowsedPlayerWrapper.STATUS_SUCCESS) {
                                        mPendingPlayers.remove(wrapper);
                                        return;
                                    }

                                    if (results.size() == 0) {
                                        mPendingPlayers.remove(wrapper);
                                        return;
                                    }

                                    // Send the response as a message so that it is properly
                                    // synchronized
                                    Message success =
                                            mHandler.obtainMessage(MSG_GET_FOLDER_ITEMS_CB);
                                    success.obj = wrapper;
                                    mHandler.sendMessage(success);
                                });
                    } break;

                    case MSG_TIMEOUT: {
                        Log.v(TAG, "Timed out waiting for players");
                        for (BrowsedPlayerWrapper wrapper : mPendingPlayers) {
                            if (DEBUG) Log.d(TAG, "Disconnecting " + wrapper.getPackageName());
                            wrapper.disconnect();
                        }
                        mPendingPlayers.clear();
                    } break;
                }

                if (mPendingPlayers.size() == 0) {
                    Log.i(TAG, "Successfully connected to "
                            + mResults.size() + " browsable players.");
                    removeMessages(MSG_TIMEOUT);
                    mCallback.run(mResults);
                }
            }
        };
    }
}
