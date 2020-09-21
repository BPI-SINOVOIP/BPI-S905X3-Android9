/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.car;

import java.util.HashMap;
import java.util.Map;
import java.util.List;
import java.util.ArrayList;
import java.util.Arrays;
import com.android.internal.annotations.GuardedBy;
import android.util.Log;

public class VmsPublishersInfo {
    private static final String TAG = "VmsPublishersInfo";
    private static final boolean DBG = true;
    private final Object mLock = new Object();
    @GuardedBy("mLock")
    private final Map<InfoWrapper, Integer> mPublishersIds = new HashMap();
    @GuardedBy("mLock")
    private final Map<Integer, byte[]> mPublishersInfo = new HashMap();

    private static class InfoWrapper {
        private final byte[] mInfo;

        public InfoWrapper(byte[] info) {
            mInfo = info;
        }

        public byte[] getInfo() {
            return mInfo;
        }

        @Override
        public boolean equals(Object o) {
            if (!(o instanceof InfoWrapper)) {
                return false;
            }
            InfoWrapper p = (InfoWrapper) o;
            return Arrays.equals(this.mInfo, p.mInfo);
        }

        @Override
        public int hashCode() {
            return Arrays.hashCode(mInfo);
        }
    }

    /**
     * Returns the ID associated with the publisher info. When called for the first time for a
     * publisher info will store the info and assign an ID
     */
    public int getIdForInfo(byte[] publisherInfo) {
        Integer publisherId;
        InfoWrapper wrappedPublisherInfo = new InfoWrapper(publisherInfo);
        synchronized (mLock) {
            maybeAddPublisherInfoLocked(wrappedPublisherInfo);
            publisherId = mPublishersIds.get(wrappedPublisherInfo);
        }
        if (DBG) {
            Log.i(TAG, "Publisher ID is: " + publisherId);
        }
        return publisherId;
    }

    public byte[] getPublisherInfo(int publisherId) {
        synchronized (mLock) {
            return mPublishersInfo.get(publisherId).clone();
        }
    }

    @GuardedBy("mLock")
    private void maybeAddPublisherInfoLocked(InfoWrapper wrappedPublisherInfo) {
        if (!mPublishersIds.containsKey(wrappedPublisherInfo)) {
            // Assign ID to the info
            Integer publisherId = mPublishersIds.size();

            mPublishersIds.put(wrappedPublisherInfo, publisherId);
            mPublishersInfo.put(publisherId, wrappedPublisherInfo.getInfo());
        }
    }
}

