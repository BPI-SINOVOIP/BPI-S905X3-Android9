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

package android.telephony.cts.embmstestapp;

import android.app.Service;
import android.content.ComponentName;
import android.content.Intent;
import android.net.Uri;
import android.os.Binder;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.RemoteException;
import android.telephony.mbms.MbmsErrors;
import android.telephony.mbms.MbmsStreamingSessionCallback;
import android.telephony.mbms.StreamingService;
import android.telephony.mbms.StreamingServiceCallback;
import android.telephony.mbms.StreamingServiceInfo;
import android.telephony.mbms.vendor.MbmsStreamingServiceBase;
import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

public class CtsStreamingService extends Service {
    private static final Set<String> ALLOWED_PACKAGES = new HashSet<String>() {{
        add("android.telephony.cts");
    }};
    private static final String TAG = "EmbmsTestStreaming";

    public static final String METHOD_INITIALIZE = "initialize";
    public static final String METHOD_REQUEST_UPDATE_STREAMING_SERVICES =
            "requestUpdateStreamingServices";
    public static final String METHOD_START_STREAMING = "startStreaming";
    public static final String METHOD_GET_PLAYBACK_URI = "getPlaybackUri";
    public static final String METHOD_STOP_STREAMING = "stopStreaming";
    public static final String METHOD_CLOSE = "close";

    public static final String CONTROL_INTERFACE_ACTION =
            "android.telephony.cts.embmstestapp.ACTION_CONTROL_MIDDLEWARE";
    public static final ComponentName CONTROL_INTERFACE_COMPONENT =
            ComponentName.unflattenFromString(
                    "android.telephony.cts.embmstestapp/.CtsStreamingService");
    public static final Uri STREAMING_URI = Uri.parse("http://www.example.com/stream1");

    public static final StreamingServiceInfo STREAMING_SERVICE_INFO;
    static {
        String id = "StreamingServiceId";
        Map<Locale, String> localeDict = new HashMap<Locale, String>() {{
            put(Locale.US, "Entertainment Source 1");
            put(Locale.CANADA, "Entertainment Source 1, eh?");
        }};
        List<Locale> locales = new ArrayList<Locale>() {{
            add(Locale.CANADA);
            add(Locale.US);
        }};
        STREAMING_SERVICE_INFO = new StreamingServiceInfo(localeDict, "class1", locales,
                id, new Date(2017, 8, 21, 18, 20, 29),
                new Date(2017, 8, 21, 18, 23, 9));
    }

    private static final int SEND_STREAMING_SERVICES_LIST = 1;

    private MbmsStreamingSessionCallback mAppCallback;
    private StreamingServiceCallback mStreamCallback;

    private HandlerThread mHandlerThread;
    private Handler mHandler;
    private Handler.Callback mWorkerCallback = (msg) -> {
        switch (msg.what) {
            case SEND_STREAMING_SERVICES_LIST:
                List<StreamingServiceInfo> services = (List<StreamingServiceInfo>) msg.obj;
                if (mAppCallback!= null) {
                    mAppCallback.onStreamingServicesUpdated(services);
                }
                break;
        }
        return true;
    };
    private List<List> mReceivedCalls = new LinkedList<>();
    private int mErrorCodeOverride = MbmsErrors.SUCCESS;

    private final MbmsStreamingServiceBase mStreamingServiceImpl = new MbmsStreamingServiceBase() {
        @Override
        public int initialize(MbmsStreamingSessionCallback callback, int subId) {
            mReceivedCalls.add(Arrays.asList(METHOD_INITIALIZE, subId));
            if (mErrorCodeOverride != MbmsErrors.SUCCESS) {
                return mErrorCodeOverride;
            }


            int packageUid = Binder.getCallingUid();
            String[] packageNames = getPackageManager().getPackagesForUid(packageUid);
            if (packageNames == null) {
                return MbmsErrors.InitializationErrors.ERROR_APP_PERMISSIONS_NOT_GRANTED;
            }
            boolean isUidAllowed = Arrays.stream(packageNames).anyMatch(ALLOWED_PACKAGES::contains);
            if (!isUidAllowed) {
                return MbmsErrors.InitializationErrors.ERROR_APP_PERMISSIONS_NOT_GRANTED;
            }

            mHandler.post(() -> {
                if (mAppCallback == null) {
                    mAppCallback = callback;
                } else {
                    callback.onError(
                            MbmsErrors.InitializationErrors.ERROR_DUPLICATE_INITIALIZE, "");
                    return;
                }
                callback.onMiddlewareReady();
            });
            return MbmsErrors.SUCCESS;
        }

        @Override
        public int requestUpdateStreamingServices(int subscriptionId, List<String> serviceClasses) {
            mReceivedCalls.add(Arrays.asList(METHOD_REQUEST_UPDATE_STREAMING_SERVICES,
                    subscriptionId, serviceClasses));
            if (mErrorCodeOverride != MbmsErrors.SUCCESS) {
                return mErrorCodeOverride;
            }

            List<StreamingServiceInfo> serviceInfos =
                    Collections.singletonList(STREAMING_SERVICE_INFO);

            mHandler.removeMessages(SEND_STREAMING_SERVICES_LIST);
            mHandler.sendMessage(
                    mHandler.obtainMessage(SEND_STREAMING_SERVICES_LIST, serviceInfos));
            return MbmsErrors.SUCCESS;
        }

        @Override
        public int startStreaming(int subscriptionId, String serviceId,
                StreamingServiceCallback callback) {
            mReceivedCalls.add(Arrays.asList(METHOD_START_STREAMING, subscriptionId, serviceId));
            if (mErrorCodeOverride != MbmsErrors.SUCCESS) {
                return mErrorCodeOverride;
            }

            mStreamCallback = callback;
            mHandler.post(() -> callback.onStreamStateUpdated(StreamingService.STATE_STARTED,
                    StreamingService.REASON_BY_USER_REQUEST));
            return MbmsErrors.SUCCESS;
        }

        @Override
        public Uri getPlaybackUri(int subscriptionId, String serviceId) {
            mReceivedCalls.add(Arrays.asList(METHOD_GET_PLAYBACK_URI, subscriptionId, serviceId));
            return STREAMING_URI;
        }

        @Override
        public void stopStreaming(int subscriptionId, String serviceId) {
            mReceivedCalls.add(Arrays.asList(METHOD_STOP_STREAMING, subscriptionId, serviceId));
        }

        @Override
        public void dispose(int subscriptionId) {
            mReceivedCalls.add(Arrays.asList(METHOD_CLOSE, subscriptionId));
            // TODO
        }

        @Override
        public void onAppCallbackDied(int uid, int subscriptionId) {
            mAppCallback = null;
        }
    };

    private final IBinder mControlInterface = new ICtsStreamingMiddlewareControl.Stub() {
        @Override
        public void reset() {
            mReceivedCalls.clear();
            mHandler.removeCallbacksAndMessages(null);
            mAppCallback = null;
            mErrorCodeOverride = MbmsErrors.SUCCESS;
        }

        @Override
        public List getStreamingSessionCalls() {
            return mReceivedCalls;
        }

        @Override
        public void forceErrorCode(int error) {
            mErrorCodeOverride = error;
        }

        @Override
        public void fireErrorOnStream(int errorCode, String message) {
            mHandler.post(() -> mStreamCallback.onError(errorCode, message));
        }

        @Override
        public void fireErrorOnSession(int errorCode, String message) {
            mHandler.post(() -> mAppCallback.onError(errorCode, message));
        }

        @Override
        public void fireOnMediaDescriptionUpdated() {
            mHandler.post(() -> mStreamCallback.onMediaDescriptionUpdated());
        }

        @Override
        public void fireOnBroadcastSignalStrengthUpdated(int signalStrength) {
            mHandler.post(() -> mStreamCallback.onBroadcastSignalStrengthUpdated(signalStrength));
        }

        @Override
        public void fireOnStreamMethodUpdated(int methodType){
            mHandler.post(() -> mStreamCallback.onStreamMethodUpdated(methodType));
        }
    };

    @Override
    public void onDestroy() {
        super.onCreate();
        mHandlerThread.quitSafely();
        logd("CtsStreamingService onDestroy");
    }

    @Override
    public IBinder onBind(Intent intent) {
        if (CONTROL_INTERFACE_ACTION.equals(intent.getAction())) {
            logd("CtsStreamingService control interface bind");
            return mControlInterface;
        }

        logd("CtsStreamingService onBind");
        if (mHandlerThread != null && mHandlerThread.isAlive()) {
            return mStreamingServiceImpl;
        }

        mHandlerThread = new HandlerThread("CtsStreamingServiceWorker");
        mHandlerThread.start();
        mHandler = new Handler(mHandlerThread.getLooper(), mWorkerCallback);
        return mStreamingServiceImpl;
    }

    private static void logd(String s) {
        Log.d(TAG, s);
    }

    private void checkInitialized() {
        if (mAppCallback == null) {
            throw new IllegalStateException("Not yet initialized");
        }
    }
}
