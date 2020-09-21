/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.bips.discovery;

import android.content.Context;
import android.net.nsd.NsdManager;
import android.net.nsd.NsdServiceInfo;
import android.os.Handler;
import android.util.Log;

import java.util.LinkedList;

/**
 * Queues Nsd resolve requests to prevent multiple simultaneous requests to NsdManager
 */
public class NsdResolveQueue {
    private static final String TAG = NsdResolveQueue.class.getSimpleName();
    private static final boolean DEBUG = false;

    private final NsdManager mNsdManager;
    private final Handler mMainHandler;

    /** Current set of registered service info resolve attempts */
    private LinkedList<NsdResolveRequest> mResolveRequests = new LinkedList<>();

    public NsdResolveQueue(Context context, NsdManager nsdManager) {
        mNsdManager = nsdManager;
        mMainHandler = new Handler(context.getMainLooper());
    }

    /** Return the {@link NsdManager} used by this queue */
    NsdManager getNsdManager() {
        return mNsdManager;
    }

    /**
     * Resolve a serviceInfo or queue the request if there is a request currently in flight.
     *
     * @param serviceInfo The service info to resolve
     * @param listener    The listener to call back once the info is resolved.
     */
    public NsdResolveRequest resolve(NsdServiceInfo serviceInfo,
            NsdManager.ResolveListener listener) {
        if (DEBUG) {
            Log.d(TAG, "Adding resolve of " + serviceInfo.getServiceName() + " to queue size="
                    + mResolveRequests.size());
        }
        NsdResolveRequest request = new NsdResolveRequest(mNsdManager, serviceInfo, listener);
        mResolveRequests.addLast(request);
        if (mResolveRequests.size() == 1) {
            resolveNextRequest();
        }
        return request;
    }

    /**
     * Resolve the next request if there is one.
     */
    private void resolveNextRequest() {
        if (!mResolveRequests.isEmpty()) {
            mResolveRequests.getFirst().start();
        }
    }

    /**
     * Holds a request to resolve a {@link NsdServiceInfo}
     */
    class NsdResolveRequest implements NsdManager.ResolveListener {
        private final NsdManager mNsdManager;
        private final NsdServiceInfo mServiceInfo;
        private final NsdManager.ResolveListener mListener;
        private long mStartTime;

        private NsdResolveRequest(NsdManager nsdManager,
                NsdServiceInfo serviceInfo,
                NsdManager.ResolveListener listener) {
            mNsdManager = nsdManager;
            mServiceInfo = serviceInfo;
            mListener = listener;
        }

        private void start() {
            mStartTime = System.currentTimeMillis();
            if (DEBUG) Log.d(TAG, "resolveService " + mServiceInfo.getServiceName());
            mNsdManager.resolveService(mServiceInfo, this);
        }

        void cancel() {
            // Note: resolve requests can only be cancelled if they have not yet begun
            if (!mResolveRequests.isEmpty() && mResolveRequests.get(0) != this) {
                mResolveRequests.remove(this);
            }
        }

        @Override
        public void onResolveFailed(NsdServiceInfo serviceInfo, int errorCode) {
            if (DEBUG) {
                Log.d(TAG, "onResolveFailed " + serviceInfo.getServiceName() + " errorCode="
                        + errorCode + " (" + (System.currentTimeMillis() - mStartTime) + " ms)");
            }
            mMainHandler.post(() -> {
                mListener.onResolveFailed(serviceInfo, errorCode);
                mResolveRequests.pop();
                resolveNextRequest();
            });
        }

        @Override
        public void onServiceResolved(NsdServiceInfo serviceInfo) {
            if (DEBUG) {
                Log.d(TAG, "onServiceResolved " + serviceInfo.getServiceName()
                        + " (" + (System.currentTimeMillis() - mStartTime) + " ms)");
            }
            mMainHandler.post(() -> {
                mListener.onServiceResolved(serviceInfo);
                mResolveRequests.pop();
                resolveNextRequest();
            });
        }
    }
}
