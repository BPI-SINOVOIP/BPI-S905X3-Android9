/*
 * Copyright (C) 2008 The Android Open Source Project
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
 
package com.android.phone;

import com.android.phone.INetworkQueryServiceCallback;
 
/**
 * Service interface to handle queries for available networks.  The
 * Phone application lets this service interface handle carrier 
 * availability queries instead of making direct calls to the Phone layer.
 */
oneway interface INetworkQueryService {
 
    /**
     * Starts a network query if it has not been started yet, and
     * request a callback through the INetworkQueryServiceCallback
     * object on query completion.  If there is an existing request,
     * then just add the callback to the list of notifications
     * that will be sent upon query completion.
     *
     * It will send the network query with the use of
     * <code>TelephonyManager.requestNetworkScan()</code> if the
     * isIncrementalResult is true. And if the isIncrementalResult
     * is set as false, it will try to send network query through
     * <code>Phone.getAvailableNetworks()</code>.
     */
    void startNetworkQuery(in INetworkQueryServiceCallback cb, in int phoneId, boolean isIncrementalResult);

    /**
     * Tells the service that the requested query is to be ignored.
     * This may not do anything for the Query request in the
     * underlying RIL, but it ensures that the callback is removed
     * from the list of notifications.
     */
    void stopNetworkQuery();

    /**
     * Tells the service to unregister the network query callback.
     * Will not attempt to stop an ongoing network query.
     * Functionally may be the same as stopNetworkQuery since that
     * function also does not stop a query request in the underlying
     * RIL.
     */
    void unregisterCallback(in INetworkQueryServiceCallback cb);
}
