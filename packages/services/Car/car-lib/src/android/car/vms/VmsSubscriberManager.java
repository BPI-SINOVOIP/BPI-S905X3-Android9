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

import android.annotation.CallbackExecutor;
import android.annotation.NonNull;
import android.annotation.SystemApi;
import android.car.Car;
import android.car.CarManagerBase;
import android.car.CarNotConnectedException;
import android.car.vms.VmsSubscriberManager.VmsSubscriberClientCallback;
import android.os.Handler;
import android.os.Binder;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.util.Log;

import com.android.internal.annotations.GuardedBy;
import com.android.internal.util.Preconditions;

import java.lang.ref.WeakReference;
import java.util.concurrent.Executor;

/**
 * API for interfacing with the VmsSubscriberService. It supports a single client callback that can
 * (un)subscribe to different layers. Getting notifactions and managing subscriptions is enabled
 * after setting the client callback with #setVmsSubscriberClientCallback.
 * SystemApi candidate
 *
 * @hide
 */
@SystemApi
public final class VmsSubscriberManager implements CarManagerBase {
    private static final boolean DBG = true;
    private static final String TAG = "VmsSubscriberManager";

    private final IVmsSubscriberService mVmsSubscriberService;
    private final IVmsSubscriberClient mSubscriberManagerClient;
    private final Object mClientCallbackLock = new Object();
    @GuardedBy("mClientCallbackLock")
    private VmsSubscriberClientCallback mClientCallback;
    @GuardedBy("mClientCallbackLock")
    private Executor mExecutor;

    /**
     * Interface exposed to VMS subscribers: it is a wrapper of IVmsSubscriberClient.
     */
    public interface VmsSubscriberClientCallback {
        /**
         * Called when the property is updated
         */
        void onVmsMessageReceived(VmsLayer layer, byte[] payload);

        /**
         * Called when layers availability change
         */
        void onLayersAvailabilityChanged(VmsAvailableLayers availableLayers);
    }

    /**
     * Hidden constructor - can only be used internally.
     * @hide
     */
    public VmsSubscriberManager(IBinder service) {
        mVmsSubscriberService = IVmsSubscriberService.Stub.asInterface(service);
        mSubscriberManagerClient = new IVmsSubscriberClient.Stub() {
            @Override
            public void onVmsMessageReceived(VmsLayer layer, byte[] payload) {
                Executor executor;
                synchronized (mClientCallbackLock) {
                    executor = mExecutor;
                }
                if (executor == null) {
                    if (DBG) {
                        Log.d(TAG, "Executor is null in onVmsMessageReceived");
                    }
                    return;
                }
                Binder.clearCallingIdentity();
                executor.execute(() -> {dispatchOnReceiveMessage(layer, payload);});
            }

            @Override
            public void onLayersAvailabilityChanged(VmsAvailableLayers availableLayers) {
                Executor executor;
                synchronized (mClientCallbackLock) {
                    executor = mExecutor;
                }
                if (executor == null) {
                    if (DBG) {
                        Log.d(TAG, "Executor is null in onLayersAvailabilityChanged");
                    }
                    return;
                }
                Binder.clearCallingIdentity();
                executor.execute(() -> {dispatchOnAvailabilityChangeMessage(availableLayers);});
            }
        };
    }

    /**
     * Sets the callback for the notification of onVmsMessageReceived events.
     * @param executor {@link Executor} to handle the callbacks
     * @param clientCallback subscriber callback that will handle onVmsMessageReceived events.
     * @throws IllegalStateException if the client callback was already set.
     */
    public void setVmsSubscriberClientCallback(@NonNull @CallbackExecutor Executor executor,
                @NonNull VmsSubscriberClientCallback clientCallback)
          throws CarNotConnectedException {
        Preconditions.checkNotNull(clientCallback);
        Preconditions.checkNotNull(executor);
        synchronized (mClientCallbackLock) {
            if (mClientCallback != null) {
                throw new IllegalStateException("Client callback is already configured.");
            }
            mClientCallback = clientCallback;
            mExecutor = executor;
        }
        try {
            mVmsSubscriberService.addVmsSubscriberToNotifications(mSubscriberManagerClient);
        } catch (RemoteException e) {
            Log.e(TAG, "Could not connect: ", e);
            throw new CarNotConnectedException(e);
        }
    }


    /**
     * Clears the client callback which disables communication with the client.
     *
     * @throws CarNotConnectedException
     */
    public void clearVmsSubscriberClientCallback() throws CarNotConnectedException {
        synchronized (mClientCallbackLock) {
            if (mExecutor == null) return;
        }
        try {
            mVmsSubscriberService.removeVmsSubscriberToNotifications(mSubscriberManagerClient);
        } catch (RemoteException e) {
            Log.e(TAG, "Could not connect: ", e);
            throw new CarNotConnectedException(e);
        }

        synchronized (mClientCallbackLock) {
            mClientCallback = null;
            mExecutor = null;
        }
    }

    /**
     * Returns a serialized publisher information for a publisher ID.
     */
    public byte[] getPublisherInfo(int publisherId)
            throws CarNotConnectedException, IllegalStateException {
        try {
            return mVmsSubscriberService.getPublisherInfo(publisherId);
        } catch (RemoteException e) {
            Log.e(TAG, "Could not connect: ", e);
            throw new CarNotConnectedException(e);
        } catch (IllegalStateException ex) {
            Car.checkCarNotConnectedExceptionFromCarService(ex);
            throw new IllegalStateException(ex);
        }
    }

    /**
     * Returns the available layers.
     */
    public VmsAvailableLayers getAvailableLayers()
            throws CarNotConnectedException, IllegalStateException {
        try {
            return mVmsSubscriberService.getAvailableLayers();
        } catch (RemoteException e) {
            Log.e(TAG, "Could not connect: ", e);
            throw new CarNotConnectedException(e);
        } catch (IllegalStateException ex) {
            Car.checkCarNotConnectedExceptionFromCarService(ex);
            throw new IllegalStateException(ex);
        }
    }

    /**
     * Subscribes to listen to the layer specified.
     *
     * @param layer the layer to subscribe to.
     * @throws IllegalStateException if the client callback was not set via
     *                               {@link #setVmsSubscriberClientCallback}.
     */
    public void subscribe(VmsLayer layer) throws CarNotConnectedException {
        verifySubscriptionIsAllowed();
        try {
            mVmsSubscriberService.addVmsSubscriber(mSubscriberManagerClient, layer);
            VmsOperationRecorder.get().subscribe(layer);
        } catch (RemoteException e) {
            Log.e(TAG, "Could not connect: ", e);
            throw new CarNotConnectedException(e);
        } catch (IllegalStateException ex) {
            Car.checkCarNotConnectedExceptionFromCarService(ex);
        }
    }

    /**
     * Subscribes to listen to the layer specified from the publisher specified.
     *
     * @param layer       the layer to subscribe to.
     * @param publisherId the publisher of the layer.
     * @throws IllegalStateException if the client callback was not set via
     *                               {@link #setVmsSubscriberClientCallback}.
     */
    public void subscribe(VmsLayer layer, int publisherId) throws CarNotConnectedException {
        verifySubscriptionIsAllowed();
        try {
            mVmsSubscriberService.addVmsSubscriberToPublisher(
                    mSubscriberManagerClient, layer, publisherId);
            VmsOperationRecorder.get().subscribe(layer, publisherId);
        } catch (RemoteException e) {
            Log.e(TAG, "Could not connect: ", e);
            throw new CarNotConnectedException(e);
        } catch (IllegalStateException ex) {
            Car.checkCarNotConnectedExceptionFromCarService(ex);
        }
    }

    public void startMonitoring() throws CarNotConnectedException {
        verifySubscriptionIsAllowed();
        try {
            mVmsSubscriberService.addVmsSubscriberPassive(mSubscriberManagerClient);
            VmsOperationRecorder.get().startMonitoring();
        } catch (RemoteException e) {
            Log.e(TAG, "Could not connect: ", e);
            throw new CarNotConnectedException(e);
        } catch (IllegalStateException ex) {
            Car.checkCarNotConnectedExceptionFromCarService(ex);
        }
    }

    /**
     * Unsubscribes from the layer/version specified.
     *
     * @param layer the layer to unsubscribe from.
     * @throws IllegalStateException if the client callback was not set via
     *                               {@link #setVmsSubscriberClientCallback}.
     */
    public void unsubscribe(VmsLayer layer) {
        verifySubscriptionIsAllowed();
        try {
            mVmsSubscriberService.removeVmsSubscriber(mSubscriberManagerClient, layer);
            VmsOperationRecorder.get().unsubscribe(layer);
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to clear subscriber", e);
            // ignore
        } catch (IllegalStateException ex) {
            Car.hideCarNotConnectedExceptionFromCarService(ex);
        }
    }

    /**
     * Unsubscribes from the layer/version specified.
     *
     * @param layer       the layer to unsubscribe from.
     * @param publisherId the pubisher of the layer.
     * @throws IllegalStateException if the client callback was not set via
     *                               {@link #setVmsSubscriberClientCallback}.
     */
    public void unsubscribe(VmsLayer layer, int publisherId) {
        try {
            mVmsSubscriberService.removeVmsSubscriberToPublisher(
                    mSubscriberManagerClient, layer, publisherId);
            VmsOperationRecorder.get().unsubscribe(layer, publisherId);
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to clear subscriber", e);
            // ignore
        } catch (IllegalStateException ex) {
            Car.hideCarNotConnectedExceptionFromCarService(ex);
        }
    }

    public void stopMonitoring() {
        try {
            mVmsSubscriberService.removeVmsSubscriberPassive(mSubscriberManagerClient);
            VmsOperationRecorder.get().stopMonitoring();
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to clear subscriber ", e);
            // ignore
        } catch (IllegalStateException ex) {
            Car.hideCarNotConnectedExceptionFromCarService(ex);
        }
    }

    private void dispatchOnReceiveMessage(VmsLayer layer, byte[] payload) {
        VmsSubscriberClientCallback clientCallback = getClientCallbackThreadSafe();
        if (clientCallback == null) {
            Log.e(TAG, "Cannot dispatch received message.");
            return;
        }
        clientCallback.onVmsMessageReceived(layer, payload);
    }

    private void dispatchOnAvailabilityChangeMessage(VmsAvailableLayers availableLayers) {
        VmsSubscriberClientCallback clientCallback = getClientCallbackThreadSafe();
        if (clientCallback == null) {
            Log.e(TAG, "Cannot dispatch availability change message.");
            return;
        }
        clientCallback.onLayersAvailabilityChanged(availableLayers);
    }

    private VmsSubscriberClientCallback getClientCallbackThreadSafe() {
        VmsSubscriberClientCallback clientCallback;
        synchronized (mClientCallbackLock) {
            clientCallback = mClientCallback;
        }
        if (clientCallback == null) {
            Log.e(TAG, "client callback not set.");
        }
        return clientCallback;
    }

    /*
     * Verifies that the subscriber is in a state where it is allowed to subscribe.
     */
    private void verifySubscriptionIsAllowed() {
        VmsSubscriberClientCallback clientCallback = getClientCallbackThreadSafe();
        if (clientCallback == null) {
            throw new IllegalStateException("Cannot subscribe.");
        }
    }

    /**
     * @hide
     */
    @Override
    public void onCarDisconnected() {
    }

    private static final class VmsDataMessage {
        private final VmsLayer mLayer;
        private final byte[] mPayload;

        public VmsDataMessage(VmsLayer layer, byte[] payload) {
            mLayer = layer;
            mPayload = payload;
        }

        public VmsLayer getLayer() {
            return mLayer;
        }

        public byte[] getPayload() {
            return mPayload;
        }
    }
}
