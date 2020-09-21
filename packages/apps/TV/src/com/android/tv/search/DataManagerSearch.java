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

package com.android.tv.search;

import android.content.Context;
import android.content.Intent;
import android.media.tv.TvContentRating;
import android.media.tv.TvContract;
import android.media.tv.TvContract.Programs;
import android.media.tv.TvInputManager;
import android.os.SystemClock;
import android.support.annotation.MainThread;
import android.text.TextUtils;
import android.util.Log;
import com.android.tv.TvSingletons;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.data.Program;
import com.android.tv.data.ProgramDataManager;
import com.android.tv.data.api.Channel;
import com.android.tv.search.LocalSearchProvider.SearchResult;
import com.android.tv.util.MainThreadExecutor;
import com.android.tv.util.TvClock;
import com.android.tv.util.Utils;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;

/**
 * An implementation of {@link SearchInterface} to search query from {@link ChannelDataManager} and
 * {@link ProgramDataManager}.
 */
public class DataManagerSearch implements SearchInterface {
    private static final String TAG = "DataManagerSearch";
    private static final boolean DEBUG = false;

    private final Context mContext;
    private final TvInputManager mTvInputManager;
    private final ChannelDataManager mChannelDataManager;
    private final ProgramDataManager mProgramDataManager;

    //for TvClock
    private TvClock mClock;

    DataManagerSearch(Context context) {
        mContext = context;
        mTvInputManager = (TvInputManager) context.getSystemService(Context.TV_INPUT_SERVICE);
        TvSingletons tvSingletons = TvSingletons.getSingletons(context);
        mChannelDataManager = tvSingletons.getChannelDataManager();
        mProgramDataManager = tvSingletons.getProgramDataManager();
        mClock = new TvClock(context);
    }

    @Override
    public List<SearchResult> search(final String query, final int limit, final int action) {
        Future<List<SearchResult>> future =
                MainThreadExecutor.getInstance()
                        .submit(
                                new Callable<List<SearchResult>>() {
                                    @Override
                                    public List<SearchResult> call() throws Exception {
                                        return searchFromDataManagers(query, limit, action);
                                    }
                                });

        try {
            return future.get();
        } catch (InterruptedException e) {
            Thread.interrupted();
            return Collections.EMPTY_LIST;
        } catch (ExecutionException e) {
            Log.w(TAG, "Error searching for " + query, e);
            return Collections.EMPTY_LIST;
        }
    }

    @MainThread
    private List<SearchResult> searchFromDataManagers(String query, int limit, int action) {
        // TODO(b/72499165): add a test.
        List<SearchResult> results = new ArrayList<>();
        if (!mChannelDataManager.isDbLoadFinished()) {
            return results;
        }
        if (action == ACTION_TYPE_SWITCH_CHANNEL || action == ACTION_TYPE_SWITCH_INPUT) {
            // Voice search query should be handled by the a system TV app.
            return results;
        }
        if (DEBUG) Log.d(TAG, "Searching channels: '" + query + "'");
        long time = SystemClock.elapsedRealtime();
        Set<Long> channelsFound = new HashSet<>();
        List<Channel> channelList = mChannelDataManager.getBrowsableChannelList();
        query = query.toLowerCase();
        if (TextUtils.isDigitsOnly(query)) {
            for (Channel channel : channelList) {
                if (channelsFound.contains(channel.getId())) {
                    continue;
                }
                if (contains(channel.getDisplayNumber(), query)) {
                    addResult(results, channelsFound, channel, null);
                }
                if (results.size() >= limit) {
                    if (DEBUG) {
                        Log.d(
                                TAG,
                                "Found "
                                        + results.size()
                                        + " channels. Elapsed time for"
                                        + " searching channels: "
                                        + (SystemClock.elapsedRealtime() - time)
                                        + "(msec)");
                    }
                    return results;
                }
            }
            // TODO: recently watched channels may have higher priority.
        }
        for (Channel channel : channelList) {
            if (channelsFound.contains(channel.getId())) {
                continue;
            }
            if (contains(channel.getDisplayName(), query)
                    || contains(channel.getDescription(), query)) {
                addResult(results, channelsFound, channel, null);
            }
            if (results.size() >= limit) {
                if (DEBUG) {
                    Log.d(
                            TAG,
                            "Found "
                                    + results.size()
                                    + " channels. Elapsed time for"
                                    + " searching channels: "
                                    + (SystemClock.elapsedRealtime() - time)
                                    + "(msec)");
                }
                return results;
            }
        }
        if (DEBUG) {
            Log.d(
                    TAG,
                    "Found "
                            + results.size()
                            + " channels. Elapsed time for"
                            + " searching channels: "
                            + (SystemClock.elapsedRealtime() - time)
                            + "(msec)");
        }
        int channelResult = results.size();
        if (DEBUG) Log.d(TAG, "Searching programs: '" + query + "'");
        time = SystemClock.elapsedRealtime();
        for (Channel channel : channelList) {
            if (channelsFound.contains(channel.getId())) {
                continue;
            }
            Program program = mProgramDataManager.getCurrentProgram(channel.getId());
            if (program == null) {
                continue;
            }
            if (contains(program.getTitle(), query)
                    && !isRatingBlocked(program.getContentRatings())) {
                addResult(results, channelsFound, channel, program);
            }
            if (results.size() >= limit) {
                if (DEBUG) {
                    Log.d(
                            TAG,
                            "Found "
                                    + (results.size() - channelResult)
                                    + " programs. Elapsed"
                                    + " time for searching programs: "
                                    + (SystemClock.elapsedRealtime() - time)
                                    + "(msec)");
                }
                return results;
            }
        }
        for (Channel channel : channelList) {
            if (channelsFound.contains(channel.getId())) {
                continue;
            }
            Program program = mProgramDataManager.getCurrentProgram(channel.getId());
            if (program == null) {
                continue;
            }
            if (contains(program.getDescription(), query)
                    && !isRatingBlocked(program.getContentRatings())) {
                addResult(results, channelsFound, channel, program);
            }
            if (results.size() >= limit) {
                if (DEBUG) {
                    Log.d(
                            TAG,
                            "Found "
                                    + (results.size() - channelResult)
                                    + " programs. Elapsed"
                                    + " time for searching programs: "
                                    + (SystemClock.elapsedRealtime() - time)
                                    + "(msec)");
                }
                return results;
            }
        }
        if (DEBUG) {
            Log.d(
                    TAG,
                    "Found "
                            + (results.size() - channelResult)
                            + " programs. Elapsed time for"
                            + " searching programs: "
                            + (SystemClock.elapsedRealtime() - time)
                            + "(msec)");
        }
        return results;
    }

    // It assumes that query is already lower cases.
    private boolean contains(String string, String query) {
        return string != null && string.toLowerCase().contains(query);
    }

    /** If query is matched to channel, {@code program} should be null. */
    private void addResult(
            List<SearchResult> results, Set<Long> channelsFound, Channel channel, Program program) {
        if (program == null) {
            program = mProgramDataManager.getCurrentProgram(channel.getId());
            if (program != null && isRatingBlocked(program.getContentRatings())) {
                program = null;
            }
        }

        SearchResult.Builder result = SearchResult.builder();

        long channelId = channel.getId();
        result.setChannelId(channelId);
        result.setChannelNumber(channel.getDisplayNumber());
        if (program == null) {
            result.setTitle(channel.getDisplayName());
            result.setDescription(channel.getDescription());
            result.setImageUri(TvContract.buildChannelLogoUri(channelId).toString());
            result.setIntentAction(Intent.ACTION_VIEW);
            result.setIntentData(buildIntentData(channelId));
            result.setContentType(Programs.CONTENT_ITEM_TYPE);
            result.setIsLive(true);
            result.setProgressPercentage(LocalSearchProvider.PROGRESS_PERCENTAGE_HIDE);
        } else {
            result.setTitle(program.getTitle());
            result.setDescription(
                    buildProgramDescription(
                            channel.getDisplayNumber(),
                            channel.getDisplayName(),
                            program.getStartTimeUtcMillis(),
                            program.getEndTimeUtcMillis()));
            result.setImageUri(program.getPosterArtUri());
            result.setIntentAction(Intent.ACTION_VIEW);
            result.setIntentData(buildIntentData(channelId));
            result.setIntentExtraData(TvContract.buildProgramUri(program.getId()).toString());
            result.setContentType(Programs.CONTENT_ITEM_TYPE);
            result.setIsLive(true);
            result.setVideoWidth(program.getVideoWidth());
            result.setVideoHeight(program.getVideoHeight());
            result.setDuration(program.getDurationMillis());
            result.setProgressPercentage(
                    getProgressPercentage(
                            program.getStartTimeUtcMillis(), program.getEndTimeUtcMillis()));
        }
        if (DEBUG) {
            Log.d(TAG, "Add a result : channel=" + channel + " program=" + program);
        }
        results.add(result.build());
        channelsFound.add(channel.getId());
    }

    private String buildProgramDescription(
            String channelNumber,
            String channelName,
            long programStartUtcMillis,
            long programEndUtcMillis) {
        return Utils.getDurationString(mContext, programStartUtcMillis, programEndUtcMillis, false)
                + System.lineSeparator()
                + channelNumber
                + " "
                + channelName;
    }

    private int getProgressPercentage(long startUtcMillis, long endUtcMillis) {
        long current = mClock.currentTimeMillis();
        if (startUtcMillis > current || endUtcMillis <= current) {
            return LocalSearchProvider.PROGRESS_PERCENTAGE_HIDE;
        }
        return (int) (100 * (current - startUtcMillis) / (endUtcMillis - startUtcMillis));
    }

    private String buildIntentData(long channelId) {
        return TvContract.buildChannelUri(channelId).toString();
    }

    private boolean isRatingBlocked(TvContentRating[] ratings) {
        if (ratings == null
                || ratings.length == 0
                || !mTvInputManager.isParentalControlsEnabled()) {
            return false;
        }
        for (TvContentRating rating : ratings) {
            try {
                if (mTvInputManager.isRatingBlocked(rating)) {
                    return true;
                }
            } catch (IllegalArgumentException e) {
                // Do nothing.
            }
        }
        return false;
    }
}
