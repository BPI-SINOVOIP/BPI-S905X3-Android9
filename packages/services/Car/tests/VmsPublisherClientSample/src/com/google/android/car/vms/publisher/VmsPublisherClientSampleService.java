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

package com.google.android.car.vms.publisher;

import android.car.vms.VmsLayer;
import android.car.vms.VmsPublisherClientService;
import android.car.vms.VmsSubscriptionState;
import android.os.Handler;
import android.os.Message;

import java.util.concurrent.atomic.AtomicBoolean;

/**
 * This service is launched during the initialization of the VMS publisher service.
 * Once onVmsPublisherServiceReady is invoked, it starts publishing a single byte every second.
 */
public class VmsPublisherClientSampleService extends VmsPublisherClientService {
    public static final int PUBLISH_EVENT = 0;
    public static final VmsLayer TEST_LAYER = new VmsLayer(0, 0, 0);
    public static final int PUBLISHER_ID = 1;

    private byte mCounter = 0;
    private AtomicBoolean mInitialized = new AtomicBoolean(false);

    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == PUBLISH_EVENT && mInitialized.get()) {
                periodicPublish();
            }
        }
    };

    /**
     * Notifies that the publisher services are ready to be used: {@link #publish(VmsLayer, byte[])}
     * and {@link #getSubscriptions()}.
     */
    @Override
    public void onVmsPublisherServiceReady() {
        VmsSubscriptionState subscriptionState = getSubscriptions();
        onVmsSubscriptionChange(subscriptionState);
    }

    @Override
    public void onVmsSubscriptionChange(VmsSubscriptionState subscriptionState) {
        if (mInitialized.compareAndSet(false, true)) {
            for (VmsLayer layer : subscriptionState.getLayers()) {
                if (layer.equals(TEST_LAYER)) {
                    mHandler.sendEmptyMessage(PUBLISH_EVENT);
                }
            }
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mInitialized.set(false);
        mHandler.removeMessages(PUBLISH_EVENT);
    }

    private void periodicPublish() {
        publish(TEST_LAYER, PUBLISHER_ID, new byte[]{mCounter});
        ++mCounter;
        mHandler.sendEmptyMessageDelayed(PUBLISH_EVENT, 1000);
    }
}
