/*
 * Copyright (C) 2018 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.tv.testing.testdata;

import android.content.Context;
import android.media.tv.TvInputInfo;
import com.android.tv.common.util.Clock;
import com.android.tv.testing.data.ChannelInfo;
import com.android.tv.testing.data.ChannelUtils;
import com.android.tv.testing.data.ProgramUtils;
import com.android.tv.testing.utils.TestUtils;
import java.util.List;

/**
 * A set of test data.
 *
 * <p>contains:
 *
 * <ul>
 *   <li>InputID
 *   <li>Channel List
 * </ul>
 *
 * Call {@link #init(Context)}, to update the TvProvider data base with the given values.
 */
public abstract class TestData {
    private List<ChannelInfo> channelList;

    protected abstract List<ChannelInfo> createChannels(Context context);

    public void init(Context context, Clock clock, long durationMs) {
        channelList = createChannels(context);
        ChannelUtils.updateChannels(context, getInputId(), channelList);
        ProgramUtils.updateProgramForAllChannelsOf(context, getInputId(), clock, durationMs);
    }

    public abstract TvInputInfo getTvInputInfo();

    public final String getInputId() {
        return getTvInputInfo().getId();
    }

    public static final TestData DEFAULT_10_CHANNELS =
            new TestData() {
                private TvInputInfo mTvInputInfo = createTvInputInfo();

                private TvInputInfo createTvInputInfo() {
                    try {
                        return TestUtils.createTvInputInfo(
                                TestUtils.createResolveInfo(
                                        "com.android.tv.testing.testdata",
                                        "com.android.tv.testing.testdata.Default10Channels"),
                                "com.android.tv.testing.testdata/.Default10Channels",
                                null,
                                TvInputInfo.TYPE_TUNER,
                                true);
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                }

                @Override
                protected List<ChannelInfo> createChannels(Context context) {
                    return ChannelUtils.createChannelInfos(context, 10);
                }

                @Override
                public TvInputInfo getTvInputInfo() {
                    return mTvInputInfo;
                }
            };
}
