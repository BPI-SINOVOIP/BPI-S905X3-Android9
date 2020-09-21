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

import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;

import android.content.Context;
import android.media.MediaController2;
import android.media.MediaController2.ControllerCallback;
import android.media.MediaItem2;
import android.media.MediaMetadata2;
import android.media.SessionCommand2;
import android.media.MediaSession2.CommandButton;
import android.media.SessionCommandGroup2;
import android.media.SessionToken2;
import android.os.Bundle;
import android.os.HandlerThread;
import android.os.ResultReceiver;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.support.test.InstrumentationRegistry;

import org.junit.AfterClass;
import org.junit.BeforeClass;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.TimeUnit;

/**
 * Base class for session test.
 */
abstract class MediaSession2TestBase {
    // Expected success
    static final int WAIT_TIME_MS = 1000;

    // Expected timeout
    static final int TIMEOUT_MS = 500;

    static TestUtils.SyncHandler sHandler;
    static Executor sHandlerExecutor;

    Context mContext;
    private List<MediaController2> mControllers = new ArrayList<>();

    interface TestControllerInterface {
        ControllerCallback getCallback();
    }

    interface WaitForConnectionInterface {
        void waitForConnect(boolean expect) throws InterruptedException;
        void waitForDisconnect(boolean expect) throws InterruptedException;
    }

    @BeforeClass
    public static void setUpThread() {
        if (sHandler == null) {
            HandlerThread handlerThread = new HandlerThread("MediaSession2TestBase");
            handlerThread.start();
            sHandler = new TestUtils.SyncHandler(handlerThread.getLooper());
            sHandlerExecutor = (runnable) -> {
                sHandler.post(runnable);
            };
        }
    }

    @AfterClass
    public static void cleanUpThread() {
        if (sHandler != null) {
            sHandler.getLooper().quitSafely();
            sHandler = null;
            sHandlerExecutor = null;
        }
    }

    @CallSuper
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getTargetContext();
    }

    @CallSuper
    public void cleanUp() throws Exception {
        for (int i = 0; i < mControllers.size(); i++) {
            mControllers.get(i).close();
        }
    }

    final MediaController2 createController(SessionToken2 token) throws InterruptedException {
        return createController(token, true, null);
    }

    final MediaController2 createController(@NonNull SessionToken2 token,
            boolean waitForConnect, @Nullable ControllerCallback callback)
            throws InterruptedException {
        TestControllerInterface instance = onCreateController(token, callback);
        if (!(instance instanceof MediaController2)) {
            throw new RuntimeException("Test has a bug. Expected MediaController2 but returned "
                    + instance);
        }
        MediaController2 controller = (MediaController2) instance;
        mControllers.add(controller);
        if (waitForConnect) {
            waitForConnect(controller, true);
        }
        return controller;
    }

    private static WaitForConnectionInterface getWaitForConnectionInterface(
            MediaController2 controller) {
        if (!(controller instanceof TestControllerInterface)) {
            throw new RuntimeException("Test has a bug. Expected controller implemented"
                    + " TestControllerInterface but got " + controller);
        }
        ControllerCallback callback = ((TestControllerInterface) controller).getCallback();
        if (!(callback instanceof WaitForConnectionInterface)) {
            throw new RuntimeException("Test has a bug. Expected controller with callback "
                    + " implemented WaitForConnectionInterface but got " + controller);
        }
        return (WaitForConnectionInterface) callback;
    }

    public static void waitForConnect(MediaController2 controller, boolean expected)
            throws InterruptedException {
        getWaitForConnectionInterface(controller).waitForConnect(expected);
    }

    public static void waitForDisconnect(MediaController2 controller, boolean expected)
            throws InterruptedException {
        getWaitForConnectionInterface(controller).waitForDisconnect(expected);
    }

    TestControllerInterface onCreateController(@NonNull SessionToken2 token,
            @Nullable ControllerCallback callback) {
        if (callback == null) {
            callback = new ControllerCallback() {};
        }
        return new TestMediaController(mContext, token, new TestControllerCallback(callback));
    }

    // TODO(jaewan): (Can be Post-P): Deprecate this
    public static class TestControllerCallback extends MediaController2.ControllerCallback
            implements WaitForConnectionInterface {
        public final ControllerCallback mCallbackProxy;
        public final CountDownLatch connectLatch = new CountDownLatch(1);
        public final CountDownLatch disconnectLatch = new CountDownLatch(1);

        TestControllerCallback(@NonNull ControllerCallback callbackProxy) {
            if (callbackProxy == null) {
                throw new IllegalArgumentException("Callback proxy shouldn't be null. Test bug");
            }
            mCallbackProxy = callbackProxy;
        }

        @CallSuper
        @Override
        public void onConnected(MediaController2 controller, SessionCommandGroup2 commands) {
            connectLatch.countDown();
        }

        @CallSuper
        @Override
        public void onDisconnected(MediaController2 controller) {
            disconnectLatch.countDown();
        }

        @Override
        public void waitForConnect(boolean expect) throws InterruptedException {
            if (expect) {
                assertTrue(connectLatch.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS));
            } else {
                assertFalse(connectLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
            }
        }

        @Override
        public void waitForDisconnect(boolean expect) throws InterruptedException {
            if (expect) {
                assertTrue(disconnectLatch.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS));
            } else {
                assertFalse(disconnectLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
            }
        }

        @Override
        public void onCustomCommand(MediaController2 controller, SessionCommand2 command,
                Bundle args, ResultReceiver receiver) {
            mCallbackProxy.onCustomCommand(controller, command, args, receiver);
        }

        @Override
        public void onPlaybackInfoChanged(MediaController2 controller,
                MediaController2.PlaybackInfo info) {
            mCallbackProxy.onPlaybackInfoChanged(controller, info);
        }

        @Override
        public void onCustomLayoutChanged(MediaController2 controller, List<CommandButton> layout) {
            mCallbackProxy.onCustomLayoutChanged(controller, layout);
        }

        @Override
        public void onAllowedCommandsChanged(MediaController2 controller,
                SessionCommandGroup2 commands) {
            mCallbackProxy.onAllowedCommandsChanged(controller, commands);
        }

        @Override
        public void onPlayerStateChanged(MediaController2 controller, int state) {
            mCallbackProxy.onPlayerStateChanged(controller, state);
        }

        @Override
        public void onSeekCompleted(MediaController2 controller, long position) {
            mCallbackProxy.onSeekCompleted(controller, position);
        }

        @Override
        public void onPlaybackSpeedChanged(MediaController2 controller, float speed) {
            mCallbackProxy.onPlaybackSpeedChanged(controller, speed);
        }

        @Override
        public void onBufferingStateChanged(MediaController2 controller, MediaItem2 item,
                int state) {
            mCallbackProxy.onBufferingStateChanged(controller, item, state);
        }

        @Override
        public void onError(MediaController2 controller, int errorCode, Bundle extras) {
            mCallbackProxy.onError(controller, errorCode, extras);
        }

        @Override
        public void onCurrentMediaItemChanged(MediaController2 controller, MediaItem2 item) {
            mCallbackProxy.onCurrentMediaItemChanged(controller, item);
        }

        @Override
        public void onPlaylistChanged(MediaController2 controller,
                List<MediaItem2> list, MediaMetadata2 metadata) {
            mCallbackProxy.onPlaylistChanged(controller, list, metadata);
        }

        @Override
        public void onPlaylistMetadataChanged(MediaController2 controller,
                MediaMetadata2 metadata) {
            mCallbackProxy.onPlaylistMetadataChanged(controller, metadata);
        }

        @Override
        public void onShuffleModeChanged(MediaController2 controller, int shuffleMode) {
            mCallbackProxy.onShuffleModeChanged(controller, shuffleMode);
        }

        @Override
        public void onRepeatModeChanged(MediaController2 controller, int repeatMode) {
            mCallbackProxy.onRepeatModeChanged(controller, repeatMode);
        }
    }

    public class TestMediaController extends MediaController2 implements TestControllerInterface {
        private final ControllerCallback mCallback;

        public TestMediaController(@NonNull Context context, @NonNull SessionToken2 token,
                @NonNull ControllerCallback callback) {
            super(context, token, sHandlerExecutor, callback);
            mCallback = callback;
        }

        @Override
        public ControllerCallback getCallback() {
            return mCallback;
        }
    }
}
