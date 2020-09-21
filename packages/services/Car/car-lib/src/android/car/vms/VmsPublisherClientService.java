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
 * limitations under the License.
 */

package android.car.vms;


import android.annotation.SystemApi;
import android.app.Service;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.annotation.Nullable;
import android.util.Log;

import com.android.internal.annotations.GuardedBy;

import java.lang.ref.WeakReference;

/**
 * Services that need VMS publisher services need to inherit from this class and also need to be
 * declared in the array vmsPublisherClients located in
 * packages/services/Car/service/res/values/config.xml (most likely, this file will be in an overlay
 * of the target product.
 *
 * The {@link com.android.car.VmsPublisherService} will start this service. The callback
 * {@link #onVmsPublisherServiceReady()} notifies when VMS publisher services can be used, and the
 * publisher can request a publisher ID in order to start publishing.
 *
 * SystemApi candidate.
 *
 * @hide
 */
@SystemApi
public abstract class VmsPublisherClientService extends Service {
    private static final boolean DBG = true;
    private static final String TAG = "VmsPublisherClient";

    private final Object mLock = new Object();

    private Handler mHandler = new VmsEventHandler(this);
    private final VmsPublisherClientBinder mVmsPublisherClient = new VmsPublisherClientBinder(this);
    private volatile IVmsPublisherService mVmsPublisherService = null;
    @GuardedBy("mLock")
    private IBinder mToken = null;

    @Override
    public IBinder onBind(Intent intent) {
        if (DBG) {
            Log.d(TAG, "onBind, intent: " + intent);
        }
        return mVmsPublisherClient.asBinder();
    }

    @Override
    public boolean onUnbind(Intent intent) {
        if (DBG) {
            Log.d(TAG, "onUnbind, intent: " + intent);
        }
        stopSelf();
        return super.onUnbind(intent);
    }

    private void setToken(IBinder token) {
        synchronized (mLock) {
            mToken = token;
        }
    }

    /**
     * Notifies that the publisher services are ready.
     */
    protected abstract void onVmsPublisherServiceReady();

    /**
     * Publishers need to implement this method to receive notifications of subscription changes.
     *
     * @param subscriptionState the state of the subscriptions.
     */
    public abstract void onVmsSubscriptionChange(VmsSubscriptionState subscriptionState);

    /**
     * Uses the VmsPublisherService binder to publish messages.
     *
     * @param layer   the layer to publish to.
     * @param payload the message to be sent.
     * @param publisherId the ID that got assigned to the publisher that published the message by
     *                    VMS core.
     * @return if the call to the method VmsPublisherService.publish was successful.
     */
    public final void publish(VmsLayer layer, int publisherId, byte[] payload) {
        if (DBG) {
            Log.d(TAG, "Publishing for layer : " + layer);
        }

        IBinder token = getTokenForPublisherServiceThreadSafe();

        try {
            mVmsPublisherService.publish(token, layer, publisherId, payload);
        } catch (RemoteException e) {
            Log.e(TAG, "unable to publish message: " + payload, e);
        }
    }

    /**
     * Uses the VmsPublisherService binder to set the layers offering.
     *
     * @param offering the layers that the publisher may publish.
     * @return if the call to VmsPublisherService.setLayersOffering was successful.
     */
    public final void setLayersOffering(VmsLayersOffering offering) {
        if (DBG) {
            Log.d(TAG, "Setting layers offering : " + offering);
        }

        IBinder token = getTokenForPublisherServiceThreadSafe();

        try {
            mVmsPublisherService.setLayersOffering(token, offering);
            VmsOperationRecorder.get().setLayersOffering(offering);
        } catch (RemoteException e) {
            Log.e(TAG, "unable to set layers offering: " + offering, e);
        }
    }

    private IBinder getTokenForPublisherServiceThreadSafe() {
        if (mVmsPublisherService == null) {
            throw new IllegalStateException("VmsPublisherService not set.");
        }

        IBinder token;
        synchronized (mLock) {
            token = mToken;
        }
        if (token == null) {
            throw new IllegalStateException("VmsPublisherService does not have a valid token.");
        }
        return token;
    }

    public final int getPublisherId(byte[] publisherInfo) {
        if (mVmsPublisherService == null) {
            throw new IllegalStateException("VmsPublisherService not set.");
        }
        Integer publisherId = null;
        try {
            Log.i(TAG, "Getting publisher static ID");
            publisherId = mVmsPublisherService.getPublisherId(publisherInfo);
        } catch (RemoteException e) {
            Log.e(TAG, "unable to invoke binder method.", e);
        }
        if (publisherId == null) {
            throw new IllegalStateException("VmsPublisherService cannot get a publisher static ID.");
        } else {
            VmsOperationRecorder.get().getPublisherId(publisherId);
        }
        return publisherId;
    }

    /**
     * Uses the VmsPublisherService binder to get the state of the subscriptions.
     *
     * @return list of layer/version or null in case of error.
     */
    public final @Nullable VmsSubscriptionState getSubscriptions() {
        if (mVmsPublisherService == null) {
            throw new IllegalStateException("VmsPublisherService not set.");
        }
        try {
            return mVmsPublisherService.getSubscriptions();
        } catch (RemoteException e) {
            Log.e(TAG, "unable to invoke binder method.", e);
        }
        return null;
    }

    private void setVmsPublisherService(IVmsPublisherService service) {
        mVmsPublisherService = service;
        onVmsPublisherServiceReady();
    }

    /**
     * Implements the interface that the VMS service uses to communicate with this client.
     */
    private static class VmsPublisherClientBinder extends IVmsPublisherClient.Stub {
        private final WeakReference<VmsPublisherClientService> mVmsPublisherClientService;
        @GuardedBy("mSequenceLock")
        private long mSequence = -1;
        private final Object mSequenceLock = new Object();

        public VmsPublisherClientBinder(VmsPublisherClientService vmsPublisherClientService) {
            mVmsPublisherClientService = new WeakReference<>(vmsPublisherClientService);
        }

        @Override
        public void setVmsPublisherService(IBinder token, IVmsPublisherService service)
                throws RemoteException {
            VmsPublisherClientService vmsPublisherClientService = mVmsPublisherClientService.get();
            if (vmsPublisherClientService == null) return;
            if (DBG) {
                Log.d(TAG, "setting VmsPublisherService.");
            }
            Handler handler = vmsPublisherClientService.mHandler;
            handler.sendMessage(
                    handler.obtainMessage(VmsEventHandler.SET_SERVICE_CALLBACK, service));
            vmsPublisherClientService.setToken(token);
        }

        @Override
        public void onVmsSubscriptionChange(VmsSubscriptionState subscriptionState)
                throws RemoteException {
            VmsPublisherClientService vmsPublisherClientService = mVmsPublisherClientService.get();
            if (vmsPublisherClientService == null) return;
            if (DBG) {
                Log.d(TAG, "subscription event: " + subscriptionState);
            }
            synchronized (mSequenceLock) {
                if (subscriptionState.getSequenceNumber() <= mSequence) {
                    Log.w(TAG, "Sequence out of order. Current sequence = " + mSequence
                            + "; expected new sequence = " + subscriptionState.getSequenceNumber());
                    // Do not propagate old notifications.
                    return;
                } else {
                    mSequence = subscriptionState.getSequenceNumber();
                }
            }
            Handler handler = vmsPublisherClientService.mHandler;
            handler.sendMessage(
                    handler.obtainMessage(VmsEventHandler.ON_SUBSCRIPTION_CHANGE_EVENT,
                            subscriptionState));
        }
    }

    /**
     * Receives events from the binder thread and dispatches them.
     */
    private final static class VmsEventHandler extends Handler {
        /** Constants handled in the handler */
        private static final int ON_SUBSCRIPTION_CHANGE_EVENT = 0;
        private static final int SET_SERVICE_CALLBACK = 1;

        private final WeakReference<VmsPublisherClientService> mVmsPublisherClientService;

        VmsEventHandler(VmsPublisherClientService service) {
            super(Looper.getMainLooper());
            mVmsPublisherClientService = new WeakReference<>(service);
        }

        @Override
        public void handleMessage(Message msg) {
            VmsPublisherClientService service = mVmsPublisherClientService.get();
            if (service == null) return;
            switch (msg.what) {
                case ON_SUBSCRIPTION_CHANGE_EVENT:
                    VmsSubscriptionState subscriptionState = (VmsSubscriptionState) msg.obj;
                    service.onVmsSubscriptionChange(subscriptionState);
                    break;
                case SET_SERVICE_CALLBACK:
                    service.setVmsPublisherService((IVmsPublisherService) msg.obj);
                    break;
                default:
                    Log.e(TAG, "Event type not handled:  " + msg.what);
                    break;
            }
        }
    }
}
