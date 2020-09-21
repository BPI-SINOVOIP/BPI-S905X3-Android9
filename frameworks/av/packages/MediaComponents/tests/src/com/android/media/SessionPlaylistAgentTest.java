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

package com.android.media;

import static org.mockito.Mockito.*;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.DataSourceDesc;
import android.media.MediaItem2;
import android.media.MediaMetadata2;
import android.media.MediaPlayerBase;
import android.media.MediaPlayerBase.PlayerEventCallback;
import android.media.MediaPlaylistAgent;
import android.media.MediaSession2;
import android.media.MediaSession2.OnDataSourceMissingHelper;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.test.AndroidTestCase;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Matchers;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Executor;

/**
 * Tests {@link SessionPlaylistAgent}.
 */
public class SessionPlaylistAgentTest extends AndroidTestCase {
    private static final String TAG = "SessionPlaylistAgentTest";
    private static final int WAIT_TIME_MS = 1000;
    private static final int INVALID_REPEAT_MODE = -100;
    private static final int INVALID_SHUFFLE_MODE = -100;

    private Handler mHandler;
    private Executor mHandlerExecutor;

    private Object mWaitLock = new Object();
    private Context mContext;
    private MediaSession2Impl mSessionImpl;
    private MediaPlayerBase mPlayer;
    private PlayerEventCallback mPlayerEventCallback;
    private SessionPlaylistAgent mAgent;
    private OnDataSourceMissingHelper mDataSourceHelper;
    private MyPlaylistEventCallback mEventCallback;

    public class MyPlaylistEventCallback extends MediaPlaylistAgent.PlaylistEventCallback {
        boolean onPlaylistChangedCalled;
        boolean onPlaylistMetadataChangedCalled;
        boolean onRepeatModeChangedCalled;
        boolean onShuffleModeChangedCalled;

        private Object mWaitLock;

        public MyPlaylistEventCallback(Object waitLock) {
            mWaitLock = waitLock;
        }

        public void clear() {
            onPlaylistChangedCalled = false;
            onPlaylistMetadataChangedCalled = false;
            onRepeatModeChangedCalled = false;
            onShuffleModeChangedCalled = false;
        }

        public void onPlaylistChanged(MediaPlaylistAgent playlistAgent, List<MediaItem2> list,
                MediaMetadata2 metadata) {
            synchronized (mWaitLock) {
                onPlaylistChangedCalled = true;
                mWaitLock.notify();
            }
        }

        public void onPlaylistMetadataChanged(MediaPlaylistAgent playlistAgent,
                MediaMetadata2 metadata) {
            synchronized (mWaitLock) {
                onPlaylistMetadataChangedCalled = true;
                mWaitLock.notify();
            }
        }

        public void onRepeatModeChanged(MediaPlaylistAgent playlistAgent, int repeatMode) {
            synchronized (mWaitLock) {
                onRepeatModeChangedCalled = true;
                mWaitLock.notify();
            }
        }

        public void onShuffleModeChanged(MediaPlaylistAgent playlistAgent, int shuffleMode) {
            synchronized (mWaitLock) {
                onShuffleModeChangedCalled = true;
                mWaitLock.notify();
            }
        }
    }

    public class MyDataSourceHelper implements OnDataSourceMissingHelper {
        @Override
        public DataSourceDesc onDataSourceMissing(MediaSession2 session, MediaItem2 item) {
            if (item.getMediaId().contains("WITHOUT_DSD")) {
                return null;
            }
            return new DataSourceDesc.Builder()
                    .setDataSource(getContext(), Uri.parse("dsd://test"))
                    .setMediaId(item.getMediaId())
                    .build();
        }
    }

    public class MockPlayer extends MediaPlayerBase {
        @Override
        public void play() {
        }

        @Override
        public void prepare() {
        }

        @Override
        public void pause() {
        }

        @Override
        public void reset() {
        }

        @Override
        public void skipToNext() {
        }

        @Override
        public void seekTo(long pos) {
        }

        @Override
        public int getPlayerState() {
            return 0;
        }

        @Override
        public int getBufferingState() {
            return 0;
        }

        @Override
        public void setAudioAttributes(AudioAttributes attributes) {
        }

        @Override
        public AudioAttributes getAudioAttributes() {
            return null;
        }

        @Override
        public void setDataSource(DataSourceDesc dsd) {
        }

        @Override
        public void setNextDataSource(DataSourceDesc dsd) {
        }

        @Override
        public void setNextDataSources(List<DataSourceDesc> dsds) {
        }

        @Override
        public DataSourceDesc getCurrentDataSource() {
            return null;
        }

        @Override
        public void loopCurrent(boolean loop) {
        }

        @Override
        public void setPlaybackSpeed(float speed) {
        }

        @Override
        public void setPlayerVolume(float volume) {
        }

        @Override
        public float getPlayerVolume() {
            return 0;
        }

        @Override
        public void registerPlayerEventCallback(Executor e, PlayerEventCallback cb) {
        }

        @Override
        public void unregisterPlayerEventCallback(PlayerEventCallback cb) {
        }

        @Override
        public void close() throws Exception {
        }
    }

    @Before
    public void setUp() throws Exception {
        mContext = getContext();
        // Workaround for dexmaker bug: https://code.google.com/p/dexmaker/issues/detail?id=2
        // Dexmaker is used by mockito.
        System.setProperty("dexmaker.dexcache", mContext.getCacheDir().getPath());

        HandlerThread handlerThread = new HandlerThread("SessionPlaylistAgent");
        handlerThread.start();
        mHandler = new Handler(handlerThread.getLooper());
        mHandlerExecutor = (runnable) -> {
            mHandler.post(runnable);
        };

        mPlayer = mock(MockPlayer.class);
        doAnswer(invocation -> {
            Object[] args = invocation.getArguments();
            mPlayerEventCallback = (PlayerEventCallback) args[1];
            return null;
        }).when(mPlayer).registerPlayerEventCallback(Matchers.any(), Matchers.any());

        mSessionImpl = mock(MediaSession2Impl.class);
        mDataSourceHelper = new MyDataSourceHelper();
        mAgent = new SessionPlaylistAgent(mSessionImpl, mPlayer);
        mAgent.setOnDataSourceMissingHelper(mDataSourceHelper);
        mEventCallback = new MyPlaylistEventCallback(mWaitLock);
        mAgent.registerPlaylistEventCallback(mHandlerExecutor, mEventCallback);
    }

    @After
    public void tearDown() throws Exception {
        mHandler.getLooper().quitSafely();
        mHandler = null;
        mHandlerExecutor = null;
    }

    @Test
    public void testSetAndGetShuflleMode() throws Exception {
        int shuffleMode = mAgent.getShuffleMode();
        if (shuffleMode != MediaPlaylistAgent.SHUFFLE_MODE_NONE) {
            mEventCallback.clear();
            synchronized (mWaitLock) {
                mAgent.setShuffleMode(MediaPlaylistAgent.SHUFFLE_MODE_NONE);
                mWaitLock.wait(WAIT_TIME_MS);
                assertTrue(mEventCallback.onShuffleModeChangedCalled);
            }
            assertEquals(MediaPlaylistAgent.SHUFFLE_MODE_NONE, mAgent.getShuffleMode());
        }

        mEventCallback.clear();
        synchronized (mWaitLock) {
            mAgent.setShuffleMode(MediaPlaylistAgent.SHUFFLE_MODE_ALL);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onShuffleModeChangedCalled);
        }
        assertEquals(MediaPlaylistAgent.SHUFFLE_MODE_ALL, mAgent.getShuffleMode());

        mEventCallback.clear();
        synchronized (mWaitLock) {
            mAgent.setShuffleMode(MediaPlaylistAgent.SHUFFLE_MODE_GROUP);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onShuffleModeChangedCalled);
        }
        assertEquals(MediaPlaylistAgent.SHUFFLE_MODE_GROUP, mAgent.getShuffleMode());

        // INVALID_SHUFFLE_MODE will not change the shuffle mode.
        mEventCallback.clear();
        synchronized (mWaitLock) {
            mAgent.setShuffleMode(INVALID_SHUFFLE_MODE);
            mWaitLock.wait(WAIT_TIME_MS);
            assertFalse(mEventCallback.onShuffleModeChangedCalled);
        }
        assertEquals(MediaPlaylistAgent.SHUFFLE_MODE_GROUP, mAgent.getShuffleMode());
    }

    @Test
    public void testSetAndGetRepeatMode() throws Exception {
        int repeatMode = mAgent.getRepeatMode();
        if (repeatMode != MediaPlaylistAgent.REPEAT_MODE_NONE) {
            mEventCallback.clear();
            synchronized (mWaitLock) {
                mAgent.setRepeatMode(MediaPlaylistAgent.REPEAT_MODE_NONE);
                mWaitLock.wait(WAIT_TIME_MS);
                assertTrue(mEventCallback.onRepeatModeChangedCalled);
            }
            assertEquals(MediaPlaylistAgent.REPEAT_MODE_NONE, mAgent.getRepeatMode());
        }

        mEventCallback.clear();
        synchronized (mWaitLock) {
            mAgent.setRepeatMode(MediaPlaylistAgent.REPEAT_MODE_ONE);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onRepeatModeChangedCalled);
        }
        assertEquals(MediaPlaylistAgent.REPEAT_MODE_ONE, mAgent.getRepeatMode());

        mEventCallback.clear();
        synchronized (mWaitLock) {
            mAgent.setRepeatMode(MediaPlaylistAgent.REPEAT_MODE_ALL);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onRepeatModeChangedCalled);
        }
        assertEquals(MediaPlaylistAgent.REPEAT_MODE_ALL, mAgent.getRepeatMode());

        mEventCallback.clear();
        synchronized (mWaitLock) {
            mAgent.setRepeatMode(MediaPlaylistAgent.REPEAT_MODE_GROUP);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onRepeatModeChangedCalled);
        }
        assertEquals(MediaPlaylistAgent.REPEAT_MODE_GROUP, mAgent.getRepeatMode());

        // INVALID_SHUFFLE_MODE will not change the shuffle mode.
        mEventCallback.clear();
        synchronized (mWaitLock) {
            mAgent.setRepeatMode(INVALID_REPEAT_MODE);
            mWaitLock.wait(WAIT_TIME_MS);
            assertFalse(mEventCallback.onRepeatModeChangedCalled);
        }
        assertEquals(MediaPlaylistAgent.REPEAT_MODE_GROUP, mAgent.getRepeatMode());
    }

    @Test
    public void testSetPlaylist() throws Exception {
        int listSize = 10;
        createAndSetPlaylist(10);
        assertEquals(listSize, mAgent.getPlaylist().size());
        assertEquals(0, mAgent.getCurShuffledIndex());
    }

    @Test
    public void testSkipItems() throws Exception {
        int listSize = 5;
        List<MediaItem2> playlist = createAndSetPlaylist(listSize);

        mAgent.setRepeatMode(MediaPlaylistAgent.REPEAT_MODE_NONE);
        // Test skipToPlaylistItem
        for (int i = listSize - 1; i >= 0; --i) {
            mAgent.skipToPlaylistItem(playlist.get(i));
            assertEquals(i, mAgent.getCurShuffledIndex());
        }

        // Test skipToNextItem
        // curPlayPos = 0
        for (int curPlayPos = 0; curPlayPos < listSize - 1; ++curPlayPos) {
            mAgent.skipToNextItem();
            assertEquals(curPlayPos + 1, mAgent.getCurShuffledIndex());
        }
        mAgent.skipToNextItem();
        assertEquals(listSize - 1, mAgent.getCurShuffledIndex());

        // Test skipToPrevious
        // curPlayPos = listSize - 1
        for (int curPlayPos = listSize - 1; curPlayPos > 0; --curPlayPos) {
            mAgent.skipToPreviousItem();
            assertEquals(curPlayPos - 1, mAgent.getCurShuffledIndex());
        }
        mAgent.skipToPreviousItem();
        assertEquals(0, mAgent.getCurShuffledIndex());

        mAgent.setRepeatMode(MediaPlaylistAgent.REPEAT_MODE_ALL);
        // Test skipToPrevious with repeat mode all
        // curPlayPos = 0
        mAgent.skipToPreviousItem();
        assertEquals(listSize - 1, mAgent.getCurShuffledIndex());

        // Test skipToNext with repeat mode all
        // curPlayPos = listSize - 1
        mAgent.skipToNextItem();
        assertEquals(0, mAgent.getCurShuffledIndex());

        mAgent.skipToPreviousItem();
        // curPlayPos = listSize - 1, nextPlayPos = 0
        // Test next play pos after setting repeat mode none.
        mAgent.setRepeatMode(MediaPlaylistAgent.REPEAT_MODE_NONE);
        assertEquals(listSize - 1, mAgent.getCurShuffledIndex());
    }

    @Test
    public void testEditPlaylist() throws Exception {
        int listSize = 5;
        List<MediaItem2> playlist = createAndSetPlaylist(listSize);

        // Test add item: [0 (cur), 1, 2, 3, 4] -> [0 (cur), 1, 5, 2, 3, 4]
        mEventCallback.clear();
        MediaItem2 item_5 = generateMediaItem(5);
        synchronized (mWaitLock) {
            playlist.add(2, item_5);
            mAgent.addPlaylistItem(2, item_5);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onPlaylistChangedCalled);
        }
        assertPlaylistEquals(playlist, mAgent.getPlaylist());

        mEventCallback.clear();
        // Move current: [0 (cur), 1, 5, 2, 3, 4] -> [0, 1, 5 (cur), 2, 3, 4]
        mAgent.skipToPlaylistItem(item_5);
        // Remove current item: [0, 1, 5 (cur), 2, 3, 4] -> [0, 1, 2 (cur), 3, 4]
        synchronized (mWaitLock) {
            playlist.remove(item_5);
            mAgent.removePlaylistItem(item_5);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onPlaylistChangedCalled);
        }
        assertPlaylistEquals(playlist, mAgent.getPlaylist());
        assertEquals(2, mAgent.getCurShuffledIndex());

        // Remove previous item: [0, 1, 2 (cur), 3, 4] -> [0, 2 (cur), 3, 4]
        mEventCallback.clear();
        MediaItem2 previousItem = playlist.get(1);
        synchronized (mWaitLock) {
            playlist.remove(previousItem);
            mAgent.removePlaylistItem(previousItem);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onPlaylistChangedCalled);
        }
        assertPlaylistEquals(playlist, mAgent.getPlaylist());
        assertEquals(1, mAgent.getCurShuffledIndex());

        // Remove next item: [0, 2 (cur), 3, 4] -> [0, 2 (cur), 4]
        mEventCallback.clear();
        MediaItem2 nextItem = playlist.get(2);
        synchronized (mWaitLock) {
            playlist.remove(nextItem);
            mAgent.removePlaylistItem(nextItem);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onPlaylistChangedCalled);
        }
        assertPlaylistEquals(playlist, mAgent.getPlaylist());
        assertEquals(1, mAgent.getCurShuffledIndex());

        // Replace item: [0, 2 (cur), 4] -> [0, 2 (cur), 5]
        mEventCallback.clear();
        synchronized (mWaitLock) {
            playlist.set(2, item_5);
            mAgent.replacePlaylistItem(2, item_5);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onPlaylistChangedCalled);
        }
        assertPlaylistEquals(playlist, mAgent.getPlaylist());
        assertEquals(1, mAgent.getCurShuffledIndex());

        // Move last and remove the last item: [0, 2 (cur), 5] -> [0, 2, 5 (cur)] -> [0, 2 (cur)]
        MediaItem2 lastItem = playlist.get(1);
        mAgent.skipToPlaylistItem(lastItem);
        mEventCallback.clear();
        synchronized (mWaitLock) {
            playlist.remove(lastItem);
            mAgent.removePlaylistItem(lastItem);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onPlaylistChangedCalled);
        }
        assertPlaylistEquals(playlist, mAgent.getPlaylist());
        assertEquals(1, mAgent.getCurShuffledIndex());

        // Remove all items
        for (int i = playlist.size() - 1; i >= 0; --i) {
            MediaItem2 item = playlist.get(i);
            mAgent.skipToPlaylistItem(item);
            mEventCallback.clear();
            synchronized (mWaitLock) {
                playlist.remove(item);
                mAgent.removePlaylistItem(item);
                mWaitLock.wait(WAIT_TIME_MS);
                assertTrue(mEventCallback.onPlaylistChangedCalled);
            }
            assertPlaylistEquals(playlist, mAgent.getPlaylist());
        }
        assertEquals(SessionPlaylistAgent.NO_VALID_ITEMS, mAgent.getCurShuffledIndex());
    }


    @Test
    public void testPlaylistWithInvalidItem() throws Exception {
        int listSize = 2;
        List<MediaItem2> playlist = createAndSetPlaylist(listSize);

        // Add item: [0 (cur), 1] -> [0 (cur), 3 (no_dsd), 1]
        mEventCallback.clear();
        MediaItem2 invalidItem2 = generateMediaItemWithoutDataSourceDesc(2);
        synchronized (mWaitLock) {
            playlist.add(1, invalidItem2);
            mAgent.addPlaylistItem(1, invalidItem2);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onPlaylistChangedCalled);
        }
        assertPlaylistEquals(playlist, mAgent.getPlaylist());
        assertEquals(0, mAgent.getCurShuffledIndex());

        // Test skip to next item:  [0 (cur), 2 (no_dsd), 1] -> [0, 2 (no_dsd), 1 (cur)]
        mAgent.skipToNextItem();
        assertEquals(2, mAgent.getCurShuffledIndex());

        // Test skip to previous item: [0, 2 (no_dsd), 1 (cur)] -> [0 (cur), 2 (no_dsd), 1]
        mAgent.skipToPreviousItem();
        assertEquals(0, mAgent.getCurShuffledIndex());

        // Remove current item: [0 (cur), 2 (no_dsd), 1] -> [2 (no_dsd), 1 (cur)]
        mEventCallback.clear();
        MediaItem2 item = playlist.get(0);
        synchronized (mWaitLock) {
            playlist.remove(item);
            mAgent.removePlaylistItem(item);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onPlaylistChangedCalled);
        }
        assertPlaylistEquals(playlist, mAgent.getPlaylist());
        assertEquals(1, mAgent.getCurShuffledIndex());

        // Remove current item: [2 (no_dsd), 1 (cur)] -> [2 (no_dsd)]
        mEventCallback.clear();
        item = playlist.get(1);
        synchronized (mWaitLock) {
            playlist.remove(item);
            mAgent.removePlaylistItem(item);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onPlaylistChangedCalled);
        }
        assertPlaylistEquals(playlist, mAgent.getPlaylist());
        assertEquals(SessionPlaylistAgent.NO_VALID_ITEMS, mAgent.getCurShuffledIndex());

        // Add invalid item: [2 (no_dsd)] -> [0 (no_dsd), 2 (no_dsd)]
        MediaItem2 invalidItem0 = generateMediaItemWithoutDataSourceDesc(0);
        mEventCallback.clear();
        synchronized (mWaitLock) {
            playlist.add(0, invalidItem0);
            mAgent.addPlaylistItem(0, invalidItem0);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onPlaylistChangedCalled);
        }
        assertPlaylistEquals(playlist, mAgent.getPlaylist());
        assertEquals(SessionPlaylistAgent.NO_VALID_ITEMS, mAgent.getCurShuffledIndex());

        // Add valid item: [0 (no_dsd), 2 (no_dsd)] -> [0 (no_dsd), 1, 2 (no_dsd)]
        MediaItem2 invalidItem1 = generateMediaItem(1);
        mEventCallback.clear();
        synchronized (mWaitLock) {
            playlist.add(1, invalidItem1);
            mAgent.addPlaylistItem(1, invalidItem1);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onPlaylistChangedCalled);
        }
        assertPlaylistEquals(playlist, mAgent.getPlaylist());
        assertEquals(1, mAgent.getCurShuffledIndex());

        // Replace the valid item with an invalid item:
        // [0 (no_dsd), 1 (cur), 2 (no_dsd)] -> [0 (no_dsd), 3 (no_dsd), 2 (no_dsd)]
        MediaItem2 invalidItem3 = generateMediaItemWithoutDataSourceDesc(3);
        mEventCallback.clear();
        synchronized (mWaitLock) {
            playlist.set(1, invalidItem3);
            mAgent.replacePlaylistItem(1, invalidItem3);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onPlaylistChangedCalled);
        }
        assertPlaylistEquals(playlist, mAgent.getPlaylist());
        assertEquals(SessionPlaylistAgent.END_OF_PLAYLIST, mAgent.getCurShuffledIndex());
    }

    @Test
    public void testPlaylistAfterOnCurrentDataSourceChanged() throws Exception {
        int listSize = 2;
        verify(mPlayer).registerPlayerEventCallback(Matchers.any(), Matchers.any());

        createAndSetPlaylist(listSize);
        assertEquals(0, mAgent.getCurShuffledIndex());

        mPlayerEventCallback.onCurrentDataSourceChanged(mPlayer, null);
        assertEquals(1, mAgent.getCurShuffledIndex());
        mPlayerEventCallback.onCurrentDataSourceChanged(mPlayer, null);
        assertEquals(SessionPlaylistAgent.END_OF_PLAYLIST, mAgent.getCurShuffledIndex());

        mAgent.skipToNextItem();
        assertEquals(SessionPlaylistAgent.END_OF_PLAYLIST, mAgent.getCurShuffledIndex());

        mAgent.setRepeatMode(MediaPlaylistAgent.REPEAT_MODE_ONE);
        assertEquals(SessionPlaylistAgent.END_OF_PLAYLIST, mAgent.getCurShuffledIndex());

        mAgent.setRepeatMode(MediaPlaylistAgent.REPEAT_MODE_ALL);
        assertEquals(0, mAgent.getCurShuffledIndex());
        mPlayerEventCallback.onCurrentDataSourceChanged(mPlayer, null);
        assertEquals(1, mAgent.getCurShuffledIndex());
        mPlayerEventCallback.onCurrentDataSourceChanged(mPlayer, null);
        assertEquals(0, mAgent.getCurShuffledIndex());
    }

    private List<MediaItem2> createAndSetPlaylist(int listSize) throws Exception {
        List<MediaItem2> items = new ArrayList<>();
        for (int i = 0; i < listSize; ++i) {
            items.add(generateMediaItem(i));
        }
        mEventCallback.clear();
        synchronized (mWaitLock) {
            mAgent.setPlaylist(items, null);
            mWaitLock.wait(WAIT_TIME_MS);
            assertTrue(mEventCallback.onPlaylistChangedCalled);
        }
        return items;
    }

    private void assertPlaylistEquals(List<MediaItem2> expected, List<MediaItem2> actual) {
        if (expected == actual) {
            return;
        }
        assertTrue(expected != null && actual != null);
        assertEquals(expected.size(), actual.size());
        for (int i = 0; i < expected.size(); ++i) {
            assertTrue(expected.get(i).equals(actual.get(i)));
        }
    }

    private MediaItem2 generateMediaItemWithoutDataSourceDesc(int key) {
        return new MediaItem2.Builder(0)
                .setMediaId("TEST_MEDIA_ID_WITHOUT_DSD_" + key)
                .build();
    }

    private MediaItem2 generateMediaItem(int key) {
        return new MediaItem2.Builder(0)
                .setMediaId("TEST_MEDIA_ID_" + key)
                .build();
    }
}
