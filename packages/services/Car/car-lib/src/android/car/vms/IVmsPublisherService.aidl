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

import android.car.vms.VmsLayer;
import android.car.vms.VmsLayersOffering;
import android.car.vms.VmsSubscriptionState;

/**
 * Exposes publisher services to VMS clients.
 *
 * @hide
 */
interface IVmsPublisherService {
    /**
     * Client call to publish a message.
     */
    oneway void publish(in IBinder token, in VmsLayer layer, int publisherId, in byte[] message) = 0;

    /**
     * Returns the list of VmsLayers that has any clients subscribed to it.
     */
    VmsSubscriptionState getSubscriptions() = 1;

    /**
     * Sets which layers the publisher can publish under which dependencties.
     */
    oneway void setLayersOffering(in IBinder token, in VmsLayersOffering offering) = 2;

    /**
     * The first time a publisher calls this API it will store the publisher info and assigns the
     * publisher an ID. Between reboots, subsequent calls with the same publisher info will
     * return the same ID so that a restarting process can obtain the same ID as it had before.
     */
    int getPublisherId(in byte[] publisherInfo) = 3;
}
