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

package com.android.tv.recommendation;

import static android.support.test.InstrumentationRegistry.getContext;
import static com.google.common.truth.Truth.assertThat;

import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.test.MoreAsserts;
import com.android.tv.data.api.Channel;
import com.android.tv.recommendation.RecommendationUtils.ChannelRecordSortedMapHelper;
import com.android.tv.testing.utils.Utils;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class RecommenderTest {
    private static final int DEFAULT_NUMBER_OF_CHANNELS = 5;
    private static final long DEFAULT_WATCH_START_TIME_MS =
            System.currentTimeMillis() - TimeUnit.DAYS.toMillis(2);
    private static final long DEFAULT_WATCH_END_TIME_MS =
            System.currentTimeMillis() - TimeUnit.DAYS.toMillis(1);
    private static final long DEFAULT_MAX_WATCH_DURATION_MS = TimeUnit.HOURS.toMillis(1);

    private final Comparator<Channel> mChannelSortKeyComparator =
            new Comparator<Channel>() {
                @Override
                public int compare(Channel lhs, Channel rhs) {
                    return mRecommender
                            .getChannelSortKey(lhs.getId())
                            .compareTo(mRecommender.getChannelSortKey(rhs.getId()));
                }
            };
    private final Runnable mStartDatamanagerRunnableAddFourChannels =
            new Runnable() {
                @Override
                public void run() {
                    // Add 4 channels in ChannelRecordMap for testing. Store the added channels to
                    // mChannels_1 ~ mChannels_4. They are sorted by channel id in increasing order.
                    mChannel_1 = mChannelRecordSortedMap.addChannel();
                    mChannel_2 = mChannelRecordSortedMap.addChannel();
                    mChannel_3 = mChannelRecordSortedMap.addChannel();
                    mChannel_4 = mChannelRecordSortedMap.addChannel();
                }
            };

    private RecommendationDataManager mDataManager;
    private Recommender mRecommender;
    private FakeEvaluator mEvaluator;
    private ChannelRecordSortedMapHelper mChannelRecordSortedMap;
    private boolean mOnRecommenderReady;
    private boolean mOnRecommendationChanged;
    private Channel mChannel_1;
    private Channel mChannel_2;
    private Channel mChannel_3;
    private Channel mChannel_4;

    @Before
    public void setUp() {
        mChannelRecordSortedMap = new ChannelRecordSortedMapHelper(getContext());
        mDataManager =
                RecommendationUtils.createMockRecommendationDataManager(mChannelRecordSortedMap);
        mChannelRecordSortedMap.resetRandom(Utils.createTestRandom());
    }

    @Test
    public void testRecommendChannels_includeRecommendedOnly_allChannelsHaveNoScore() {
        createRecommender(true, mStartDatamanagerRunnableAddFourChannels);

    // Recommender doesn't recommend any channels because all channels are not recommended.
    assertThat(mRecommender.recommendChannels()).isEmpty();
    assertThat(mRecommender.recommendChannels(-5)).isEmpty();
    assertThat(mRecommender.recommendChannels(0)).isEmpty();
    assertThat(mRecommender.recommendChannels(3)).isEmpty();
    assertThat(mRecommender.recommendChannels(4)).isEmpty();
    assertThat(mRecommender.recommendChannels(5)).isEmpty();
    }

    @Test
    public void testRecommendChannels_notIncludeRecommendedOnly_allChannelsHaveNoScore() {
        createRecommender(false, mStartDatamanagerRunnableAddFourChannels);

    // Recommender recommends every channel because it recommends not-recommended channels too.
    assertThat(mRecommender.recommendChannels()).hasSize(4);
    assertThat(mRecommender.recommendChannels(-5)).isEmpty();
    assertThat(mRecommender.recommendChannels(0)).isEmpty();
    assertThat(mRecommender.recommendChannels(3)).hasSize(3);
    assertThat(mRecommender.recommendChannels(4)).hasSize(4);
    assertThat(mRecommender.recommendChannels(5)).hasSize(4);
    }

    @Test
    public void testRecommendChannels_includeRecommendedOnly_allChannelsHaveScore() {
        createRecommender(true, mStartDatamanagerRunnableAddFourChannels);

        setChannelScores_scoreIncreasesAsChannelIdIncreases();

        // recommendChannels must be sorted by score in decreasing order.
        // (i.e. sorted by channel ID in decreasing order in this case)
        MoreAsserts.assertContentsInOrder(
                mRecommender.recommendChannels(), mChannel_4, mChannel_3, mChannel_2, mChannel_1);
    assertThat(mRecommender.recommendChannels(-5)).isEmpty();
    assertThat(mRecommender.recommendChannels(0)).isEmpty();
        MoreAsserts.assertContentsInOrder(
                mRecommender.recommendChannels(3), mChannel_4, mChannel_3, mChannel_2);
        MoreAsserts.assertContentsInOrder(
                mRecommender.recommendChannels(4), mChannel_4, mChannel_3, mChannel_2, mChannel_1);
        MoreAsserts.assertContentsInOrder(
                mRecommender.recommendChannels(5), mChannel_4, mChannel_3, mChannel_2, mChannel_1);
    }

    @Test
    public void testRecommendChannels_notIncludeRecommendedOnly_allChannelsHaveScore() {
        createRecommender(false, mStartDatamanagerRunnableAddFourChannels);

        setChannelScores_scoreIncreasesAsChannelIdIncreases();

        // recommendChannels must be sorted by score in decreasing order.
        // (i.e. sorted by channel ID in decreasing order in this case)
        MoreAsserts.assertContentsInOrder(
                mRecommender.recommendChannels(), mChannel_4, mChannel_3, mChannel_2, mChannel_1);
    assertThat(mRecommender.recommendChannels(-5)).isEmpty();
    assertThat(mRecommender.recommendChannels(0)).isEmpty();
        MoreAsserts.assertContentsInOrder(
                mRecommender.recommendChannels(3), mChannel_4, mChannel_3, mChannel_2);
        MoreAsserts.assertContentsInOrder(
                mRecommender.recommendChannels(4), mChannel_4, mChannel_3, mChannel_2, mChannel_1);
        MoreAsserts.assertContentsInOrder(
                mRecommender.recommendChannels(5), mChannel_4, mChannel_3, mChannel_2, mChannel_1);
    }

    @Test
    public void testRecommendChannels_includeRecommendedOnly_fewChannelsHaveScore() {
        createRecommender(true, mStartDatamanagerRunnableAddFourChannels);

        mEvaluator.setChannelScore(mChannel_1.getId(), 1.0);
        mEvaluator.setChannelScore(mChannel_2.getId(), 1.0);

        // Only two channels are recommended because recommender doesn't recommend other channels.
        MoreAsserts.assertContentsInAnyOrder(
                mRecommender.recommendChannels(), mChannel_1, mChannel_2);
    assertThat(mRecommender.recommendChannels(-5)).isEmpty();
    assertThat(mRecommender.recommendChannels(0)).isEmpty();
        MoreAsserts.assertContentsInAnyOrder(
                mRecommender.recommendChannels(3), mChannel_1, mChannel_2);
        MoreAsserts.assertContentsInAnyOrder(
                mRecommender.recommendChannels(4), mChannel_1, mChannel_2);
        MoreAsserts.assertContentsInAnyOrder(
                mRecommender.recommendChannels(5), mChannel_1, mChannel_2);
    }

    @Test
    public void testRecommendChannels_notIncludeRecommendedOnly_fewChannelsHaveScore() {
        createRecommender(false, mStartDatamanagerRunnableAddFourChannels);

        mEvaluator.setChannelScore(mChannel_1.getId(), 1.0);
        mEvaluator.setChannelScore(mChannel_2.getId(), 1.0);

    assertThat(mRecommender.recommendChannels()).hasSize(4);
        MoreAsserts.assertContentsInAnyOrder(
                mRecommender.recommendChannels().subList(0, 2), mChannel_1, mChannel_2);

    assertThat(mRecommender.recommendChannels(-5)).isEmpty();
    assertThat(mRecommender.recommendChannels(0)).isEmpty();

    assertThat(mRecommender.recommendChannels(3)).hasSize(3);
        MoreAsserts.assertContentsInAnyOrder(
                mRecommender.recommendChannels(3).subList(0, 2), mChannel_1, mChannel_2);

    assertThat(mRecommender.recommendChannels(4)).hasSize(4);
        MoreAsserts.assertContentsInAnyOrder(
                mRecommender.recommendChannels(4).subList(0, 2), mChannel_1, mChannel_2);

    assertThat(mRecommender.recommendChannels(5)).hasSize(4);
        MoreAsserts.assertContentsInAnyOrder(
                mRecommender.recommendChannels(5).subList(0, 2), mChannel_1, mChannel_2);
    }

    @Test
    public void testGetChannelSortKey_recommendAllChannels() {
        createRecommender(true, mStartDatamanagerRunnableAddFourChannels);

        setChannelScores_scoreIncreasesAsChannelIdIncreases();

        List<Channel> expectedChannelList = mRecommender.recommendChannels();
        List<Channel> channelList = Arrays.asList(mChannel_1, mChannel_2, mChannel_3, mChannel_4);
        Collections.sort(channelList, mChannelSortKeyComparator);

        // Recommended channel list and channel list sorted by sort key must be the same.
        MoreAsserts.assertContentsInOrder(channelList, expectedChannelList.toArray());
        assertSortKeyNotInvalid(channelList);
    }

    @Test
    public void testGetChannelSortKey_recommendFewChannels() {
        // Test with recommending 3 channels.
        createRecommender(true, mStartDatamanagerRunnableAddFourChannels);

        setChannelScores_scoreIncreasesAsChannelIdIncreases();

        List<Channel> expectedChannelList = mRecommender.recommendChannels(3);
    // A channel which is not recommended by the recommender has to get an invalid sort key.
    assertThat(mRecommender.getChannelSortKey(mChannel_1.getId()))
        .isEqualTo(Recommender.INVALID_CHANNEL_SORT_KEY);

        List<Channel> channelList = Arrays.asList(mChannel_2, mChannel_3, mChannel_4);
        Collections.sort(channelList, mChannelSortKeyComparator);

        MoreAsserts.assertContentsInOrder(channelList, expectedChannelList.toArray());
        assertSortKeyNotInvalid(channelList);
    }

    @Test
    public void testListener_onRecommendationChanged() {
        createRecommender(true, mStartDatamanagerRunnableAddFourChannels);
    // FakeEvaluator doesn't recommend a channel with empty watch log. As every channel
    // doesn't have a watch log, nothing is recommended and recommendation isn't changed.
    assertThat(mOnRecommendationChanged).isFalse();

        // Set lastRecommendationUpdatedTimeUtcMs to check recommendation changed because,
        // recommender has a minimum recommendation update period.
        mRecommender.setLastRecommendationUpdatedTimeUtcMs(
                System.currentTimeMillis() - TimeUnit.MINUTES.toMillis(10));
        long latestWatchEndTimeMs = DEFAULT_WATCH_START_TIME_MS;
        for (long channelId : mChannelRecordSortedMap.keySet()) {
            mEvaluator.setChannelScore(channelId, 1.0);
      // Add a log to recalculate the recommendation score.
      assertThat(
              mChannelRecordSortedMap.addWatchLog(
                  channelId, latestWatchEndTimeMs, TimeUnit.MINUTES.toMillis(10)))
          .isTrue();
            latestWatchEndTimeMs += TimeUnit.MINUTES.toMillis(10);
        }

    // onRecommendationChanged must be called, because recommend channels are not empty,
    // by setting score to each channel.
    assertThat(mOnRecommendationChanged).isTrue();
    }

    @Test
    public void testListener_onRecommenderReady() {
        createRecommender(
                true,
                new Runnable() {
                    @Override
                    public void run() {
                        mChannelRecordSortedMap.addChannels(DEFAULT_NUMBER_OF_CHANNELS);
                        mChannelRecordSortedMap.addRandomWatchLogs(
                                DEFAULT_WATCH_START_TIME_MS,
                                DEFAULT_WATCH_END_TIME_MS,
                                DEFAULT_MAX_WATCH_DURATION_MS);
                    }
                });

    // After loading channels and watch logs are finished, recommender must be available to use.
    assertThat(mOnRecommenderReady).isTrue();
    }

    private void assertSortKeyNotInvalid(List<Channel> channelList) {
        for (Channel channel : channelList) {
            MoreAsserts.assertNotEqual(
                    Recommender.INVALID_CHANNEL_SORT_KEY,
                    mRecommender.getChannelSortKey(channel.getId()));
        }
    }

    private void createRecommender(
            boolean includeRecommendedOnly, Runnable startDataManagerRunnable) {
        mRecommender =
                new Recommender(
                        new Recommender.Listener() {
                            @Override
                            public void onRecommenderReady() {
                                mOnRecommenderReady = true;
                            }

                            @Override
                            public void onRecommendationChanged() {
                                mOnRecommendationChanged = true;
                            }
                        },
                        includeRecommendedOnly,
                        mDataManager);

        mEvaluator = new FakeEvaluator();
        mRecommender.registerEvaluator(mEvaluator);
        mChannelRecordSortedMap.setRecommender(mRecommender);

        // When mRecommender is instantiated, its dataManager will be started, and load channels
        // and watch history data if it is not started.
        if (startDataManagerRunnable != null) {
            startDataManagerRunnable.run();
            mRecommender.onChannelRecordChanged();
        }
        // After loading channels and watch history data are finished,
        // RecommendationDataManager calls listener.onChannelRecordLoaded()
        // which will be mRecommender.onChannelRecordLoaded().
        mRecommender.onChannelRecordLoaded();
    }

    private List<Long> getChannelIdListSorted() {
        return new ArrayList<>(mChannelRecordSortedMap.keySet());
    }

    private void setChannelScores_scoreIncreasesAsChannelIdIncreases() {
        List<Long> channelIdList = getChannelIdListSorted();
        double score = Math.pow(0.5, channelIdList.size());
        for (long channelId : channelIdList) {
            // Channel with smaller id has smaller score than channel with higher id.
            mEvaluator.setChannelScore(channelId, score);
            score *= 2.0;
        }
    }

    private class FakeEvaluator extends Recommender.Evaluator {
        private final Map<Long, Double> mChannelScore = new HashMap<>();

        @Override
        public double evaluateChannel(long channelId) {
            if (getRecommender().getChannelRecord(channelId) == null) {
                return NOT_RECOMMENDED;
            }
            Double score = mChannelScore.get(channelId);
            return score == null ? NOT_RECOMMENDED : score;
        }

        public void setChannelScore(long channelId, double score) {
            mChannelScore.put(channelId, score);
        }
    }
}
