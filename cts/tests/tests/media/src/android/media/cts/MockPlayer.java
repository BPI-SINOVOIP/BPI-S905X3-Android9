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

package android.media.cts;

import android.media.AudioAttributes;
import android.media.DataSourceDesc;
import android.media.MediaPlayerBase;
import androidx.annotation.NonNull;
import android.util.ArrayMap;

import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;

/**
 * A mock implementation of {@link MediaPlayerBase} for testing.
 */
public class MockPlayer extends MediaPlayerBase {
    public final CountDownLatch mCountDownLatch;

    public boolean mPlayCalled;
    public boolean mPauseCalled;
    public boolean mStopCalled;
    public boolean mPrepareCalled;
    public boolean mSeekToCalled;
    public long mSeekPosition;
    public long mCurrentPosition;
    public long mBufferedPosition;
    public @PlayerState int mLastPlayerState;

    public ArrayMap<PlayerEventCallback, Executor> mCallbacks = new ArrayMap<>();

    private AudioAttributes mAudioAttributes;

    public MockPlayer(int count) {
        mCountDownLatch = (count > 0) ? new CountDownLatch(count) : null;
    }

    @Override
    public void close() {
        // no-op
    }

    @Override
    public void reset() {
        // no-op
    }

    @Override
    public void play() {
        mPlayCalled = true;
        if (mCountDownLatch != null) {
            mCountDownLatch.countDown();
        }
    }

    @Override
    public void pause() {
        mPauseCalled = true;
        if (mCountDownLatch != null) {
            mCountDownLatch.countDown();
        }
    }

    // TODO: Uncomment or remove
    /*
    @Override
    public void stop() {
        mStopCalled = true;
        if (mCountDownLatch != null) {
            mCountDownLatch.countDown();
        }
    }
    */

    @Override
    public void prepare() {
        mPrepareCalled = true;
        if (mCountDownLatch != null) {
            mCountDownLatch.countDown();
        }
    }

    @Override
    public void seekTo(long pos) {
        mSeekToCalled = true;
        mSeekPosition = pos;
        if (mCountDownLatch != null) {
            mCountDownLatch.countDown();
        }
    }

    @Override
    public void skipToNext() {
        // No-op. This skipToNext() means 'skip to next item in the setNextDataSources()'
    }

    @Override
    public int getPlayerState() {
        return mLastPlayerState;
    }

    @Override
    public long getCurrentPosition() {
        return mCurrentPosition;
    }

    @Override
    public long getBufferedPosition() {
        return mBufferedPosition;
    }

    @Override
    public int getBufferingState() {
        // TODO: implement this
        return -1;
    }

    @Override
    public void registerPlayerEventCallback(@NonNull Executor executor,
            @NonNull PlayerEventCallback callback) {
        mCallbacks.put(callback, executor);
    }

    @Override
    public void unregisterPlayerEventCallback(@NonNull PlayerEventCallback callback) {
        mCallbacks.remove(callback);
    }

    public void notifyPlaybackState(final int state) {
        mLastPlayerState = state;
        for (int i = 0; i < mCallbacks.size(); i++) {
            final PlayerEventCallback callback = mCallbacks.keyAt(i);
            final Executor executor = mCallbacks.valueAt(i);
            executor.execute(() -> callback.onPlayerStateChanged(this, state));
        }
    }

    public void notifyCurrentDataSourceChanged(DataSourceDesc dsd) {
        for (int i = 0; i < mCallbacks.size(); i++) {
            final PlayerEventCallback callback = mCallbacks.keyAt(i);
            final Executor executor = mCallbacks.valueAt(i);
            executor.execute(() -> callback.onCurrentDataSourceChanged(this, dsd));
        }
    }

    public void notifyMediaPrepared(DataSourceDesc dsd) {
        for (int i = 0; i < mCallbacks.size(); i++) {
            final PlayerEventCallback callback = mCallbacks.keyAt(i);
            final Executor executor = mCallbacks.valueAt(i);
            executor.execute(() -> callback.onMediaPrepared(this, dsd));
        }
    }

    public void notifyBufferingStateChanged(DataSourceDesc dsd, @BuffState int buffState) {
        for (int i = 0; i < mCallbacks.size(); i++) {
            final PlayerEventCallback callback = mCallbacks.keyAt(i);
            final Executor executor = mCallbacks.valueAt(i);
            executor.execute(() -> callback.onBufferingStateChanged(this, dsd, buffState));
        }
    }

    public void notifyError(int what) {
        for (int i = 0; i < mCallbacks.size(); i++) {
            final PlayerEventCallback callback = mCallbacks.keyAt(i);
            final Executor executor = mCallbacks.valueAt(i);
            // TODO: Uncomment or remove
            //executor.execute(() -> callback.onError(null, what, 0));
        }
    }

    @Override
    public void setAudioAttributes(AudioAttributes attributes) {
        mAudioAttributes = attributes;
    }

    @Override
    public AudioAttributes getAudioAttributes() {
        return mAudioAttributes;
    }

    @Override
    public void setDataSource(@NonNull DataSourceDesc dsd) {
        // TODO: Implement this
    }

    @Override
    public void setNextDataSource(@NonNull DataSourceDesc dsd) {
        // TODO: Implement this
    }

    @Override
    public void setNextDataSources(@NonNull List<DataSourceDesc> dsds) {
        // TODO: Implement this
    }

    @Override
    public DataSourceDesc getCurrentDataSource() {
        // TODO: Implement this
        return null;
    }

    @Override
    public void loopCurrent(boolean loop) {
        // TODO: implement this
    }

    @Override
    public void setPlaybackSpeed(float speed) {
        // TODO: implement this
    }

    @Override
    public void setPlayerVolume(float volume) {
        // TODO: implement this
    }

    @Override
    public float getPlayerVolume() {
        // TODO: implement this
        return -1;
    }
}
