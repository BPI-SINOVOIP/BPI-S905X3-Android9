/*
 * Copyright 2017 The Android Open Source Project
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

import static org.mockito.Mockito.*;

import android.media.MediaDescription;
import android.media.MediaMetadata;
import android.media.session.MediaSession;
import android.media.session.PlaybackState;
import android.os.HandlerThread;
import android.os.TestLooperManager;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class MediaPlayerWrapperTest {
    private static final int MSG_TIMEOUT = 0;

    private HandlerThread mThread;
    private MediaMetadata.Builder mTestMetadata;
    private ArrayList<MediaDescription.Builder> mTestQueue;
    private PlaybackState.Builder mTestState;

    @Captor ArgumentCaptor<MediaController.Callback> mControllerCbs;
    @Captor ArgumentCaptor<MediaData> mMediaUpdateData;
    @Mock Log.TerribleFailureHandler mFailHandler;
    @Mock MediaController mMockController;
    @Mock MediaPlayerWrapper.Callback mTestCbs;

    List<MediaSession.QueueItem> getQueueFromDescriptions(
            List<MediaDescription.Builder> descriptions) {
        ArrayList<MediaSession.QueueItem> newList = new ArrayList<MediaSession.QueueItem>();

        for (MediaDescription.Builder bob : descriptions) {
            newList.add(
                    new MediaSession.QueueItem(
                            bob.build(), Long.valueOf(bob.build().getMediaId())));
        }

        return newList;
    }

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        // Set failure handler to capture Log.wtf messages
        Log.setWtfHandler(mFailHandler);

        // Set up Looper thread for the timeout handler
        mThread = new HandlerThread("MediaPlayerWrapperTestThread");
        mThread.start();

        // Set up new metadata that can be used in each test
        mTestMetadata =
                new MediaMetadata.Builder()
                        .putString(MediaMetadata.METADATA_KEY_TITLE, "BT Test Song")
                        .putString(MediaMetadata.METADATA_KEY_ARTIST, "BT Test Artist")
                        .putString(MediaMetadata.METADATA_KEY_ALBUM, "BT Test Album")
                        .putLong(MediaMetadata.METADATA_KEY_DURATION, 5000L);

        mTestState =
                new PlaybackState.Builder()
                        .setActiveQueueItemId(100)
                        .setState(PlaybackState.STATE_PAUSED, 0, 1.0f);

        mTestQueue = new ArrayList<MediaDescription.Builder>();
        mTestQueue.add(
                new MediaDescription.Builder()
                        .setTitle("BT Test Song")
                        .setSubtitle("BT Test Artist")
                        .setDescription("BT Test Album")
                        .setMediaId("100"));
        mTestQueue.add(
                new MediaDescription.Builder()
                        .setTitle("BT Test Song 2")
                        .setSubtitle("BT Test Artist 2")
                        .setDescription("BT Test Album 2")
                        .setMediaId("101"));
        mTestQueue.add(
                new MediaDescription.Builder()
                        .setTitle("BT Test Song 3")
                        .setSubtitle("BT Test Artist 3")
                        .setDescription("BT Test Album 3")
                        .setMediaId("102"));

        when(mMockController.getPackageName()).thenReturn("mMockController");
        // NOTE: We use doReturn below because using the normal stubbing method
        // doesn't immediately update the stub with the new return value and this
        // can cause the old stub to be used.

        // Stub default metadata for the Media Controller
        doReturn(mTestMetadata.build()).when(mMockController).getMetadata();
        doReturn(mTestState.build()).when(mMockController).getPlaybackState();
        doReturn(getQueueFromDescriptions(mTestQueue)).when(mMockController).getQueue();

        // Enable testing flag which enables Log.wtf statements. Some tests test against improper
        // behaviour and the TerribleFailureListener is a good way to ensure that the error occurred
        MediaPlayerWrapper.sTesting = true;
    }

    /*
     * Test to make sure that the wrapper fails to be built if passed invalid
     * data.
     */
    @Test
    public void testNullControllerLooper() {
        MediaPlayerWrapper wrapper = MediaPlayerWrapper.wrap(null, mThread.getLooper());
        Assert.assertNull(wrapper);

        wrapper = MediaPlayerWrapper.wrap(mMockController, null);
        Assert.assertNull(wrapper);
    }

    /*
     * Test to make sure that isReady() returns false if there is no playback state,
     * there is no metadata, or if the metadata has no title.
     */
    @Test
    public void testIsReady() {
        MediaPlayerWrapper wrapper = MediaPlayerWrapper.wrap(mMockController, mThread.getLooper());
        Assert.assertTrue(wrapper.isReady());

        // Test isReady() is false when the playback state is null
        doReturn(null).when(mMockController).getPlaybackState();
        Assert.assertFalse(wrapper.isReady());

        // Restore the old playback state
        doReturn(mTestState.build()).when(mMockController).getPlaybackState();
        Assert.assertTrue(wrapper.isReady());

        // Test isReady() is false when the metadata is null
        doReturn(null).when(mMockController).getMetadata();
        Assert.assertFalse(wrapper.isReady());

        // Restore the old metadata
        doReturn(mTestMetadata.build()).when(mMockController).getMetadata();
        Assert.assertTrue(wrapper.isReady());
    }

    /*
     * Test to make sure that if a new controller is registered with different metadata than the
     * previous controller, the new metadata is pulled upon registration.
     */
    @Test
    public void testControllerUpdate() {
        // Create the wrapper object and register the looper with the timeout handler
        MediaPlayerWrapper wrapper = MediaPlayerWrapper.wrap(mMockController, mThread.getLooper());
        Assert.assertTrue(wrapper.isReady());
        wrapper.registerCallback(mTestCbs);

        // Create a new MediaController that has different metadata than the previous controller
        MediaController mUpdatedController = mock(MediaController.class);
        doReturn(mTestState.build()).when(mUpdatedController).getPlaybackState();
        mTestMetadata.putString(MediaMetadata.METADATA_KEY_TITLE, "New Title");
        doReturn(mTestMetadata.build()).when(mUpdatedController).getMetadata();
        doReturn(null).when(mMockController).getQueue();

        // Update the wrappers controller to the new controller
        wrapper.updateMediaController(mUpdatedController);

        // Send a metadata update with the same data that the controller had upon registering
        verify(mUpdatedController).registerCallback(mControllerCbs.capture(), any());
        MediaController.Callback controllerCallbacks = mControllerCbs.getValue();
        controllerCallbacks.onMetadataChanged(mTestMetadata.build());

        // Verify that a callback was never called since no data was updated
        verify(mTestCbs, never()).mediaUpdatedCallback(any());
    }

    /*
     * Test to make sure that a media player update gets sent whenever a Media metadata or playback
     * state change occurs instead of waiting for all data to be synced if the player doesn't
     * support queues.
     */
    @Test
    public void testNoQueueMediaUpdates() {
        // Create the wrapper object and register the looper with the timeout handler
        TestLooperManager looperManager = new TestLooperManager(mThread.getLooper());
        MediaPlayerWrapper wrapper =
                MediaPlayerWrapper.wrap(mMockController, mThread.getLooper());
        wrapper.registerCallback(mTestCbs);

        // Return null when getting the queue
        doReturn(null).when(mMockController).getQueue();

        // Grab the callbacks the wrapper registered with the controller
        verify(mMockController).registerCallback(mControllerCbs.capture(), any());
        MediaController.Callback controllerCallbacks = mControllerCbs.getValue();

        // Update Metdata returned by controller
        mTestMetadata.putString(MediaMetadata.METADATA_KEY_TITLE, "New Title");
        doReturn(mTestMetadata.build()).when(mMockController).getMetadata();
        controllerCallbacks.onMetadataChanged(mTestMetadata.build());

        // Assert that the metadata was updated and playback state wasn't
        verify(mTestCbs, times(1)).mediaUpdatedCallback(mMediaUpdateData.capture());
        MediaData data = mMediaUpdateData.getValue();
        Assert.assertEquals(
                "Returned Metadata isn't equal to given Metadata",
                data.metadata,
                Util.toMetadata(mTestMetadata.build()));
        Assert.assertEquals(
                "Returned PlaybackState isn't equal to original PlaybackState",
                data.state.toString(),
                mTestState.build().toString());
        Assert.assertEquals("Returned Queue isn't empty", data.queue.size(), 0);

        // Update PlaybackState returned by controller
        mTestState.setActiveQueueItemId(103);
        doReturn(mTestState.build()).when(mMockController).getPlaybackState();
        controllerCallbacks.onPlaybackStateChanged(mTestState.build());

        // Assert that the PlaybackState was changed but metadata stayed the same
        verify(mTestCbs, times(2)).mediaUpdatedCallback(mMediaUpdateData.capture());
        data = mMediaUpdateData.getValue();
        Assert.assertEquals(
                "Returned PlaybackState isn't equal to given PlaybackState",
                data.state.toString(),
                mTestState.build().toString());
        Assert.assertEquals(
                "Returned Metadata isn't equal to given Metadata",
                data.metadata,
                Util.toMetadata(mTestMetadata.build()));
        Assert.assertEquals("Returned Queue isn't empty", data.queue.size(), 0);

        // Verify that there are no timeout messages pending and there were no timeouts
        Assert.assertFalse(wrapper.getTimeoutHandler().hasMessages(MSG_TIMEOUT));
        verify(mFailHandler, never()).onTerribleFailure(any(), any(), anyBoolean());
    }

    /*
     * This test updates the metadata and playback state returned by the
     * controller then sends an update. This is to make sure that all relevant
     * information is sent with every update. In the case without a queue,
     * metadata and playback state are updated.
     */
    @Test
    public void testDataOnUpdateNoQueue() {
        // Create the wrapper object and register the looper with the timeout handler
        TestLooperManager looperManager = new TestLooperManager(mThread.getLooper());
        MediaPlayerWrapper wrapper =
                MediaPlayerWrapper.wrap(mMockController, mThread.getLooper());
        wrapper.registerCallback(mTestCbs);

        // Return null when getting the queue
        doReturn(null).when(mMockController).getQueue();

        // Grab the callbacks the wrapper registered with the controller
        verify(mMockController).registerCallback(mControllerCbs.capture(), any());
        MediaController.Callback controllerCallbacks = mControllerCbs.getValue();

        // Update Metadata returned by controller
        mTestMetadata.putString(MediaMetadata.METADATA_KEY_TITLE, "New Title");
        doReturn(mTestMetadata.build()).when(mMockController).getMetadata();

        // Update PlaybackState returned by controller
        mTestState.setActiveQueueItemId(103);
        doReturn(mTestState.build()).when(mMockController).getPlaybackState();

        // Call the callback
        controllerCallbacks.onPlaybackStateChanged(mTestState.build());

        // Assert that both metadata and playback state are there.
        verify(mTestCbs, times(1)).mediaUpdatedCallback(mMediaUpdateData.capture());
        MediaData data = mMediaUpdateData.getValue();
        Assert.assertEquals(
                "Returned PlaybackState isn't equal to given PlaybackState",
                data.state.toString(),
                mTestState.build().toString());
        Assert.assertEquals(
                "Returned Metadata isn't equal to given Metadata",
                data.metadata,
                Util.toMetadata(mTestMetadata.build()));
        Assert.assertEquals("Returned Queue isn't empty", data.queue.size(), 0);

        // Verify that there are no timeout messages pending and there were no timeouts
        Assert.assertFalse(wrapper.getTimeoutHandler().hasMessages(MSG_TIMEOUT));
        verify(mFailHandler, never()).onTerribleFailure(any(), any(), anyBoolean());
    }

    /*
     * This test checks whether getCurrentMetadata() returns the corresponding item from
     * the now playing list instead of the current metadata if there is a match.
     */
    @Test
    public void testCurrentSongFromQueue() {
        // Create the wrapper object and register the looper with the timeout handler
        TestLooperManager looperManager = new TestLooperManager(mThread.getLooper());

        mTestState.setActiveQueueItemId(101);
        doReturn(mTestState.build()).when(mMockController).getPlaybackState();

        MediaPlayerWrapper wrapper =
                MediaPlayerWrapper.wrap(mMockController, mThread.getLooper());
        wrapper.registerCallback(mTestCbs);

        // The current metadata doesn't contain track number info so check that
        // field to see if the correct data was used.
        Assert.assertEquals(wrapper.getCurrentMetadata().trackNum, "2");
        Assert.assertEquals(wrapper.getCurrentMetadata().numTracks, "3");
    }

    @Test
    public void testNullMetadata() {
        // Create the wrapper object and register the looper with the timeout handler
        TestLooperManager looperManager = new TestLooperManager(mThread.getLooper());
        MediaPlayerWrapper wrapper =
                MediaPlayerWrapper.wrap(mMockController, mThread.getLooper());
        wrapper.registerCallback(mTestCbs);

        // Return null when getting the queue
        doReturn(null).when(mMockController).getQueue();

        // Grab the callbacks the wrapper registered with the controller
        verify(mMockController).registerCallback(mControllerCbs.capture(), any());
        MediaController.Callback controllerCallbacks = mControllerCbs.getValue();

        // Update Metadata returned by controller
        mTestMetadata.putString(MediaMetadata.METADATA_KEY_TITLE, "New Title");
        doReturn(mTestMetadata.build()).when(mMockController).getMetadata();

        // Call the callback
        controllerCallbacks.onMetadataChanged(null);

        // Assert that the metadata returned by getMetadata() is used instead of null
        verify(mTestCbs, times(1)).mediaUpdatedCallback(mMediaUpdateData.capture());
        MediaData data = mMediaUpdateData.getValue();
        Assert.assertEquals("Returned metadata is incorrect", data.metadata,
                Util.toMetadata(mTestMetadata.build()));
    }

    @Test
    public void testNullQueue() {
        // Create the wrapper object and register the looper with the timeout handler
        TestLooperManager looperManager = new TestLooperManager(mThread.getLooper());
        MediaPlayerWrapper wrapper =
                MediaPlayerWrapper.wrap(mMockController, mThread.getLooper());
        wrapper.registerCallback(mTestCbs);

        // Return null when getting the queue
        doReturn(null).when(mMockController).getQueue();

        // Grab the callbacks the wrapper registered with the controller
        verify(mMockController).registerCallback(mControllerCbs.capture(), any());
        MediaController.Callback controllerCallbacks = mControllerCbs.getValue();

        // Call the callback
        controllerCallbacks.onQueueChanged(null);

        // Assert that both metadata and playback state are there.
        verify(mTestCbs, times(1)).mediaUpdatedCallback(mMediaUpdateData.capture());
        MediaData data = mMediaUpdateData.getValue();
        Assert.assertEquals("Returned Queue isn't null", data.queue.size(), 0);
    }

    /*
     * This test checks to see if the now playing queue data is cached.
     */
    @Test
    public void testQueueCached() {
        // Create the wrapper object and register the looper with the timeout handler
        TestLooperManager looperManager = new TestLooperManager(mThread.getLooper());
        MediaPlayerWrapper wrapper =
                MediaPlayerWrapper.wrap(mMockController, mThread.getLooper());
        wrapper.registerCallback(mTestCbs);

        // Call getCurrentQueue() multiple times.
        for (int i = 0; i < 3; i++) {
            Assert.assertEquals(wrapper.getCurrentQueue(),
                    Util.toMetadataList(getQueueFromDescriptions(mTestQueue)));
        }

        // Verify that getQueue() was only called twice. Once on creation and once during
        // registration
        verify(mMockController, times(2)).getQueue();
    }

    /*
     * This test sends repeated Playback State updates that only have a short
     * position update change to see if they get debounced.
     */
    @Test
    public void testPlaybackStateUpdateSpam() {
        // Create the wrapper object and register the looper with the timeout handler
        TestLooperManager looperManager = new TestLooperManager(mThread.getLooper());
        MediaPlayerWrapper wrapper =
                MediaPlayerWrapper.wrap(mMockController, mThread.getLooper());
        wrapper.registerCallback(mTestCbs);

        // Return null when getting the queue
        doReturn(null).when(mMockController).getQueue();

        // Grab the callbacks the wrapper registered with the controller
        verify(mMockController).registerCallback(mControllerCbs.capture(), any());
        MediaController.Callback controllerCallbacks = mControllerCbs.getValue();

        // Update PlaybackState returned by controller (Should trigger update)
        mTestState.setState(PlaybackState.STATE_PLAYING, 1000, 1.0f);
        doReturn(mTestState.build()).when(mMockController).getPlaybackState();
        controllerCallbacks.onPlaybackStateChanged(mTestState.build());

        // Assert that both metadata and only the first playback state is there.
        verify(mTestCbs, times(1)).mediaUpdatedCallback(mMediaUpdateData.capture());
        MediaData data = mMediaUpdateData.getValue();
        Assert.assertEquals(
                "Returned PlaybackState isn't equal to given PlaybackState",
                data.state.toString(),
                mTestState.build().toString());
        Assert.assertEquals(
                "Returned Metadata isn't equal to given Metadata",
                data.metadata,
                Util.toMetadata(mTestMetadata.build()));
        Assert.assertEquals("Returned Queue isn't empty", data.queue.size(), 0);

        // Update PlaybackState returned by controller (Shouldn't trigger update)
        mTestState.setState(PlaybackState.STATE_PLAYING, 1020, 1.0f);
        doReturn(mTestState.build()).when(mMockController).getPlaybackState();
        controllerCallbacks.onPlaybackStateChanged(mTestState.build());

        // Update PlaybackState returned by controller (Shouldn't trigger update)
        mTestState.setState(PlaybackState.STATE_PLAYING, 1040, 1.0f);
        doReturn(mTestState.build()).when(mMockController).getPlaybackState();
        controllerCallbacks.onPlaybackStateChanged(mTestState.build());

        // Update PlaybackState returned by controller (Should trigger update)
        mTestState.setState(PlaybackState.STATE_PLAYING, 3000, 1.0f);
        doReturn(mTestState.build()).when(mMockController).getPlaybackState();
        controllerCallbacks.onPlaybackStateChanged(mTestState.build());


        // Verify that there are no timeout messages pending and there were no timeouts
        Assert.assertFalse(wrapper.getTimeoutHandler().hasMessages(MSG_TIMEOUT));
        verify(mFailHandler, never()).onTerribleFailure(any(), any(), anyBoolean());
    }

    /*
     * Check to make sure that cleanup tears down the object properly
     */
    @Test
    public void testWrapperCleanup() {
        // Create the wrapper object and register the looper with the timeout handler
        TestLooperManager looperManager = new TestLooperManager(mThread.getLooper());
        MediaPlayerWrapper wrapper =
                MediaPlayerWrapper.wrap(mMockController, mThread.getLooper());
        wrapper.registerCallback(mTestCbs);

        // Cleanup the wrapper
        wrapper.cleanup();

        // Ensure that everything was cleaned up
        verify(mMockController).unregisterCallback(any());
        Assert.assertNull(wrapper.getTimeoutHandler());
    }

    /*
     * Test to check that a PlaybackState of none is being ignored as that usually means that the
     * MediaController isn't ready.
     */
    @Test
    public void testIgnorePlaystateNone() {
        // Create the wrapper object and register the looper with the timeout handler
        TestLooperManager looperManager = new TestLooperManager(mThread.getLooper());
        MediaPlayerWrapper wrapper =
                MediaPlayerWrapper.wrap(mMockController, mThread.getLooper());
        wrapper.registerCallback(mTestCbs);

        // Grab the callbacks the wrapper registered with the controller
        verify(mMockController).registerCallback(mControllerCbs.capture(), any());
        MediaController.Callback controllerCallbacks = mControllerCbs.getValue();

        // Update PlaybackState returned by controller
        mTestState.setState(PlaybackState.STATE_NONE, 0, 1.0f);
        doReturn(mTestState.build()).when(mMockController).getPlaybackState();
        controllerCallbacks.onPlaybackStateChanged(mTestState.build());

        // Verify that there was no update
        verify(mTestCbs, never()).mediaUpdatedCallback(any());

        // Verify that there are no timeout messages pending and there were no timeouts
        Assert.assertFalse(wrapper.getTimeoutHandler().hasMessages(MSG_TIMEOUT));
        verify(mFailHandler, never()).onTerribleFailure(any(), any(), anyBoolean());
    }

    /*
     * Test to make sure that on a controller that supports browsing, the
     * Media Metadata, Queue, and Playback state all have to be in sync in
     * order for a media update to be sent via registered callback.
     */
    @Test
    public void testMetadataSync() {
        // Create the wrapper object and register the looper with the timeout handler
        TestLooperManager looperManager = new TestLooperManager(mThread.getLooper());
        MediaPlayerWrapper wrapper =
                MediaPlayerWrapper.wrap(mMockController, mThread.getLooper());
        wrapper.registerCallback(mTestCbs);

        // Grab the callbacks the wrapper registered with the controller
        verify(mMockController).registerCallback(mControllerCbs.capture(), any());
        MediaController.Callback controllerCallbacks = mControllerCbs.getValue();

        // Update Metadata returned by controller
        mTestMetadata.putString(MediaMetadata.METADATA_KEY_TITLE, "New Title");
        doReturn(mTestMetadata.build()).when(mMockController).getMetadata();
        controllerCallbacks.onMetadataChanged(mTestMetadata.build());

        // Update PlaybackState returned by controller
        mTestState.setActiveQueueItemId(103);
        doReturn(mTestState.build()).when(mMockController).getPlaybackState();
        controllerCallbacks.onPlaybackStateChanged(mTestState.build());

        // Update Queue returned by controller
        mTestQueue.add(
                new MediaDescription.Builder()
                        .setTitle("New Title")
                        .setSubtitle("BT Test Artist")
                        .setDescription("BT Test Album")
                        .setMediaId("103"));
        doReturn(getQueueFromDescriptions(mTestQueue)).when(mMockController).getQueue();
        controllerCallbacks.onQueueChanged(getQueueFromDescriptions(mTestQueue));

        // Assert that the callback was called with the updated data
        verify(mTestCbs, times(1)).mediaUpdatedCallback(mMediaUpdateData.capture());
        verify(mFailHandler, never()).onTerribleFailure(any(), any(), anyBoolean());
        MediaData data = mMediaUpdateData.getValue();
        Assert.assertEquals(
                "Returned Metadata isn't equal to given Metadata",
                data.metadata,
                Util.toMetadata(mTestMetadata.build()));
        Assert.assertEquals(
                "Returned PlaybackState isn't equal to given PlaybackState",
                data.state.toString(),
                mTestState.build().toString());
        Assert.assertEquals(
                "Returned Queue isn't equal to given Queue",
                data.queue,
                Util.toMetadataList(getQueueFromDescriptions(mTestQueue)));

        // Verify that there are no timeout messages pending and there were no timeouts
        Assert.assertFalse(wrapper.getTimeoutHandler().hasMessages(MSG_TIMEOUT));
        verify(mFailHandler, never()).onTerribleFailure(any(), any(), anyBoolean());
    }

    /*
     * Test to make sure that an error occurs when the MediaController fails to
     * update all its media data in a resonable amount of time.
     */
    @Test
    public void testMetadataSyncFail() {
        // Create the wrapper object and register the looper with the timeout handler
        TestLooperManager looperManager =
                InstrumentationRegistry.getInstrumentation()
                        .acquireLooperManager(mThread.getLooper());
        MediaPlayerWrapper wrapper =
                MediaPlayerWrapper.wrap(mMockController, mThread.getLooper());
        wrapper.registerCallback(mTestCbs);

        // Grab the callbacks the wrapper registered with the controller
        verify(mMockController).registerCallback(mControllerCbs.capture(), any());
        MediaController.Callback controllerCallbacks = mControllerCbs.getValue();

        // Update Metadata returned by controller
        mTestMetadata.putString(MediaMetadata.METADATA_KEY_TITLE, "Mismatch Title");
        doReturn(mTestMetadata.build()).when(mMockController).getMetadata();
        controllerCallbacks.onMetadataChanged(mTestMetadata.build());

        // Force the timeout to execute immediately
        looperManager.execute(looperManager.next());

        // Assert that there was a timeout
        verify(mFailHandler).onTerribleFailure(any(), any(), anyBoolean());

        // Assert that the callback was called with the mismatch data
        verify(mTestCbs, times(1)).mediaUpdatedCallback(mMediaUpdateData.capture());
        MediaData data = mMediaUpdateData.getValue();
        Assert.assertEquals(
                "Returned Metadata isn't equal to given Metadata",
                data.metadata,
                Util.toMetadata(mTestMetadata.build()));
        Assert.assertEquals(
                "Returned PlaybackState isn't equal to given PlaybackState",
                data.state.toString(),
                mTestState.build().toString());
        Assert.assertEquals(
                "Returned Queue isn't equal to given Queue",
                data.queue,
                Util.toMetadataList(getQueueFromDescriptions(mTestQueue)));
    }

    /*
     * testMetadataSyncFuzz() tests for the same conditions as testMetadataSync()
     * but randomizes the order in which the MediaController update callbacks are
     * called. The test is repeated 100 times for completeness.
     */
    @Test
    public void testMetadataSyncFuzz() {
        // The number of times the random order test is run
        final int numTestLoops = 100;

        // Create the wrapper object and register the looper with the timeout handler
        TestLooperManager looperManager =
                InstrumentationRegistry.getInstrumentation()
                        .acquireLooperManager(mThread.getLooper());
        MediaPlayerWrapper wrapper =
                MediaPlayerWrapper.wrap(mMockController, mThread.getLooper());
        wrapper.registerCallback(mTestCbs);

        // Grab the callbacks the wrapper registered with the controller
        verify(mMockController).registerCallback(mControllerCbs.capture(), any());
        MediaController.Callback controllerCallbacks = mControllerCbs.getValue();

        MediaMetadata.Builder m = new MediaMetadata.Builder();
        PlaybackState.Builder s = new PlaybackState.Builder();
        s.setState(PlaybackState.STATE_PAUSED, 0, 1.0f);
        MediaDescription.Builder d = new MediaDescription.Builder();
        for (int i = 1; i <= numTestLoops; i++) {
            // Setup Media Info for current itteration
            m.putString(MediaMetadata.METADATA_KEY_TITLE, "BT Fuzz Song " + i);
            m.putString(MediaMetadata.METADATA_KEY_ARTIST, "BT Fuzz Artist " + i);
            m.putString(MediaMetadata.METADATA_KEY_ALBUM, "BT Fuzz Album " + i);
            m.putLong(MediaMetadata.METADATA_KEY_DURATION, 5000L);
            s.setActiveQueueItemId(i);
            d.setTitle("BT Fuzz Song " + i);
            d.setSubtitle("BT Fuzz Artist " + i);
            d.setDescription("BT Fuzz Album " + i);
            d.setMediaId(Integer.toString(i));

            // Create a new Queue each time to prevent double counting caused by
            // Playback State matching the updated Queue
            ArrayList<MediaSession.QueueItem> q = new ArrayList<MediaSession.QueueItem>();
            q.add(new MediaSession.QueueItem(d.build(), i));

            // Call the MediaController callbacks in a random order
            ArrayList<Integer> callbackOrder = new ArrayList<>(Arrays.asList(0, 1, 2));
            Collections.shuffle(callbackOrder);
            for (int j = 0; j < 3; j++) {
                switch (callbackOrder.get(j)) {
                    case 0: // Update Metadata
                        doReturn(m.build()).when(mMockController).getMetadata();
                        controllerCallbacks.onMetadataChanged(m.build());
                        break;
                    case 1: // Update PlaybackState
                        doReturn(s.build()).when(mMockController).getPlaybackState();
                        controllerCallbacks.onPlaybackStateChanged(s.build());
                        break;
                    case 2: // Update Queue
                        doReturn(q).when(mMockController).getQueue();
                        controllerCallbacks.onQueueChanged(q);
                        break;
                }
            }

            // Check that the callback was called a certain number of times and
            // that all the Media info matches what was given
            verify(mTestCbs, times(i)).mediaUpdatedCallback(mMediaUpdateData.capture());
            MediaData data = mMediaUpdateData.getValue();
            Assert.assertEquals(
                    "Returned Metadata isn't equal to given Metadata",
                    data.metadata,
                    Util.toMetadata(m.build()));
            Assert.assertEquals(
                    "Returned PlaybackState isn't equal to given PlaybackState",
                    data.state.toString(),
                    s.build().toString());
            Assert.assertEquals("Returned Queue isn't equal to given Queue",
                    data.queue,
                    Util.toMetadataList(q));
        }

        // Verify that there are no timeout messages pending and there were no timeouts
        Assert.assertFalse(wrapper.getTimeoutHandler().hasMessages(MSG_TIMEOUT));
        verify(mFailHandler, never()).onTerribleFailure(any(), any(), anyBoolean());
    }
}
