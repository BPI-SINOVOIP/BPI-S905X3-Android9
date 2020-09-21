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

package com.android.car;

import android.car.vms.IVmsPublisherClient;
import android.car.vms.IVmsPublisherService;
import android.car.vms.IVmsSubscriberClient;
import android.car.vms.VmsLayer;
import android.car.vms.VmsLayersOffering;
import android.car.vms.VmsSubscriptionState;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import android.os.UserHandle;
import android.text.TextUtils;
import android.util.ArrayMap;
import android.util.ArraySet;
import android.util.Log;

import com.android.car.hal.VmsHalService;
import com.android.car.hal.VmsHalService.VmsHalPublisherListener;

import java.io.PrintWriter;
import java.util.Arrays;
import java.util.Map;
import java.util.Set;

/**
 * Receives HAL updates by implementing VmsHalService.VmsHalListener.
 * Binds to publishers and configures them to use this service.
 * Notifies publishers of subscription changes.
 */
public class VmsPublisherService extends IVmsPublisherService.Stub implements CarServiceBase {
    private static final boolean DBG = true;
    private static final String TAG = "VmsPublisherService";

    private static final int MSG_HAL_SUBSCRIPTION_CHANGED = 1;

    private final Context mContext;
    private final VmsHalService mHal;
    private final Map<String, PublisherConnection> mPublisherConnectionMap = new ArrayMap<>();
    private final Map<String, IVmsPublisherClient> mPublisherMap = new ArrayMap<>();
    private final Set<String> mSafePermissions;
    private final Handler mHandler = new EventHandler();
    private final VmsHalPublisherListener mHalPublisherListener;

    private BroadcastReceiver mBootCompleteReceiver;

    public VmsPublisherService(Context context, VmsHalService hal) {
        mContext = context;
        mHal = hal;

        mHalPublisherListener = subscriptionState -> mHandler.sendMessage(
                mHandler.obtainMessage(MSG_HAL_SUBSCRIPTION_CHANGED, subscriptionState));

        // Load permissions that can be granted to publishers.
        mSafePermissions = new ArraySet<>(
                Arrays.asList(mContext.getResources().getStringArray(R.array.vmsSafePermissions)));
    }

    // Implements CarServiceBase interface.
    @Override
    public void init() {
        mHal.addPublisherListener(mHalPublisherListener);

        if (isTestEnvironment()) {
            Log.d(TAG, "Running under test environment");
            bindToAllPublishers();
        } else {
            mBootCompleteReceiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    if (Intent.ACTION_LOCKED_BOOT_COMPLETED.equals(intent.getAction())) {
                        onLockedBootCompleted();
                    } else {
                        Log.e(TAG, "Unexpected action received: " + intent);
                    }
                }
            };

            mContext.registerReceiver(mBootCompleteReceiver,
                    new IntentFilter(Intent.ACTION_LOCKED_BOOT_COMPLETED));
        }
    }

    private void bindToAllPublishers() {
        String[] publisherNames = mContext.getResources().getStringArray(
                R.array.vmsPublisherClients);
        if (DBG) Log.d(TAG, "Publishers found: " + publisherNames.length);

        for (String publisherName : publisherNames) {
            if (TextUtils.isEmpty(publisherName)) {
                Log.e(TAG, "empty publisher name");
                continue;
            }
            ComponentName name = ComponentName.unflattenFromString(publisherName);
            if (name == null) {
                Log.e(TAG, "invalid publisher name: " + publisherName);
                continue;
            }

            if (!mContext.getPackageManager().isPackageAvailable(name.getPackageName())) {
                Log.w(TAG, "VMS publisher not installed: " + publisherName);
                continue;
            }

            bind(name);
        }
    }

    @Override
    public void release() {
        if (mBootCompleteReceiver != null) {
            mContext.unregisterReceiver(mBootCompleteReceiver);
            mBootCompleteReceiver = null;
        }
        mHal.removePublisherListener(mHalPublisherListener);

        for (PublisherConnection connection : mPublisherConnectionMap.values()) {
            mContext.unbindService(connection);
        }
        mPublisherConnectionMap.clear();
        mPublisherMap.clear();
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println("*" + getClass().getSimpleName() + "*");
        writer.println("mSafePermissions: " + mSafePermissions);
        writer.println("mPublisherMap:" + mPublisherMap);
        writer.println("mPublisherConnectionMap:" + mPublisherConnectionMap);
    }

    /* Called in arbitrary binder thread */
    @Override
    public void setLayersOffering(IBinder token, VmsLayersOffering offering) {
        mHal.setPublisherLayersOffering(token, offering);
    }

    /* Called in arbitrary binder thread */
    @Override
    public void publish(IBinder token, VmsLayer layer, int publisherId, byte[] payload) {
        if (DBG) {
            Log.d(TAG, "Publishing for layer: " + layer);
        }
        ICarImpl.assertVmsPublisherPermission(mContext);

        // Send the message to application listeners.
        Set<IVmsSubscriberClient> listeners =
                mHal.getSubscribersForLayerFromPublisher(layer, publisherId);

        if (DBG) {
            Log.d(TAG, "Number of subscribed apps: " + listeners.size());
        }
        for (IVmsSubscriberClient listener : listeners) {
            try {
                listener.onVmsMessageReceived(layer, payload);
            } catch (RemoteException ex) {
                Log.e(TAG, "unable to publish to listener: " + listener);
            }
        }

        // Send the message to HAL
        if (mHal.isHalSubscribed(layer)) {
            Log.d(TAG, "HAL is subscribed");
            mHal.setDataMessage(layer, payload);
        } else {
            Log.d(TAG, "HAL is NOT subscribed");
        }
    }

    /* Called in arbitrary binder thread */
    @Override
    public VmsSubscriptionState getSubscriptions() {
        ICarImpl.assertVmsPublisherPermission(mContext);
        return mHal.getSubscriptionState();
    }

    /* Called in arbitrary binder thread */
    @Override
    public int getPublisherId(byte[] publisherInfo) {
        ICarImpl.assertVmsPublisherPermission(mContext);
        return mHal.getPublisherId(publisherInfo);
    }

    private void onLockedBootCompleted() {
        if (DBG) Log.i(TAG, "onLockedBootCompleted");

        bindToAllPublishers();
    }

    /**
     * This method is only invoked by VmsHalService.notifyPublishers which is synchronized.
     * Therefore this method only sees a non-decreasing sequence.
     */
    private void handleHalSubscriptionChanged(VmsSubscriptionState subscriptionState) {
        // Send the message to application listeners.
        for (IVmsPublisherClient client : mPublisherMap.values()) {
            try {
                client.onVmsSubscriptionChange(subscriptionState);
            } catch (RemoteException ex) {
                Log.e(TAG, "unable to send notification to: " + client, ex);
            }
        }
    }

    /**
     * Tries to bind to a publisher.
     *
     * @param name publisher component name (e.g. android.car.vms.logger/.LoggingService).
     */
    private void bind(ComponentName name) {
        String publisherName = name.flattenToString();
        if (DBG) {
            Log.d(TAG, "binding to: " + publisherName);
        }

        if (mPublisherConnectionMap.containsKey(publisherName)) {
            // Already registered, nothing to do.
            return;
        }
        grantPermissions(name);
        Intent intent = new Intent();
        intent.setComponent(name);
        PublisherConnection connection = new PublisherConnection(name);
        if (mContext.bindServiceAsUser(intent, connection,
                Context.BIND_AUTO_CREATE, UserHandle.SYSTEM)) {
            mPublisherConnectionMap.put(publisherName, connection);
        } else {
            Log.e(TAG, "unable to bind to: " + publisherName);
        }
    }

    /**
     * Removes the publisher and associated connection.
     *
     * @param name publisher component name (e.g. android.car.vms.Logger).
     */
    private void unbind(ComponentName name) {
        String publisherName = name.flattenToString();
        if (DBG) {
            Log.d(TAG, "unbinding from: " + publisherName);
        }

        boolean found = mPublisherMap.remove(publisherName) != null;
        if (found) {
            PublisherConnection connection = mPublisherConnectionMap.get(publisherName);
            mContext.unbindService(connection);
            mPublisherConnectionMap.remove(publisherName);
        } else {
            Log.e(TAG, "unbind: unknown publisher." + publisherName);
        }
    }

    private void grantPermissions(ComponentName component) {
        final PackageManager packageManager = mContext.getPackageManager();
        final String packageName = component.getPackageName();
        PackageInfo packageInfo;
        try {
            packageInfo = packageManager.getPackageInfo(packageName,
                    PackageManager.GET_PERMISSIONS);
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(TAG, "Error getting package info for " + packageName, e);
            return;
        }
        if (packageInfo.requestedPermissions == null) return;
        for (String permission : packageInfo.requestedPermissions) {
            if (!mSafePermissions.contains(permission)) {
                continue;
            }
            if (packageManager.checkPermission(permission, packageName)
                    == PackageManager.PERMISSION_GRANTED) {
                continue;
            }
            try {
                packageManager.grantRuntimePermission(packageName, permission,
                        UserHandle.SYSTEM);
                Log.d(TAG, "Permission " + permission + " granted to " + packageName);
            } catch (SecurityException | IllegalArgumentException e) {
                Log.e(TAG, "Error while trying to grant " + permission + " to " + packageName,
                        e);
            }
        }
    }

    private boolean isTestEnvironment() {
        // If the context is derived from other package it means we're running under
        // environment.
        return !TextUtils.equals(mContext.getBasePackageName(), mContext.getPackageName());
    }

    class PublisherConnection implements ServiceConnection {
        private final IBinder mToken = new Binder();
        private final ComponentName mName;

        PublisherConnection(ComponentName name) {
            mName = name;
        }

        private final Runnable mBindRunnable = new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "delayed binding for: " + mName);
                bind(mName);
            }
        };

        /**
         * Once the service binds to a publisher service, the publisher binder is added to
         * mPublisherMap
         * and the publisher is configured to use this service.
         */
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            if (DBG) {
                Log.d(TAG, "onServiceConnected, name: " + name + ", binder: " + binder);
            }
            IVmsPublisherClient service = IVmsPublisherClient.Stub.asInterface(binder);
            mPublisherMap.put(name.flattenToString(), service);
            try {
                service.setVmsPublisherService(mToken, VmsPublisherService.this);
            } catch (RemoteException e) {
                Log.e(TAG, "unable to configure publisher: " + name, e);
            }
        }

        /**
         * Tries to rebind to the publisher service.
         */
        @Override
        public void onServiceDisconnected(ComponentName name) {
            String publisherName = name.flattenToString();
            Log.d(TAG, "onServiceDisconnected, name: " + publisherName);

            int millisecondsToWait = mContext.getResources().getInteger(
                    com.android.car.R.integer.millisecondsBeforeRebindToVmsPublisher);
            if (!mName.flattenToString().equals(name.flattenToString())) {
                throw new IllegalArgumentException(
                    "Mismatch on publisherConnection. Expected: " + mName + " Got: " + name);
            }
            mHandler.postDelayed(mBindRunnable, millisecondsToWait);

            unbind(name);
        }
    }

    private class EventHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_HAL_SUBSCRIPTION_CHANGED:
                    handleHalSubscriptionChanged((VmsSubscriptionState) msg.obj);
                    return;
            }
            super.handleMessage(msg);
        }
    }
}
