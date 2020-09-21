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

package com.android.tv.data;

import static android.support.test.InstrumentationRegistry.getInstrumentation;
import static android.support.test.InstrumentationRegistry.getTargetContext;
import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.ContentObserver;
import android.database.Cursor;
import android.media.tv.TvContract;
import android.media.tv.TvContract.Channels;
import android.net.Uri;
import android.os.AsyncTask;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.test.MoreAsserts;
import android.test.mock.MockContentProvider;
import android.test.mock.MockContentResolver;
import android.test.mock.MockCursor;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;
import com.android.tv.data.api.Channel;
import com.android.tv.testing.constants.Constants;
import com.android.tv.testing.data.ChannelInfo;
import com.android.tv.util.TvInputManagerHelper;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Matchers;
import org.mockito.Mockito;

/**
 * Test for {@link ChannelDataManager}
 *
 * <p>A test method may include tests for multiple methods to minimize the DB access. Note that all
 * the methods of {@link ChannelDataManager} should be called from the UI thread.
 */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class ChannelDataManagerTest {
    private static final boolean DEBUG = false;
    private static final String TAG = "ChannelDataManagerTest";

    // Wait time for expected success.
    private static final long WAIT_TIME_OUT_MS = 1000L;
    private static final String DUMMY_INPUT_ID = "dummy";
    private static final String COLUMN_BROWSABLE = "browsable";
    private static final String COLUMN_LOCKED = "locked";

    private ChannelDataManager mChannelDataManager;
    private TestChannelDataManagerListener mListener;
    private FakeContentResolver mContentResolver;
    private FakeContentProvider mContentProvider;

    @Before
    public void setUp() {
        assertWithMessage("More than 2 channels to test")
                .that(Constants.UNIT_TEST_CHANNEL_COUNT > 2)
                .isTrue();

        mContentProvider = new FakeContentProvider(getTargetContext());
        mContentResolver = new FakeContentResolver();
        mContentResolver.addProvider(TvContract.AUTHORITY, mContentProvider);
        mListener = new TestChannelDataManagerListener();
        getInstrumentation()
                .runOnMainSync(
                        new Runnable() {
                            @Override
                            public void run() {
                                TvInputManagerHelper mockHelper =
                                        Mockito.mock(TvInputManagerHelper.class);
                                Mockito.when(mockHelper.hasTvInputInfo(Matchers.anyString()))
                                        .thenReturn(true);
                                mChannelDataManager =
                                        new ChannelDataManager(
                                                getTargetContext(),
                                                mockHelper,
                                                AsyncTask.SERIAL_EXECUTOR,
                                                mContentResolver);
                                mChannelDataManager.addListener(mListener);
                            }
                        });
    }

    @After
    public void tearDown() {
        getInstrumentation()
                .runOnMainSync(
                        new Runnable() {
                            @Override
                            public void run() {
                                mChannelDataManager.stop();
                            }
                        });
    }

    private void startAndWaitForComplete() throws InterruptedException {
        getInstrumentation()
                .runOnMainSync(
                        new Runnable() {
                            @Override
                            public void run() {
                                mChannelDataManager.start();
                            }
                        });
        assertThat(mListener.loadFinishedLatch.await(WAIT_TIME_OUT_MS, TimeUnit.MILLISECONDS))
                .isTrue();
    }

    private void restart() throws InterruptedException {
        getInstrumentation()
                .runOnMainSync(
                        new Runnable() {
                            @Override
                            public void run() {
                                mChannelDataManager.stop();
                                mListener.reset();
                            }
                        });
        startAndWaitForComplete();
    }

    @Test
    public void testIsDbLoadFinished() throws InterruptedException {
        startAndWaitForComplete();
        assertThat(mChannelDataManager.isDbLoadFinished()).isTrue();
    }

    /**
     * Test for following methods - {@link ChannelDataManager#getChannelCount} - {@link
     * ChannelDataManager#getChannelList} - {@link ChannelDataManager#getChannel}
     */
    @Test
    public void testGetChannels() throws InterruptedException {
        startAndWaitForComplete();

        // Test {@link ChannelDataManager#getChannelCount}
        assertThat(mChannelDataManager.getChannelCount())
                .isEqualTo(Constants.UNIT_TEST_CHANNEL_COUNT);

        // Test {@link ChannelDataManager#getChannelList}
        List<ChannelInfo> channelInfoList = new ArrayList<>();
        for (int i = 1; i <= Constants.UNIT_TEST_CHANNEL_COUNT; i++) {
            channelInfoList.add(ChannelInfo.create(getTargetContext(), i));
        }
        List<Channel> channelList = mChannelDataManager.getChannelList();
        for (Channel channel : channelList) {
            boolean found = false;
            for (ChannelInfo channelInfo : channelInfoList) {
                if (TextUtils.equals(channelInfo.name, channel.getDisplayName())) {
                    found = true;
                    channelInfoList.remove(channelInfo);
                    break;
                }
            }
            assertWithMessage("Cannot find (" + channel + ")").that(found).isTrue();
        }

        // Test {@link ChannelDataManager#getChannelIndex()}
        for (Channel channel : channelList) {
            assertThat(mChannelDataManager.getChannel(channel.getId())).isEqualTo(channel);
        }
    }

    /** Test for {@link ChannelDataManager#getChannelCount} when no channel is available. */
    @Test
    public void testGetChannels_noChannels() throws InterruptedException {
        mContentProvider.clear();
        startAndWaitForComplete();
        assertThat(mChannelDataManager.getChannelCount()).isEqualTo(0);
    }

    /**
     * Test for following methods and channel listener with notifying change. - {@link
     * ChannelDataManager#updateBrowsable} - {@link ChannelDataManager#applyUpdatedValuesToDb}
     */
    @Test
    public void testBrowsable() throws InterruptedException {
        startAndWaitForComplete();

        // Test if all channels are browsable
        List<Channel> channelList = mChannelDataManager.getChannelList();
        List<Channel> browsableChannelList = mChannelDataManager.getBrowsableChannelList();
        for (Channel browsableChannel : browsableChannelList) {
            boolean found = channelList.remove(browsableChannel);
            assertWithMessage("Cannot find (" + browsableChannel + ")").that(found).isTrue();
        }
        assertThat(channelList).isEmpty();

        // Prepare for next tests.
        channelList = mChannelDataManager.getChannelList();
        TestChannelDataManagerChannelListener channelListener =
                new TestChannelDataManagerChannelListener();
        Channel channel1 = channelList.get(0);
        mChannelDataManager.addChannelListener(channel1.getId(), channelListener);

        // Test {@link ChannelDataManager#updateBrowsable} & notification.
        mChannelDataManager.updateBrowsable(channel1.getId(), false, false);
        assertThat(mListener.channelBrowsableChangedCalled).isTrue();
        assertThat(mChannelDataManager.getBrowsableChannelList()).doesNotContain(channel1);
        MoreAsserts.assertContentsInAnyOrder(channelListener.updatedChannels, channel1);
        channelListener.reset();

        // Test {@link ChannelDataManager#applyUpdatedValuesToDb}
        // Disable the update notification to avoid the unwanted call of "onLoadFinished".
        mContentResolver.mNotifyDisabled = true;
        mChannelDataManager.applyUpdatedValuesToDb();
        restart();
        browsableChannelList = mChannelDataManager.getBrowsableChannelList();
        assertThat(browsableChannelList).hasSize(Constants.UNIT_TEST_CHANNEL_COUNT - 1);
        assertThat(browsableChannelList).doesNotContain(channel1);
    }

    /**
     * Test for following methods and channel listener without notifying change. - {@link
     * ChannelDataManager#updateBrowsable} - {@link ChannelDataManager#applyUpdatedValuesToDb}
     */
    @Test
    public void testBrowsable_skipNotification() throws InterruptedException {
        startAndWaitForComplete();

        List<Channel> channels = mChannelDataManager.getChannelList();
        // Prepare for next tests.
        TestChannelDataManagerChannelListener channelListener =
                new TestChannelDataManagerChannelListener();
        Channel channel1 = channels.get(0);
        Channel channel2 = channels.get(1);
        mChannelDataManager.addChannelListener(channel1.getId(), channelListener);
        mChannelDataManager.addChannelListener(channel2.getId(), channelListener);

        // Test {@link ChannelDataManager#updateBrowsable} & skip notification.
        mChannelDataManager.updateBrowsable(channel1.getId(), false, true);
        mChannelDataManager.updateBrowsable(channel2.getId(), false, true);
        mChannelDataManager.updateBrowsable(channel1.getId(), true, true);
        assertThat(mListener.channelBrowsableChangedCalled).isFalse();
        List<Channel> browsableChannelList = mChannelDataManager.getBrowsableChannelList();
        assertThat(browsableChannelList).contains(channel1);
        assertThat(browsableChannelList).doesNotContain(channel2);

        // Test {@link ChannelDataManager#applyUpdatedValuesToDb}
        // Disable the update notification to avoid the unwanted call of "onLoadFinished".
        mContentResolver.mNotifyDisabled = true;
        mChannelDataManager.applyUpdatedValuesToDb();
        restart();
        browsableChannelList = mChannelDataManager.getBrowsableChannelList();
        assertThat(browsableChannelList).hasSize(Constants.UNIT_TEST_CHANNEL_COUNT - 1);
        assertThat(browsableChannelList).doesNotContain(channel2);
    }

    /**
     * Test for following methods and channel listener. - {@link ChannelDataManager#updateLocked} -
     * {@link ChannelDataManager#applyUpdatedValuesToDb}
     */
    @Test
    public void testLocked() throws InterruptedException {
        startAndWaitForComplete();

        // Test if all channels aren't locked at the first time.
        List<Channel> channelList = mChannelDataManager.getChannelList();
        for (Channel channel : channelList) {
            assertWithMessage(channel + " is locked").that(channel.isLocked()).isFalse();
        }

        // Prepare for next tests.
        Channel channel = mChannelDataManager.getChannelList().get(0);

        // Test {@link ChannelDataManager#updateLocked}
        mChannelDataManager.updateLocked(channel.getId(), true);
        assertThat(mChannelDataManager.getChannel(channel.getId()).isLocked()).isTrue();

        // Test {@link ChannelDataManager#applyUpdatedValuesToDb}.
        // Disable the update notification to avoid the unwanted call of "onLoadFinished".
        mContentResolver.mNotifyDisabled = true;
        mChannelDataManager.applyUpdatedValuesToDb();
        restart();
        assertThat(mChannelDataManager.getChannel(channel.getId()).isLocked()).isTrue();

        // Cleanup
        mChannelDataManager.updateLocked(channel.getId(), false);
    }

    /** Test ChannelDataManager when channels in TvContract are updated, removed, or added. */
    @Test
    public void testChannelListChanged() throws InterruptedException {
        startAndWaitForComplete();

        // Test channel add.
        mListener.reset();
        long testChannelId = Constants.UNIT_TEST_CHANNEL_COUNT + 1;
        ChannelInfo testChannelInfo = ChannelInfo.create(getTargetContext(), (int) testChannelId);
        testChannelId = Constants.UNIT_TEST_CHANNEL_COUNT + 1;
        mContentProvider.simulateInsert(testChannelInfo);
        assertThat(mListener.channelListUpdatedLatch.await(WAIT_TIME_OUT_MS, TimeUnit.MILLISECONDS))
                .isTrue();
        assertThat(mChannelDataManager.getChannelCount())
                .isEqualTo(Constants.UNIT_TEST_CHANNEL_COUNT + 1);

        // Test channel update
        mListener.reset();
        TestChannelDataManagerChannelListener channelListener =
                new TestChannelDataManagerChannelListener();
        mChannelDataManager.addChannelListener(testChannelId, channelListener);
        String newName = testChannelInfo.name + "_test";
        mContentProvider.simulateUpdate(testChannelId, newName);
        assertThat(mListener.channelListUpdatedLatch.await(WAIT_TIME_OUT_MS, TimeUnit.MILLISECONDS))
                .isTrue();
        assertThat(
                        channelListener.channelChangedLatch.await(
                                WAIT_TIME_OUT_MS, TimeUnit.MILLISECONDS))
                .isTrue();
        assertThat(channelListener.removedChannels).isEmpty();
        assertThat(channelListener.updatedChannels).hasSize(1);
        Channel updatedChannel = channelListener.updatedChannels.get(0);
        assertThat(updatedChannel.getId()).isEqualTo(testChannelId);
        assertThat(updatedChannel.getDisplayNumber()).isEqualTo(testChannelInfo.number);
        assertThat(updatedChannel.getDisplayName()).isEqualTo(newName);
        assertThat(mChannelDataManager.getChannelCount())
                .isEqualTo(Constants.UNIT_TEST_CHANNEL_COUNT + 1);

        // Test channel remove.
        mListener.reset();
        channelListener.reset();
        mContentProvider.simulateDelete(testChannelId);
        assertThat(mListener.channelListUpdatedLatch.await(WAIT_TIME_OUT_MS, TimeUnit.MILLISECONDS))
                .isTrue();
        assertThat(
                        channelListener.channelChangedLatch.await(
                                WAIT_TIME_OUT_MS, TimeUnit.MILLISECONDS))
                .isTrue();
        assertThat(channelListener.removedChannels).hasSize(1);
        assertThat(channelListener.updatedChannels).isEmpty();
        Channel removedChannel = channelListener.removedChannels.get(0);
        assertThat(removedChannel.getDisplayName()).isEqualTo(newName);
        assertThat(removedChannel.getDisplayNumber()).isEqualTo(testChannelInfo.number);
        assertThat(mChannelDataManager.getChannelCount())
                .isEqualTo(Constants.UNIT_TEST_CHANNEL_COUNT);
    }

    private static class ChannelInfoWrapper {
        public ChannelInfo channelInfo;
        public boolean browsable;
        public boolean locked;

        public ChannelInfoWrapper(ChannelInfo channelInfo) {
            this.channelInfo = channelInfo;
            browsable = true;
            locked = false;
        }
    }

    private class FakeContentResolver extends MockContentResolver {
        boolean mNotifyDisabled;

        @Override
        public void notifyChange(Uri uri, ContentObserver observer, boolean syncToNetwork) {
            super.notifyChange(uri, observer, syncToNetwork);
            if (DEBUG) {
                Log.d(
                        TAG,
                        "onChanged(uri="
                                + uri
                                + ", observer="
                                + observer
                                + ") - Notification "
                                + (mNotifyDisabled ? "disabled" : "enabled"));
            }
            if (mNotifyDisabled) {
                return;
            }
            // Do not call {@link ContentObserver#onChange} directly to run it on the correct
            // thread.
            if (observer != null) {
                observer.dispatchChange(false, uri);
            } else {
                mChannelDataManager.getContentObserver().dispatchChange(false, uri);
            }
        }
    }

    // This implements the minimal methods in content resolver
    // and detailed assumptions are written in each method.
    private class FakeContentProvider extends MockContentProvider {
        private final SparseArray<ChannelInfoWrapper> mChannelInfoList = new SparseArray<>();

        public FakeContentProvider(Context context) {
            super(context);
            for (int i = 1; i <= Constants.UNIT_TEST_CHANNEL_COUNT; i++) {
                mChannelInfoList.put(
                        i, new ChannelInfoWrapper(ChannelInfo.create(getTargetContext(), i)));
            }
        }

        /**
         * Implementation of {@link ContentProvider#query}. This assumes that {@link
         * ChannelDataManager} queries channels with empty {@code selection}. (i.e. channels are
         * always queries for all)
         */
        @Override
        public Cursor query(
                Uri uri,
                String[] projection,
                String selection,
                String[] selectionArgs,
                String sortOrder) {
            if (DEBUG) {
                Log.d(TAG, "dump query");
                Log.d(TAG, "  uri=" + uri);
                Log.d(TAG, "  projection=" + Arrays.toString(projection));
                Log.d(TAG, "  selection=" + selection);
            }
            assertChannelUri(uri);
            return new FakeCursor(projection);
        }

        /**
         * Implementation of {@link ContentProvider#update}. This assumes that {@link
         * ChannelDataManager} update channels only for changing browsable and locked.
         */
        @Override
        public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
            if (DEBUG) Log.d(TAG, "update(uri=" + uri + ", selection=" + selection);
            assertChannelUri(uri);
            List<Long> channelIds = new ArrayList<>();
            try {
                long channelId = ContentUris.parseId(uri);
                channelIds.add(channelId);
            } catch (NumberFormatException e) {
                // Update for multiple channels.
                if (TextUtils.isEmpty(selection)) {
                    for (int i = 0; i < mChannelInfoList.size(); i++) {
                        channelIds.add((long) mChannelInfoList.keyAt(i));
                    }
                } else {
                    // See {@link Utils#buildSelectionForIds} for the syntax.
                    String selectionForId =
                            selection.substring(
                                    selection.indexOf("(") + 1, selection.lastIndexOf(")"));
                    String[] ids = selectionForId.split(", ");
                    if (ids != null) {
                        for (String id : ids) {
                            channelIds.add(Long.parseLong(id));
                        }
                    }
                }
            }
            int updateCount = 0;
            for (long channelId : channelIds) {
                boolean updated = false;
                ChannelInfoWrapper channel = mChannelInfoList.get((int) channelId);
                if (channel == null) {
                    return 0;
                }
                if (values.containsKey(COLUMN_BROWSABLE)) {
                    updated = true;
                    channel.browsable = (values.getAsInteger(COLUMN_BROWSABLE) == 1);
                }
                if (values.containsKey(COLUMN_LOCKED)) {
                    updated = true;
                    channel.locked = (values.getAsInteger(COLUMN_LOCKED) == 1);
                }
                updateCount += updated ? 1 : 0;
            }
            if (updateCount > 0) {
                if (channelIds.size() == 1) {
                    mContentResolver.notifyChange(uri, null);
                } else {
                    mContentResolver.notifyChange(Channels.CONTENT_URI, null);
                }
            } else {
                if (DEBUG) {
                    Log.d(TAG, "Update to channel(uri=" + uri + ") is ignored for " + values);
                }
            }
            return updateCount;
        }

        /**
         * Simulates channel data insert. This assigns original network ID (the same with channel
         * number) to channel ID.
         */
        public void simulateInsert(ChannelInfo testChannelInfo) {
            long channelId = testChannelInfo.originalNetworkId;
            mChannelInfoList.put(
                    (int) channelId,
                    new ChannelInfoWrapper(
                            ChannelInfo.create(getTargetContext(), (int) channelId)));
            mContentResolver.notifyChange(TvContract.buildChannelUri(channelId), null);
        }

        /** Simulates channel data delete. */
        public void simulateDelete(long channelId) {
            mChannelInfoList.remove((int) channelId);
            mContentResolver.notifyChange(TvContract.buildChannelUri(channelId), null);
        }

        /** Simulates channel data update. */
        public void simulateUpdate(long channelId, String newName) {
            ChannelInfoWrapper channel = mChannelInfoList.get((int) channelId);
            ChannelInfo.Builder builder = new ChannelInfo.Builder(channel.channelInfo);
            builder.setName(newName);
            channel.channelInfo = builder.build();
            mContentResolver.notifyChange(TvContract.buildChannelUri(channelId), null);
        }

        private void assertChannelUri(Uri uri) {
            assertWithMessage("Uri(" + uri + ") isn't channel uri")
                    .that(uri.toString().startsWith(Channels.CONTENT_URI.toString()))
                    .isTrue();
        }

        public void clear() {
            mChannelInfoList.clear();
        }

        public ChannelInfoWrapper get(int position) {
            return mChannelInfoList.get(mChannelInfoList.keyAt(position));
        }

        public int getCount() {
            return mChannelInfoList.size();
        }

        public long keyAt(int position) {
            return mChannelInfoList.keyAt(position);
        }
    }

    private class FakeCursor extends MockCursor {
        private final String[] allColumns = {
            Channels._ID,
            Channels.COLUMN_DISPLAY_NAME,
            Channels.COLUMN_DISPLAY_NUMBER,
            Channels.COLUMN_INPUT_ID,
            Channels.COLUMN_VIDEO_FORMAT,
            Channels.COLUMN_ORIGINAL_NETWORK_ID,
            COLUMN_BROWSABLE,
            COLUMN_LOCKED
        };
        private final String[] mColumns;
        private int mPosition;

        public FakeCursor(String[] columns) {
            mColumns = (columns == null) ? allColumns : columns;
            mPosition = -1;
        }

        @Override
        public String getColumnName(int columnIndex) {
            return mColumns[columnIndex];
        }

        @Override
        public int getColumnIndex(String columnName) {
            for (int i = 0; i < mColumns.length; i++) {
                if (mColumns[i].equalsIgnoreCase(columnName)) {
                    return i;
                }
            }
            return -1;
        }

        @Override
        public long getLong(int columnIndex) {
            String columnName = getColumnName(columnIndex);
            switch (columnName) {
                case Channels._ID:
                    return mContentProvider.keyAt(mPosition);
                default: // fall out
            }
            if (DEBUG) {
                Log.d(TAG, "Column (" + columnName + ") is ignored in getLong()");
            }
            return 0;
        }

        @Override
        public String getString(int columnIndex) {
            String columnName = getColumnName(columnIndex);
            ChannelInfoWrapper channel = mContentProvider.get(mPosition);
            switch (columnName) {
                case Channels.COLUMN_DISPLAY_NAME:
                    return channel.channelInfo.name;
                case Channels.COLUMN_DISPLAY_NUMBER:
                    return channel.channelInfo.number;
                case Channels.COLUMN_INPUT_ID:
                    return DUMMY_INPUT_ID;
                case Channels.COLUMN_VIDEO_FORMAT:
                    return channel.channelInfo.getVideoFormat();
                default: // fall out
            }
            if (DEBUG) {
                Log.d(TAG, "Column (" + columnName + ") is ignored in getString()");
            }
            return null;
        }

        @Override
        public int getInt(int columnIndex) {
            String columnName = getColumnName(columnIndex);
            ChannelInfoWrapper channel = mContentProvider.get(mPosition);
            switch (columnName) {
                case Channels.COLUMN_ORIGINAL_NETWORK_ID:
                    return channel.channelInfo.originalNetworkId;
                case COLUMN_BROWSABLE:
                    return channel.browsable ? 1 : 0;
                case COLUMN_LOCKED:
                    return channel.locked ? 1 : 0;
                default: // fall out
            }
            if (DEBUG) {
                Log.d(TAG, "Column (" + columnName + ") is ignored in getInt()");
            }
            return 0;
        }

        @Override
        public int getCount() {
            return mContentProvider.getCount();
        }

        @Override
        public boolean moveToNext() {
            return ++mPosition < mContentProvider.getCount();
        }

        @Override
        public void close() {
            // No-op.
        }
    }

    private static class TestChannelDataManagerListener implements ChannelDataManager.Listener {
        public CountDownLatch loadFinishedLatch = new CountDownLatch(1);
        public CountDownLatch channelListUpdatedLatch = new CountDownLatch(1);
        public boolean channelBrowsableChangedCalled;

        @Override
        public void onLoadFinished() {
            loadFinishedLatch.countDown();
        }

        @Override
        public void onChannelListUpdated() {
            channelListUpdatedLatch.countDown();
        }

        @Override
        public void onChannelBrowsableChanged() {
            channelBrowsableChangedCalled = true;
        }

        public void reset() {
            loadFinishedLatch = new CountDownLatch(1);
            channelListUpdatedLatch = new CountDownLatch(1);
            channelBrowsableChangedCalled = false;
        }
    }

    private static class TestChannelDataManagerChannelListener
            implements ChannelDataManager.ChannelListener {
        public CountDownLatch channelChangedLatch = new CountDownLatch(1);
        public final List<Channel> removedChannels = new ArrayList<>();
        public final List<Channel> updatedChannels = new ArrayList<>();

        @Override
        public void onChannelRemoved(Channel channel) {
            removedChannels.add(channel);
            channelChangedLatch.countDown();
        }

        @Override
        public void onChannelUpdated(Channel channel) {
            updatedChannels.add(channel);
            channelChangedLatch.countDown();
        }

        public void reset() {
            channelChangedLatch = new CountDownLatch(1);
            removedChannels.clear();
            updatedChannels.clear();
        }
    }
}
