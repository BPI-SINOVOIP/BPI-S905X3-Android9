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

package com.android.tv;

import android.content.Intent;
import android.media.tv.TvContract;
import android.media.tv.TvInputInfo;
import android.net.Uri;
import android.os.Handler;
import android.provider.Settings;
import android.support.annotation.MainThread;
import android.support.annotation.Nullable;
import android.util.ArraySet;
import android.util.Log;

import android.content.Context;

import com.android.tv.common.SoftPreconditions;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.data.api.Channel;
import com.android.tv.util.TvInputManagerHelper;
import com.android.tv.util.Utils;
import com.android.tv.common.util.SystemProperties;
import com.android.tv.data.ChannelNumber;
import com.android.tv.common.util.SystemProperties;

import com.droidlogic.app.tv.DroidLogicTvUtils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Comparator;
import java.util.concurrent.locks.ReentrantReadWriteLock;

import android.provider.Settings;

/**
 * It manages the current tuned channel among browsable channels. And it determines the next channel
 * by channel up/down. But, it doesn't actually tune through TvView.
 */
@MainThread
public class ChannelTuner {
    private static final String TAG = "ChannelTuner";
    private static final boolean DEBUG = false || SystemProperties.USE_DEBUG_CHANNEL_UPDATE.getValue();
    private static final boolean DEBUG_LOCK = false || SystemProperties.USE_DEBUG_LOCK.getValue();
    private static final String BROADCAST_SKIP_ALL_CHANNELS = "android.action.skip.all.channels";

    private boolean mStarted;
    private boolean mChannelDataManagerLoaded;
    private final List<Channel> mChannels = new ArrayList<>();
    private final List<Channel> mBrowsableChannels = new ArrayList<>();
    private final List<Channel> mVideoChannels = new ArrayList<>();
    private final List<Channel> mRadioChannels = new ArrayList<>();
    private final Map<Long, Channel> mChannelMap = new HashMap<>();
    // TODO: need to check that mChannelIndexMap can be removed, once mCurrentChannelIndex
    // is changed to mCurrentChannel(Id).
    private final Map<Long, Integer> mChannelIndexMap = new HashMap<>();

    private final Handler mHandler = new Handler();
    private final ChannelDataManager mChannelDataManager;
    private final Set<Listener> mListeners = new ArraySet<>();
    @Nullable private Channel mCurrentChannel;
    private final TvInputManagerHelper mInputManager;
    @Nullable private TvInputInfo mCurrentChannelInputInfo;

    private Context mContext;
    private ReentrantReadWriteLock mReentrantReadWriteLock = new ReentrantReadWriteLock();

    private final ChannelDataManager.Listener mChannelDataManagerListener =
            new ChannelDataManager.Listener() {
                @Override
                public void onLoadFinished() {
                    mChannelDataManagerLoaded = true;
                    updateChannelData(mChannelDataManager.getChannelList());
                    for (Listener l : mListeners) {
                        l.onLoadFinished();
                    }
                }

                @Override
                public void onChannelListUpdated() {
                    updateChannelData(mChannelDataManager.getChannelList());
                }

                @Override
                public void onChannelBrowsableChanged() {
                    updateBrowsableChannels();
                    for (Listener l : mListeners) {
                        l.onBrowsableChannelListChanged();
                    }
                }
            };

    public ChannelTuner(ChannelDataManager channelDataManager, TvInputManagerHelper inputManager) {
        mChannelDataManager = channelDataManager;
        mInputManager = inputManager;
    }

    /** Starts ChannelTuner. It cannot be called twice before calling {@link #stop}. */
    public void start() {
        if (mStarted) {
            throw new IllegalStateException("start is called twice");
        }
        mStarted = true;
        mChannelDataManager.addListener(mChannelDataManagerListener);
        if (mChannelDataManager.isDbLoadFinished()) {
            mHandler.post(
                    new Runnable() {
                        @Override
                        public void run() {
                            mChannelDataManagerListener.onLoadFinished();
                        }
                    });
        }
    }

    /** Stops ChannelTuner. */
    public void stop() {
        if (!mStarted) {
            return;
        }
        mStarted = false;
        mHandler.removeCallbacksAndMessages(null);
        mChannelDataManager.removeListener(mChannelDataManagerListener);
        mCurrentChannel = null;
        mChannels.clear();
        mBrowsableChannels.clear();
        mChannelMap.clear();
        mChannelIndexMap.clear();
        mVideoChannels.clear();
        mRadioChannels.clear();
        mChannelDataManagerLoaded = false;
    }

    /** Returns true, if all the channels are loaded. */
    public boolean areAllChannelsLoaded() {
        return mChannelDataManagerLoaded;
    }

    /** Returns browsable channel lists. */
    public List<Channel> getBrowsableChannelList() {
        mReentrantReadWriteLock.readLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getBrowsableChannelList writeLock = " + writeLock);
        }
        List<Channel> result = Collections.unmodifiableList(mBrowsableChannels);
        mReentrantReadWriteLock.readLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getBrowsableChannelList unlock");
        }
        return result;
    }

    public List<Channel> getAllChannelList() {
        mReentrantReadWriteLock.readLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getAllChannelList writeLock = " + writeLock);
        }
        List<Channel> result = Collections.unmodifiableList(mChannels);
        mReentrantReadWriteLock.readLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getAllChannelList unlock");
        }
        return result;
    }

    public int getAllChannelListCount() {
        mReentrantReadWriteLock.readLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getAllChannelListCount writeLock = " + writeLock);
        }
        int result = mChannels.size();
        mReentrantReadWriteLock.readLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getAllChannelListCount unlock");
        }
        return result;
    }

    /** Returns the number of browsable channels. */
    public int getBrowsableChannelCount() {
        mReentrantReadWriteLock.readLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getBrowsableChannelCount writeLock = " + writeLock);
        }
        int result = mBrowsableChannels.size();
        mReentrantReadWriteLock.readLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getBrowsableChannelCount unlock");
        }
        return result;
    }

    /** Returns the current channel. */
    @Nullable
    public Channel getCurrentChannel() {
        return mCurrentChannel;
    }

    /**
     * Returns the current channel index.
     */
    public int getCurrentChannelIndex() {
        mReentrantReadWriteLock.readLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getCurrentChannelIndex writeLock = " + writeLock);
        }
        int currentChannelIndex = 0;
        if (mCurrentChannel != null) {
            Integer integer = mChannelIndexMap.get(mCurrentChannel.getId());
            if (integer != null) {
                currentChannelIndex = integer.intValue();
            }
        }
        mReentrantReadWriteLock.readLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getCurrentChannelIndex unlock");
        }
        return currentChannelIndex;
    }

    /**
     * Returns the channel index by given channel.
     */
    public int getChannelIndex(Channel channel) {
        mReentrantReadWriteLock.readLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getChannelIndex writeLock = " + writeLock);
        }
        int result = mChannelIndexMap.get(channel.getId());
        mReentrantReadWriteLock.readLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getChannelIndex unlock");
        }
        return result;
    }

    /**
     * Returns the channel by index.
     */
    public Channel getChannelByIndex(int channelIndex) {
        mReentrantReadWriteLock.readLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getChannelByIndex2 writeLock = " + writeLock);
        }
        Channel result = null;
        if (channelIndex >= 0 && channelIndex < mChannels.size()) {
            result = mChannels.get(channelIndex);
         }
        mReentrantReadWriteLock.readLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getChannelByIndex2 unlock");
        }
        return result;
    }

    /**
     * Returns the channel index which will be deleted by channelId.
     */
    public int getChannelIndexById(long channelId) {
        mReentrantReadWriteLock.readLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getChannelIndexById writeLock = " + writeLock);
        }
        int mChannelIndex = 0;
        for (int i = 0; i < mChannels.size(); i++) {
            if (mChannels.get(i).getId() == channelId) {
                mChannelIndex = i;
                break;
            }
        }
        mReentrantReadWriteLock.readLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getChannelIndexById unlock");
        }
        return mChannelIndex;
    }

    /**
     * Sets the current channel. Call this method only when setting the current channel without
     * actually tuning to it.
     *
     * @param currentChannel The new current channel to set to.
     */
    public void setCurrentChannel(Channel currentChannel) {
        mCurrentChannel = currentChannel;
        if (DEBUG) {
            Log.d(TAG, "setCurrentChannel " + mCurrentChannel);
        }
    }

    /** Returns the current channel's ID. */
    public long getCurrentChannelId() {
        return mCurrentChannel != null ? mCurrentChannel.getId() : Channel.INVALID_ID;
    }

    /** Returns the current channel's URI */
    public Uri getCurrentChannelUri() {
        if (mCurrentChannel == null) {
            return null;
        }
        if (mCurrentChannel.isPassthrough()) {
            return TvContract.buildChannelUriForPassthroughInput(mCurrentChannel.getInputId());
        } else {
            return TvContract.buildChannelUri(mCurrentChannel.getId());
        }
    }

    /** Returns the current {@link TvInputInfo}. */
    @Nullable
    public TvInputInfo getCurrentInputInfo() {
        return mCurrentChannelInputInfo;
    }

    public boolean setCurrentInputInfo(TvInputInfo prepareone) {
        if (prepareone != null) {
            mCurrentChannelInputInfo = mInputManager.getTvInputInfo(prepareone.getId());
            return true;
        }
        return false;
    }

    /** Returns true, if the current channel is for a passthrough TV input. */
    public boolean isCurrentChannelPassthrough() {
        return mCurrentChannel != null && mCurrentChannel.isPassthrough();
    }

    /**
     * Moves the current channel to the next (or previous) browsable channel.
     *
     * @return true, if the channel is changed to the adjacent channel. If there is no browsable
     *     channel, it returns false.
     */
    public boolean moveToAdjacentBrowsableChannel(boolean up) {
        Channel channel = getAdjacentBrowsableChannel(up);
        if (channel == null || (mCurrentChannel != null && mCurrentChannel.equals(channel))) {
            return false;
        }
        setCurrentChannelAndNotify(mChannelMap.get(channel.getId()));
        return true;
    }

    /**
     * Returns a next browsable channel. It doesn't change the current channel unlike {@link
     * #moveToAdjacentBrowsableChannel}.
     */
    public Channel getAdjacentBrowsableChannel(boolean up) {
        mReentrantReadWriteLock.readLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getAdjacentBrowsableChannel writeLock = " + writeLock);
        }
        if (isCurrentChannelPassthrough() || /*getBrowsableChannelCount()*/mBrowsableChannels.size() == 0) {
            mReentrantReadWriteLock.readLock().unlock();
            if (DEBUG_LOCK) {
                Log.d(TAG, "getAdjacentBrowsableChannel unlock 1");
            }
            return null;
        }
        int channelIndex;
        if (mCurrentChannel == null) {
            channelIndex = 0;
            Channel channel = mChannels.get(channelIndex);
            if (((!channel.isOtherChannel() && channel.isBrowsable()) || (channel.isOtherChannel() && !channel.IsHidden())) && ((MainActivity)mContext).mQuickKeyInfo.isChannelMatchAtvDtvSource(channel)) {
                mReentrantReadWriteLock.readLock().unlock();
                if (DEBUG_LOCK) {
                    Log.d(TAG, "getAdjacentBrowsableChannel unlock 2");
                }
                return channel;
            }
        } else {
            channelIndex = mChannelIndexMap.get(mCurrentChannel.getId());
        }
        int size = mChannels.size();
        for (int i = 0; i < size; ++i) {
            int nextChannelIndex = up ? channelIndex + 1 + i : channelIndex - 1 - i + size;
            if (nextChannelIndex >= size) {
                nextChannelIndex -= size;
            }
            Channel channel = mChannels.get(nextChannelIndex);
            if (((!channel.isOtherChannel() && channel.isBrowsable()) || (channel.isOtherChannel() && !channel.IsHidden())) && ((MainActivity)mContext).mQuickKeyInfo.isChannelMatchAtvDtvSource(channel)) {
                mReentrantReadWriteLock.readLock().unlock();
                if (DEBUG_LOCK) {
                    Log.d(TAG, "getAdjacentBrowsableChannel unlock 3");
                }
                return channel;
            }
        }
        Log.e(TAG, "This code should not be reached");
        mReentrantReadWriteLock.readLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getAdjacentBrowsableChannel unlock 4");
        }
        return null;
    }

    /**
     * Finds the nearest browsable channel from a channel with {@code channelId}. If the channel
     * with {@code channelId} is browsable, the channel will be returned.
     */
    public Channel findNearestBrowsableChannel(long channelId) {
        mReentrantReadWriteLock.readLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "findNearestBrowsableChannel writeLock = " + writeLock);
        }
        if (/*getBrowsableChannelCount()*/mBrowsableChannels.size() == 0) {
            mReentrantReadWriteLock.readLock().unlock();
            if (DEBUG_LOCK) {
                Log.d(TAG, "findNearestBrowsableChannel unlock 1");
            }
            return null;
        }
        Channel channel = mChannelMap.get(channelId);
        if (channel == null) {
            channel = mBrowsableChannels.get(0);
            mReentrantReadWriteLock.readLock().unlock();
            if (DEBUG_LOCK) {
                Log.d(TAG, "findNearestBrowsableChannel unlock 2");
            }
            return channel/*mBrowsableChannels.get(0)*/;
        } else if ((!channel.isOtherChannel() && channel.isBrowsable()) || (channel.isOtherChannel() && !channel.IsHidden())) {
            mReentrantReadWriteLock.readLock().unlock();
            if (DEBUG_LOCK) {
                Log.d(TAG, "findNearestBrowsableChannel unlock 3");
            }
        }
        int index = mChannelIndexMap.get(channelId);
        int size = mChannels.size();
        for (int i = 1; i <= size / 2; ++i) {
            Channel upChannel = mChannels.get((index + i) % size);
            if ((!upChannel.isOtherChannel() && upChannel.isBrowsable()) || (upChannel.isOtherChannel() && !upChannel.IsHidden())) {
                mReentrantReadWriteLock.readLock().unlock();
                if (DEBUG_LOCK) {
                    Log.d(TAG, "findNearestBrowsableChannel unlock 4");
                }
                return upChannel;
            }
            Channel downChannel = mChannels.get((index - i + size) % size);
            if ((!downChannel.isOtherChannel() && downChannel.isBrowsable()) || (downChannel.isOtherChannel() && !downChannel.IsHidden())) {
                if (DEBUG_LOCK) {
                    Log.d(TAG, "findNearestBrowsableChannel unlock 5");
                }
                return downChannel;
            }
        }
        mReentrantReadWriteLock.readLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "findNearestBrowsableChannel unlock 6");
        }
        throw new IllegalStateException(
                "This code should be unreachable in findNearestBrowsableChannel");
    }

    /**
     * Moves the current channel to {@code channel}. It can move to a non-browsable channel as well
     * as a browsable channel.
     *
     * @return true, the channel change is success. But, if the channel doesn't exist, the channel
     *     change will be failed and it will return false.
     */
    public boolean moveToChannel(Channel channel) {
        mReentrantReadWriteLock.readLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "moveToChannel writeLock = " + writeLock);
        }
        if (channel == null) {
            mReentrantReadWriteLock.readLock().unlock();
            if (DEBUG_LOCK) {
                Log.d(TAG, "moveToChannel unlock 1");
            }
            return false;
        }
        if (channel.isPassthrough()) {
            setCurrentChannelAndNotify(channel);
            mReentrantReadWriteLock.readLock().unlock();
            if (DEBUG_LOCK) {
                Log.d(TAG, "moveToChannel unlock 2");
            }
            return true;
        }
        SoftPreconditions.checkState(mChannelDataManagerLoaded, TAG, "Channel data is not loaded");
        Channel newChannel = mChannelMap.get(channel.getId());
        if (newChannel != null && ((MainActivity)mContext).mQuickKeyInfo.isChannelMatchAtvDtvSource(channel)) {
            setCurrentChannelAndNotify(newChannel);
            mReentrantReadWriteLock.readLock().unlock();
            if (DEBUG_LOCK) {
                Log.d(TAG, "moveToChannel unlock 3");
            }
            return true;
        }
        mReentrantReadWriteLock.readLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "moveToChannel unlock 1");
        }
        return false;
    }

    /** Resets the current channel to {@code null}. */
    public void resetCurrentChannel() {
        setCurrentChannelAndNotify(null);
    }

    /** Adds {@link Listener}. */
    public void addListener(Listener listener) {
        mListeners.add(listener);
    }

    /** Removes {@link Listener}. */
    public void removeListener(Listener listener) {
        mListeners.remove(listener);
    }

    public interface Listener {
        /** Called when all the channels are loaded. */
        void onLoadFinished();
        /** Called when the browsable channel list is changed. */
        void onBrowsableChannelListChanged();
        /** Called when the current channel is removed. */
        void onCurrentChannelUnavailable(Channel channel);
        /** Called when the current channel is changed. */
        void onChannelChanged(Channel previousChannel, Channel currentChannel);
        void onAllChannelsListChanged();
    }

    private void setCurrentChannelAndNotify(Channel channel) {
        if (mCurrentChannel == channel
                || (channel != null && channel.hasSameReadOnlyInfo(mCurrentChannel))) {
            return;
        }
        Channel previousChannel = mCurrentChannel;
        if (previousChannel != null) {
            setRecentChannelId(previousChannel);
        }
        mCurrentChannel = channel;
        if (DEBUG) {
            Log.d(TAG, "setCurrentChannelAndNotify " + mCurrentChannel);
        }
        if (mCurrentChannel != null) {
            mCurrentChannelInputInfo = mInputManager.getTvInputInfo(mCurrentChannel.getInputId());
        }
        for (Listener l : mListeners) {
            l.onChannelChanged(previousChannel, mCurrentChannel);
        }
    }

    private void updateChannelData(List<Channel> channels) {
        mReentrantReadWriteLock.writeLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "updateChannelData channels = " + (channels != null ? channels.size() : 0) + ", writeLock = " + writeLock);
        }
        if (mContext != null) {
            //[DroidLogic]
            //when updateChannelData,save the channel counts in Settings.
            Settings.System.putInt(mContext.getContentResolver(), DroidLogicTvUtils.ALL_CHANNELS_NUMBER, channels.size());
        }
        mChannels.clear();
        mChannels.addAll(channels);
        Collections.sort(mChannels, new TypeComparator());

        mChannelMap.clear();
        mChannelIndexMap.clear();
        mVideoChannels.clear();
        mRadioChannels.clear();
        for (int i = 0; i < channels.size(); ++i) {
            Channel channel = channels.get(i);
            long channelId = channel.getId();
            mChannelMap.put(channelId, channel);
            mChannelIndexMap.put(channelId, i);
            if (channel.isVideoChannel()) {
                mVideoChannels.add(channel);
            } else if (channel.isRadioChannel()) {
                mRadioChannels.add(channel);
            }
            if (DEBUG_LOCK) {
                Log.d(TAG, "updateChannelData no." + i + "->" + channel);
            }
        }
        updateBrowsableChannelsLocked();

        if (mCurrentChannel != null && !mCurrentChannel.isPassthrough()) {
            Channel prevChannel = mCurrentChannel;
            setCurrentChannelAndNotify(mChannelMap.get(mCurrentChannel.getId()));
            if (mCurrentChannel == null) {
                for (Listener l : mListeners) {
                    l.onCurrentChannelUnavailable(prevChannel);
                }
            }
        }
        // TODO: Do not call onBrowsableChannelListChanged, when only non-browsable
        // channels are changed.
        for (Listener l : mListeners) {
            l.onAllChannelsListChanged();
            l.onBrowsableChannelListChanged();
        }
        mReentrantReadWriteLock.writeLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "updateChannelData unlock");
        }
    }

    private void updateBrowsableChannels() {
        mReentrantReadWriteLock.writeLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "updateBrowsableChannels writeLock = " + writeLock);
        }
        updateBrowsableChannelsLocked();
        mReentrantReadWriteLock.writeLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "updateBrowsableChannels unlock");
        }
    }

    private void updateBrowsableChannelsLocked() {
        if (DEBUG) {
            Log.d(TAG, "updateBrowsableChannels");
        }
        mBrowsableChannels.clear();
        int i = 0;
        for (Channel channel : mChannels) {
            if (DEBUG) Log.d(TAG, "updateBrowsableChannelsLocked no." + (i++) + "->" + channel);
            //also delete hidden channel
            if ((!channel.isOtherChannel() && channel.isBrowsable()) || (channel.isOtherChannel() && !channel.IsHidden())) {//other source may not have permissions to write browse
                mBrowsableChannels.add(channel);
            }
        }

        if (!Utils.isCurrentDeviceIdPassthrough(mContext)) {
            if (mBrowsableChannels.size() == 0 ) {
                Intent intent = new Intent();
                intent.setAction(BROADCAST_SKIP_ALL_CHANNELS);
                mContext.sendBroadcast(intent);
            } else {
                Log.d(TAG, "updateBrowsableChannelsLocked mBrowsableChannels.size(): " + mBrowsableChannels.size());
            }
        }
    }

    public void setContext(Context context) {
        mContext = context;
    }

    public void setRecentChannelId(Channel channel) {
        if (mContext != null && channel != null) {
            long recentChannelIndex = Utils.getRecentWatchedChannelId(mContext);
            if (recentChannelIndex != channel.getId()) {
                Utils.setRecentWatchedChannelId(mContext, channel);
            }
        }
    }

    public Channel getChannelById(long id) {
        mReentrantReadWriteLock.writeLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getChannelById writeLock = " + writeLock);
        }
        Channel channelbyid = null;
        if (mChannelMap != null) {
            channelbyid = mChannelMap.get(id);
        }
        mReentrantReadWriteLock.writeLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getChannelById unlock");
        }
        return channelbyid;
    }

    public List<Channel> getVideoChannelList() {
        mReentrantReadWriteLock.readLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getVideoChannelList writeLock = " + writeLock);
        }
        List<Channel> result = Collections.unmodifiableList(mVideoChannels);
        mReentrantReadWriteLock.readLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getVideoChannelList unlock");
        }
        return result;
    }

    public List<Channel> getRadioChannelList() {
        mReentrantReadWriteLock.readLock().lock();
        boolean writeLock = mReentrantReadWriteLock.isWriteLocked();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getRadioChannelList writeLock = " + writeLock);
        }
        List<Channel> result = Collections.unmodifiableList(mRadioChannels);
        mReentrantReadWriteLock.readLock().unlock();
        if (DEBUG_LOCK) {
            Log.d(TAG, "getRadioChannelList unlock");
        }
        return result;
    }

    private class TypeComparator implements Comparator<Channel> {
        public int compare(Channel object1, Channel object2) {
            /*boolean b1 = object1.isVideoChannel() ;
            boolean b2 = object2.isVideoChannel();
            if (b1 && !b2) {
                return -1;
            } else if (!b1 && b2) {
                return 1;
            }
            b1 = object1.isOtherChannel();
            b2 = object2.isOtherChannel();
            //add other channel before analog channel
            if (b1 && !b2) {
                if (object2.isAnalogChannel()) {
                   return -1;
                }
            } else if (!b1 && b2) {
                if (object1.isAnalogChannel()) {
                   return 1;
                }
            }*/
            //sort as display number for channel list
            String p1 = object1.getDisplayNumber();
            String p2 = object2.getDisplayNumber();
            return ChannelNumber.compare(p1, p2);
        }
    }
}
