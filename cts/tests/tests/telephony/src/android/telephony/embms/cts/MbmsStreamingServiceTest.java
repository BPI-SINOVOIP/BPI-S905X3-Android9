/*
 * Copyright (C) 2017 The Android Open Source Project
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

package android.telephony.embms.cts;

import android.annotation.Nullable;
import android.telephony.cts.embmstestapp.CtsStreamingService;
import android.telephony.mbms.MbmsErrors;
import android.telephony.mbms.StreamingService;
import android.telephony.mbms.StreamingServiceCallback;

import com.android.internal.os.SomeArgs;

import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

public class MbmsStreamingServiceTest extends MbmsStreamingTestBase {
    private class TestStreamingServiceCallback extends StreamingServiceCallback {
        private final BlockingQueue<SomeArgs> mErrorCalls = new LinkedBlockingQueue<>();
        private final BlockingQueue<SomeArgs> mStreamStateUpdatedCalls =
                new LinkedBlockingQueue<>();
        private final BlockingQueue<SomeArgs> mMediaDescriptionUpdatedCalls =
                new LinkedBlockingQueue<>();
        private final BlockingQueue<SomeArgs> mBroadcastSignalStrengthUpdatedCalls =
                new LinkedBlockingQueue<>();
        private final BlockingQueue<SomeArgs> mStreamMethodUpdatedCalls =
                new LinkedBlockingQueue<>();

        @Override
        public void onError(int errorCode, @Nullable String message) {
            SomeArgs args = SomeArgs.obtain();
            args.arg1 = errorCode;
            args.arg2 = message;
            mErrorCalls.add(args);
        }

        @Override
        public void onStreamStateUpdated(@StreamingService.StreamingState int state,
                @StreamingService.StreamingStateChangeReason int reason) {
            SomeArgs args = SomeArgs.obtain();
            args.arg1 = state;
            args.arg2 = reason;
            mStreamStateUpdatedCalls.add(args);
        }

        @Override
        public void onMediaDescriptionUpdated() {
            SomeArgs args = SomeArgs.obtain();
            mMediaDescriptionUpdatedCalls.add(args);
        }

        @Override
        public void onBroadcastSignalStrengthUpdated(int signalStrength) {
            SomeArgs args = SomeArgs.obtain();
            args.arg1 = signalStrength;
            mBroadcastSignalStrengthUpdatedCalls.add(args);
        }

        @Override
        public void onStreamMethodUpdated(int methodType) {
            SomeArgs args = SomeArgs.obtain();
            args.arg1 = methodType;
            mStreamMethodUpdatedCalls.add(args);
        }

        public SomeArgs waitOnError() {
            try {
                return mErrorCalls.poll(ASYNC_TIMEOUT, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                return null;
            }
        }

        public SomeArgs waitOnStreamStateUpdated() {
            try {
                return mStreamStateUpdatedCalls.poll(ASYNC_TIMEOUT, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                return null;
            }
        }

        public SomeArgs waitOnMediaDescriptionUpdated() {
            try {
                return mMediaDescriptionUpdatedCalls.poll(ASYNC_TIMEOUT, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                return null;
            }
        }

        public SomeArgs waitOnBroadcastSignalStrengthUpdated() {
            try {
                return mBroadcastSignalStrengthUpdatedCalls.poll(
                        ASYNC_TIMEOUT, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                return null;
            }
        }

        public SomeArgs waitOnStreamMethodUpdated() {
            try {
                return mStreamMethodUpdatedCalls.poll(ASYNC_TIMEOUT, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                return null;
            }
        }

    }

    private TestStreamingServiceCallback mStreamingServiceCallback =
            new TestStreamingServiceCallback();

    public void testStartStreaming() throws Exception {
        StreamingService streamingService = mStreamingSession.startStreaming(
                CtsStreamingService .STREAMING_SERVICE_INFO,
                mCallbackExecutor, mStreamingServiceCallback);
        assertNotNull(streamingService);
        assertEquals(CtsStreamingService.STREAMING_SERVICE_INFO, streamingService.getInfo());

        SomeArgs args = mStreamingServiceCallback.waitOnStreamStateUpdated();
        assertEquals(StreamingService.STATE_STARTED, args.arg1);
        assertEquals(StreamingService.REASON_BY_USER_REQUEST, args.arg2);

        List<List<Object>> startStreamingCalls =
                getMiddlewareCalls(CtsStreamingService.METHOD_START_STREAMING);
        assertEquals(1, startStreamingCalls.size());
        assertEquals(CtsStreamingService.STREAMING_SERVICE_INFO.getServiceId(),
                startStreamingCalls.get(0).get(2));
    }

    public void testGetPlaybackUri() throws Exception {
        StreamingService streamingService = mStreamingSession.startStreaming(
                CtsStreamingService .STREAMING_SERVICE_INFO,
                mCallbackExecutor, mStreamingServiceCallback);
        assertEquals(CtsStreamingService.STREAMING_URI, streamingService.getPlaybackUri());

        List<List<Object>> getPlaybackUriCalls =
                getMiddlewareCalls(CtsStreamingService.METHOD_GET_PLAYBACK_URI);
        assertEquals(1, getPlaybackUriCalls.size());
        assertEquals(CtsStreamingService.STREAMING_SERVICE_INFO.getServiceId(),
                getPlaybackUriCalls.get(0).get(2));
    }

    public void testStopStreaming() throws Exception {
        StreamingService streamingService = mStreamingSession.startStreaming(
                CtsStreamingService .STREAMING_SERVICE_INFO,
                mCallbackExecutor, mStreamingServiceCallback);
        streamingService.close();
        List<List<Object>> stopStreamingCalls =
                getMiddlewareCalls(CtsStreamingService.METHOD_STOP_STREAMING);
        assertEquals(1, stopStreamingCalls.size());
        assertEquals(CtsStreamingService.STREAMING_SERVICE_INFO.getServiceId(),
                stopStreamingCalls.get(0).get(2));
    }

    public void testStreamingCallbacks() throws Exception {
        mStreamingSession.startStreaming(
                CtsStreamingService .STREAMING_SERVICE_INFO,
                mCallbackExecutor, mStreamingServiceCallback);

        mMiddlewareControl.fireErrorOnStream(
                MbmsErrors.StreamingErrors.ERROR_UNABLE_TO_START_SERVICE, "");
        assertEquals(MbmsErrors.StreamingErrors.ERROR_UNABLE_TO_START_SERVICE,
                mStreamingServiceCallback.waitOnError().arg1);

        mMiddlewareControl.fireOnMediaDescriptionUpdated();
        assertNotNull(mStreamingServiceCallback.waitOnMediaDescriptionUpdated());

        int broadcastSignalStrength = 3;
        mMiddlewareControl.fireOnBroadcastSignalStrengthUpdated(broadcastSignalStrength);
        assertEquals(broadcastSignalStrength,
                mStreamingServiceCallback.waitOnBroadcastSignalStrengthUpdated().arg1);

        mMiddlewareControl.fireOnStreamMethodUpdated(StreamingService.BROADCAST_METHOD);
        assertEquals(StreamingService.BROADCAST_METHOD,
                mStreamingServiceCallback.waitOnStreamMethodUpdated().arg1);
    }

    public void testStartStreamingFailure() throws Exception {
        mMiddlewareControl.forceErrorCode(
                MbmsErrors.GeneralErrors.ERROR_MIDDLEWARE_TEMPORARILY_UNAVAILABLE);
        mStreamingSession.startStreaming(CtsStreamingService.STREAMING_SERVICE_INFO,
                mCallbackExecutor, mStreamingServiceCallback);
        assertEquals(MbmsErrors.GeneralErrors.ERROR_MIDDLEWARE_TEMPORARILY_UNAVAILABLE,
                mCallback.waitOnError().arg1);
    }
}
