/*
 * Copyright 2015 Google Inc. All rights reserved.
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
 *
 * Modifications copyright (C) 2018 DTVKit
 */

package org.dtvkit.companionlibrary.model;

public final class EventPeriod {
    private String mDvbUri;
    private long mStartUtc;
    private long mEndUtc;

    public EventPeriod(String dvbUri, long startUtc, long endUtc)
    {
        mDvbUri = dvbUri;
        mStartUtc = startUtc;
        mEndUtc = endUtc;
    }

    public String getDvbUri() {
        return mDvbUri;
    }
    public long getStartUtc() {
        return mStartUtc;
    }
    public long getEndUtc() {
        return mEndUtc;
    }
}
