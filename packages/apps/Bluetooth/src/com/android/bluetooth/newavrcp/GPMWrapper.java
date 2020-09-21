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

import android.media.session.MediaSession;
import android.util.Log;

/**
 * Google Play Music hides some of the metadata behind a specific key in the Extras of the
 * MediaDescription in the MediaSession.QueueItem. This class exists to provide alternate
 * methods to allow Google Play Music to match the default behaviour of MediaPlayerWrapper.
 */
class GPMWrapper extends MediaPlayerWrapper {
    private static final String TAG = "NewAvrcpGPMWrapper";
    private static final boolean DEBUG = true;

    @Override
    boolean isMetadataSynced() {
        if (getQueue() == null) {
            return false;
        }

        // Check if currentPlayingQueueId is in the queue
        MediaSession.QueueItem currItem = null;
        for (MediaSession.QueueItem item : getQueue()) {
            // The item exists in the current queue
            if (item.getQueueId() == getActiveQueueID()) {
                currItem = item;
                break;
            }
        }

        // Check if current playing song in Queue matches current Metadata
        Metadata qitem = Util.toMetadata(currItem);
        Metadata mdata = Util.toMetadata(getMetadata());
        if (currItem == null || !qitem.equals(mdata)) {
            if (DEBUG) {
                Log.d(TAG, "Metadata currently out of sync for Google Play Music");
                Log.d(TAG, "  └ Current queueItem: " + qitem);
                Log.d(TAG, "  └ Current metadata : " + mdata);
            }
            return false;
        }

        return true;
    }
}
