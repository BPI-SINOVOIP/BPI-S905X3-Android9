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

package com.android.cts.verifier.wifiaware.testcase;

import static com.android.cts.verifier.wifiaware.CallbackUtils.CALLBACK_TIMEOUT_SEC;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.util.Log;

import com.android.cts.verifier.R;
import com.android.cts.verifier.wifiaware.CallbackUtils;

import java.util.Arrays;

/**
 * Test case for data-path, in-band test cases:
 * open/passphrase * solicited/unsolicited * publish/subscribe.
 *
 * Subscribe test sequence:
 * 1. Attach
 *    wait for results (session)
 * 2. Subscribe
 *    wait for results (subscribe session)
 * 3. Wait for discovery (possibly with ranging)
 * 4. Send message
 *    Wait for success
 * 5. Wait for rx message
 * 6. Request network
 *    Wait for network
 * 7. Destroy session
 *
 * Publish test sequence:
 * 1. Attach
 *    wait for results (session)
 * 2. Publish
 *    wait for results (publish session)
 * 3. Wait for rx message
 * 4. Request network
 * 5. Send message
 *    Wait for success
 * 6. Wait for network
 * 7. Destroy session
 */
public class DataPathInBandTestCase extends DiscoveryBaseTestCase {
    private static final String TAG = "DataPathInBandTestCase";
    private static final boolean DBG = true;

    private static final byte[] MSG_PUB_TO_SUB = "Ready".getBytes();
    private static final String PASSPHRASE = "Some super secret password";

    private boolean mIsSecurityOpen;
    private boolean mIsPublish;

    public DataPathInBandTestCase(Context context, boolean isSecurityOpen, boolean isPublish,
            boolean isUnsolicited) {
        super(context, isUnsolicited, false);

        mIsSecurityOpen = isSecurityOpen;
        mIsPublish = isPublish;
    }

    @Override
    protected boolean executeTest() throws InterruptedException {
        if (DBG) {
            Log.d(TAG,
                    "executeTest: mIsSecurityOpen=" + mIsSecurityOpen + ", mIsPublish=" + mIsPublish
                            + ", mIsUnsolicited=" + mIsUnsolicited);
        }

        boolean success;
        if (mIsPublish) {
            success = executeTestPublisher();
        } else {
            success = executeTestSubscriber();
        }
        if (!success) {
            return false;
        }

        // destroy session
        mWifiAwareDiscoverySession.close();
        mWifiAwareDiscoverySession = null;

        mListener.onTestMsgReceived(mContext.getString(R.string.aware_status_lifecycle_ok));
        return true;
    }

    private boolean executeTestSubscriber() throws InterruptedException {
        if (DBG) Log.d(TAG, "executeTestSubscriber");
        if (!executeSubscribe()) {
            return false;
        }

        // 5. wait to receive message
        CallbackUtils.DiscoveryCb.CallbackData callbackData = mDiscoveryCb.waitForCallbacks(
                CallbackUtils.DiscoveryCb.ON_MESSAGE_RECEIVED);
        switch (callbackData.callback) {
            case CallbackUtils.DiscoveryCb.TIMEOUT:
                setFailureReason(mContext.getString(R.string.aware_status_receive_timeout));
                Log.e(TAG, "executeTestSubscriber: receive message TIMEOUT");
                return false;
        }
        mListener.onTestMsgReceived(mContext.getString(R.string.aware_status_received));
        if (DBG) Log.d(TAG, "executeTestSubscriber: received message");

        //    validate that received the expected message
        if (!Arrays.equals(MSG_PUB_TO_SUB, callbackData.serviceSpecificInfo)) {
            setFailureReason(mContext.getString(R.string.aware_status_receive_failure));
            Log.e(TAG, "executeTestSubscriber: receive message message content mismatch: rx='"
                    + new String(callbackData.serviceSpecificInfo) + "'");
            return false;
        }

        // 6. request network
        ConnectivityManager cm = (ConnectivityManager) mContext.getSystemService(
                Context.CONNECTIVITY_SERVICE);
        NetworkRequest nr = new NetworkRequest.Builder().addTransportType(
                NetworkCapabilities.TRANSPORT_WIFI_AWARE).setNetworkSpecifier(
                mIsSecurityOpen ? mWifiAwareDiscoverySession.createNetworkSpecifierOpen(mPeerHandle)
                        : mWifiAwareDiscoverySession.createNetworkSpecifierPassphrase(mPeerHandle,
                                PASSPHRASE)).build();
        CallbackUtils.NetworkCb networkCb = new CallbackUtils.NetworkCb();
        cm.requestNetwork(nr, networkCb, CALLBACK_TIMEOUT_SEC * 1000);
        mListener.onTestMsgReceived(
                mContext.getString(R.string.aware_status_network_requested));
        if (DBG) Log.d(TAG, "executeTestSubscriber: requested network");
        boolean networkAvailable = networkCb.waitForNetwork();
        cm.unregisterNetworkCallback(networkCb);
        if (!networkAvailable) {
            setFailureReason(mContext.getString(R.string.aware_status_network_failed));
            Log.e(TAG, "executeTestSubscriber: network request rejected - ON_UNAVAILABLE");
            return false;
        }
        mListener.onTestMsgReceived(mContext.getString(R.string.aware_status_network_success));
        if (DBG) Log.d(TAG, "executeTestSubscriber: network request granted - AVAILABLE");

        return true;
    }

    private boolean executeTestPublisher() throws InterruptedException {
        if (DBG) Log.d(TAG, "executeTestPublisher");
        if (!executePublish()) {
            return false;
        }

        // 4. Request network
        ConnectivityManager cm = (ConnectivityManager) mContext.getSystemService(
                Context.CONNECTIVITY_SERVICE);
        NetworkRequest nr = new NetworkRequest.Builder().addTransportType(
                NetworkCapabilities.TRANSPORT_WIFI_AWARE).setNetworkSpecifier(
                mIsSecurityOpen ? mWifiAwareDiscoverySession.createNetworkSpecifierOpen(mPeerHandle)
                        : mWifiAwareDiscoverySession.createNetworkSpecifierPassphrase(mPeerHandle,
                                PASSPHRASE)).build();
        CallbackUtils.NetworkCb networkCb = new CallbackUtils.NetworkCb();
        cm.requestNetwork(nr, networkCb, CALLBACK_TIMEOUT_SEC * 1000);
        mListener.onTestMsgReceived(
                mContext.getString(R.string.aware_status_network_requested));
        if (DBG) Log.d(TAG, "executeTestPublisher: requested network");

        // 5. send message & wait for send status
        mWifiAwareDiscoverySession.sendMessage(mPeerHandle, MESSAGE_ID, MSG_PUB_TO_SUB);
        CallbackUtils.DiscoveryCb.CallbackData callbackData = mDiscoveryCb.waitForCallbacks(
                CallbackUtils.DiscoveryCb.ON_MESSAGE_SEND_SUCCEEDED
                        | CallbackUtils.DiscoveryCb.ON_MESSAGE_SEND_FAILED);
        switch (callbackData.callback) {
            case CallbackUtils.DiscoveryCb.TIMEOUT:
                setFailureReason(mContext.getString(R.string.aware_status_send_timeout));
                Log.e(TAG, "executeTestPublisher: send message TIMEOUT");
                return false;
            case CallbackUtils.DiscoveryCb.ON_MESSAGE_SEND_FAILED:
                setFailureReason(mContext.getString(R.string.aware_status_send_failed));
                Log.e(TAG, "executeTestPublisher: send message ON_MESSAGE_SEND_FAILED");
                return false;
        }
        mListener.onTestMsgReceived(mContext.getString(R.string.aware_status_send_success));
        if (DBG) Log.d(TAG, "executeTestPublisher: send message succeeded");

        if (callbackData.messageId != MESSAGE_ID) {
            setFailureReason(mContext.getString(R.string.aware_status_send_fail_parameter));
            Log.e(TAG, "executeTestPublisher: send message succeeded but message ID mismatch : "
                    + callbackData.messageId);
            return false;
        }

        // 6. wait for network
        boolean networkAvailable = networkCb.waitForNetwork();
        cm.unregisterNetworkCallback(networkCb);
        if (!networkAvailable) {
            setFailureReason(mContext.getString(R.string.aware_status_network_failed));
            Log.e(TAG, "executeTestPublisher: request network rejected - ON_UNAVAILABLE");
            return false;
        }
        mListener.onTestMsgReceived(mContext.getString(R.string.aware_status_network_success));
        if (DBG) Log.d(TAG, "executeTestPublisher: network request granted - AVAILABLE");

        return true;
    }
}
