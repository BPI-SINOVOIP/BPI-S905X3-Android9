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

import android.content.Context;
import android.net.MacAddress;
import android.net.wifi.aware.DiscoverySession;
import android.net.wifi.aware.PeerHandle;
import android.net.wifi.aware.PublishConfig;
import android.net.wifi.aware.SubscribeConfig;
import android.net.wifi.aware.WifiAwareSession;
import android.util.Log;
import android.util.Pair;

import com.android.cts.verifier.R;
import com.android.cts.verifier.wifiaware.BaseTestCase;
import com.android.cts.verifier.wifiaware.CallbackUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Base test case providing utilities for Discovery:
 *
 * Subscribe test sequence:
 * 1. Attach
 *    wait for results (session)
 * 2. Subscribe
 *    wait for results (subscribe session)
 * 3. Wait for discovery (possibly with ranging)
 * 4. Send message
 *    Wait for success
 *
 * Publish test sequence:
 * 1. Attach
 *    wait for results (session)
 * 2. Publish
 *    wait for results (publish session)
 * 3. Wait for rx message
 */
public abstract class DiscoveryBaseTestCase extends BaseTestCase {
    private static final String TAG = "DiscoveryBaseTestCase";
    private static final boolean DBG = true;

    private static final String SERVICE_NAME = "CtsVerifierTestService";
    private static final byte[] MATCH_FILTER_BYTES = "bytes used for matching".getBytes();
    private static final byte[] PUB_SSI = "Extra bytes in the publisher discovery".getBytes();
    private static final byte[] SUB_SSI = "Arbitrary bytes for the subscribe discovery".getBytes();
    private static final byte[] MSG_SUB_TO_PUB = "Let's talk".getBytes();
    protected static final int MESSAGE_ID = 1234;
    protected static final int LARGE_ENOUGH_DISTANCE = 100000; // 100 meters

    protected boolean mIsUnsolicited;
    protected boolean mIsRangingRequired;

    protected final Object mLock = new Object();

    private String mFailureReason;
    protected WifiAwareSession mWifiAwareSession;
    protected DiscoverySession mWifiAwareDiscoverySession;
    protected CallbackUtils.DiscoveryCb mDiscoveryCb;
    protected PeerHandle mPeerHandle;
    protected MacAddress mMyMacAddress;
    protected MacAddress mPeerMacAddress;

    public DiscoveryBaseTestCase(Context context, boolean isUnsolicited,
            boolean isRangingRequired) {
        super(context);

        mIsUnsolicited = isUnsolicited;
        mIsRangingRequired = isRangingRequired;
    }

    private boolean executeAttach() throws InterruptedException {
        // attach (optionally with an identity listener)
        CallbackUtils.AttachCb attachCb = new CallbackUtils.AttachCb();
        CallbackUtils.IdentityListenerSingleShot identityL = new CallbackUtils
                .IdentityListenerSingleShot();
        if (mIsRangingRequired) {
            mWifiAwareManager.attach(attachCb, identityL, mHandler);
        } else {
            mWifiAwareManager.attach(attachCb, mHandler);
        }
        Pair<Integer, WifiAwareSession> results = attachCb.waitForAttach();
        switch (results.first) {
            case CallbackUtils.AttachCb.TIMEOUT:
                setFailureReason(mContext.getString(R.string.aware_status_attach_timeout));
                Log.e(TAG, "executeTest: attach TIMEOUT");
                return false;
            case CallbackUtils.AttachCb.ON_ATTACH_FAILED:
                setFailureReason(mContext.getString(R.string.aware_status_attach_fail));
                Log.e(TAG, "executeTest: attach ON_ATTACH_FAILED");
                return false;
        }
        mWifiAwareSession = results.second;
        if (mWifiAwareSession == null) {
            setFailureReason(mContext.getString(R.string.aware_status_attach_fail));
            Log.e(TAG, "executeTest: attach callback succeeded but null session returned!?");
            return false;
        }
        mListener.onTestMsgReceived(mContext.getString(R.string.aware_status_attached));
        if (DBG) {
            Log.d(TAG, "executeTest: attach succeeded");
        }

        // 1.5 optionally wait for identity (necessary in ranging cases)
        if (mIsRangingRequired) {
            byte[] mac = identityL.waitForMac();
            if (mac == null) {
                setFailureReason(mContext.getString(R.string.aware_status_identity_fail));
                Log.e(TAG, "executeAttach: identity callback not triggered");
                return false;
            }
            mMyMacAddress = MacAddress.fromBytes(mac);
            mListener.onTestMsgReceived(mResources.getString(R.string.aware_status_identity,
                    mMyMacAddress));
            if (DBG) {
                Log.d(TAG, "executeAttach: identity received: " + mMyMacAddress.toString());
            }
        }

        return true;
    }

    protected boolean executeSubscribe() throws InterruptedException {
        // 1. attach
        if (!executeAttach()) {
            return false;
        }

        mDiscoveryCb = new CallbackUtils.DiscoveryCb();

        // 2. subscribe
        List<byte[]> matchFilter = new ArrayList<>();
        matchFilter.add(MATCH_FILTER_BYTES);
        SubscribeConfig.Builder builder = new SubscribeConfig.Builder().setServiceName(
                SERVICE_NAME).setServiceSpecificInfo(SUB_SSI).setMatchFilter(
                matchFilter).setSubscribeType(
                mIsUnsolicited ? SubscribeConfig.SUBSCRIBE_TYPE_PASSIVE
                        : SubscribeConfig.SUBSCRIBE_TYPE_ACTIVE).setTerminateNotificationEnabled(
                true);
        if (mIsRangingRequired) {
            // set up a distance that will always trigger - i.e. that we're already in that range
            builder.setMaxDistanceMm(LARGE_ENOUGH_DISTANCE);
        }
        SubscribeConfig subscribeConfig = builder.build();
        if (DBG) Log.d(TAG, "executeTestSubscriber: subscribeConfig=" + subscribeConfig);
        mWifiAwareSession.subscribe(subscribeConfig, mDiscoveryCb, mHandler);

        //    wait for results - subscribe session
        CallbackUtils.DiscoveryCb.CallbackData callbackData = mDiscoveryCb.waitForCallbacks(
                CallbackUtils.DiscoveryCb.ON_SUBSCRIBE_STARTED
                        | CallbackUtils.DiscoveryCb.ON_SESSION_CONFIG_FAILED);
        switch (callbackData.callback) {
            case CallbackUtils.DiscoveryCb.TIMEOUT:
                setFailureReason(mContext.getString(R.string.aware_status_subscribe_timeout));
                Log.e(TAG, "executeTestSubscriber: subscribe TIMEOUT");
                return false;
            case CallbackUtils.DiscoveryCb.ON_SESSION_CONFIG_FAILED:
                setFailureReason(mContext.getString(R.string.aware_status_subscribe_failed));
                Log.e(TAG, "executeTestSubscriber: subscribe ON_SESSION_CONFIG_FAILED");
                return false;
        }
        mWifiAwareDiscoverySession = callbackData.subscribeDiscoverySession;
        if (mWifiAwareDiscoverySession == null) {
            setFailureReason(mContext.getString(R.string.aware_status_subscribe_null_session));
            Log.e(TAG, "executeTestSubscriber: subscribe succeeded but null session returned");
            return false;
        }
        mListener.onTestMsgReceived(mContext.getString(R.string.aware_status_subscribe_started));
        if (DBG) Log.d(TAG, "executeTestSubscriber: subscribe succeeded");

        // 3. wait for discovery
        callbackData = mDiscoveryCb.waitForCallbacks(
                mIsRangingRequired ? CallbackUtils.DiscoveryCb.ON_SERVICE_DISCOVERED_WITH_RANGE
                        : CallbackUtils.DiscoveryCb.ON_SERVICE_DISCOVERED);
        switch (callbackData.callback) {
            case CallbackUtils.DiscoveryCb.TIMEOUT:
                setFailureReason(mContext.getString(R.string.aware_status_discovery_timeout));
                Log.e(TAG, "executeTestSubscriber: waiting for discovery TIMEOUT");
                return false;
        }
        mPeerHandle = callbackData.peerHandle;
        if (!mIsRangingRequired) {
            mListener.onTestMsgReceived(mContext.getString(R.string.aware_status_discovery));
            if (DBG) Log.d(TAG, "executeTestSubscriber: discovery");
        } else {
            if (DBG) {
                Log.d(TAG, "executeTestSubscriber: discovery with range="
                        + callbackData.distanceMm);
            }
        }

        //    validate discovery parameters match
        if (mIsRangingRequired) {
            try {
                mPeerMacAddress = MacAddress.fromBytes(callbackData.serviceSpecificInfo);
            } catch (IllegalArgumentException e) {
                setFailureReason(mContext.getString(R.string.aware_status_discovery_fail));
                Log.e(TAG, "executeTestSubscriber: invalid MAC received in SSI: rx='" + new String(
                        callbackData.serviceSpecificInfo) + "'");
                return false;
            }
            mListener.onTestMsgReceived(
                    mResources.getString(R.string.aware_status_discovery_with_info,
                            mPeerMacAddress));
        } else {
            if (!Arrays.equals(PUB_SSI, callbackData.serviceSpecificInfo)) {
                setFailureReason(mContext.getString(R.string.aware_status_discovery_fail));
                Log.e(TAG, "executeTestSubscriber: discovery but SSI mismatch: rx='" + new String(
                        callbackData.serviceSpecificInfo) + "'");
                return false;
            }
        }
        if (callbackData.matchFilter.size() != 1 || !Arrays.equals(MATCH_FILTER_BYTES,
                callbackData.matchFilter.get(0))) {
            setFailureReason(mContext.getString(R.string.aware_status_discovery_fail));
            StringBuffer sb = new StringBuffer();
            sb.append("size=").append(callbackData.matchFilter.size());
            for (byte[] mf: callbackData.matchFilter) {
                sb.append(", e='").append(new String(mf)).append("'");
            }
            Log.e(TAG, "executeTestSubscriber: discovery but matchFilter mismatch: "
                    + sb.toString());
            return false;
        }
        if (mPeerHandle == null) {
            setFailureReason(mContext.getString(R.string.aware_status_discovery_fail));
            Log.e(TAG, "executeTestSubscriber: discovery but null peerHandle");
            return false;
        }

        // 4. send message & wait for send status
        mWifiAwareDiscoverySession.sendMessage(mPeerHandle, MESSAGE_ID,
                mIsRangingRequired ? mMyMacAddress.toByteArray() : MSG_SUB_TO_PUB);
        callbackData = mDiscoveryCb.waitForCallbacks(
                CallbackUtils.DiscoveryCb.ON_MESSAGE_SEND_SUCCEEDED
                        | CallbackUtils.DiscoveryCb.ON_MESSAGE_SEND_FAILED);
        switch (callbackData.callback) {
            case CallbackUtils.DiscoveryCb.TIMEOUT:
                setFailureReason(mContext.getString(R.string.aware_status_send_timeout));
                Log.e(TAG, "executeTestSubscriber: send message TIMEOUT");
                return false;
            case CallbackUtils.DiscoveryCb.ON_MESSAGE_SEND_FAILED:
                setFailureReason(mContext.getString(R.string.aware_status_send_failed));
                Log.e(TAG, "executeTestSubscriber: send message ON_MESSAGE_SEND_FAILED");
                return false;
        }
        mListener.onTestMsgReceived(mContext.getString(R.string.aware_status_send_success));
        if (DBG) Log.d(TAG, "executeTestSubscriber: send message succeeded");

        if (callbackData.messageId != MESSAGE_ID) {
            setFailureReason(mContext.getString(R.string.aware_status_send_fail_parameter));
            Log.e(TAG, "executeTestSubscriber: send message message ID mismatch: "
                    + callbackData.messageId);
            return false;
        }

        return true;
    }

    protected boolean executePublish() throws InterruptedException {
        // 1. attach
        if (!executeAttach()) {
            return false;
        }

        mDiscoveryCb = new CallbackUtils.DiscoveryCb();

        // 2. publish
        List<byte[]> matchFilter = new ArrayList<>();
        matchFilter.add(MATCH_FILTER_BYTES);
        PublishConfig publishConfig = new PublishConfig.Builder().setServiceName(
                SERVICE_NAME).setServiceSpecificInfo(
                mIsRangingRequired ? mMyMacAddress.toByteArray() : PUB_SSI).setMatchFilter(
                matchFilter).setPublishType(mIsUnsolicited ? PublishConfig.PUBLISH_TYPE_UNSOLICITED
                : PublishConfig.PUBLISH_TYPE_SOLICITED).setTerminateNotificationEnabled(
                true).setRangingEnabled(mIsRangingRequired).build();
        if (DBG) Log.d(TAG, "executeTestPublisher: publishConfig=" + publishConfig);
        mWifiAwareSession.publish(publishConfig, mDiscoveryCb, mHandler);

        //    wait for results - publish session
        CallbackUtils.DiscoveryCb.CallbackData callbackData = mDiscoveryCb.waitForCallbacks(
                CallbackUtils.DiscoveryCb.ON_PUBLISH_STARTED
                        | CallbackUtils.DiscoveryCb.ON_SESSION_CONFIG_FAILED);
        switch (callbackData.callback) {
            case CallbackUtils.DiscoveryCb.TIMEOUT:
                setFailureReason(mContext.getString(R.string.aware_status_publish_timeout));
                Log.e(TAG, "executeTestPublisher: publish TIMEOUT");
                return false;
            case CallbackUtils.DiscoveryCb.ON_SESSION_CONFIG_FAILED:
                setFailureReason(mContext.getString(R.string.aware_status_publish_failed));
                Log.e(TAG, "executeTestPublisher: publish ON_SESSION_CONFIG_FAILED");
                return false;
        }
        mWifiAwareDiscoverySession = callbackData.publishDiscoverySession;
        if (mWifiAwareDiscoverySession == null) {
            setFailureReason(mContext.getString(R.string.aware_status_publish_null_session));
            Log.e(TAG, "executeTestPublisher: publish succeeded but null session returned");
            return false;
        }
        mListener.onTestMsgReceived(mContext.getString(R.string.aware_status_publish_started));
        if (DBG) Log.d(TAG, "executeTestPublisher: publish succeeded");

        // 3. wait to receive message: no timeout since this depends on (human) operator starting
        //    the test on the subscriber device.
        callbackData = mDiscoveryCb.waitForCallbacksNoTimeout(
                CallbackUtils.DiscoveryCb.ON_MESSAGE_RECEIVED);
        mPeerHandle = callbackData.peerHandle;
        mListener.onTestMsgReceived(mContext.getString(R.string.aware_status_received));
        if (DBG) Log.d(TAG, "executeTestPublisher: received message");

        //    validate that received the expected message
        if (mIsRangingRequired) {
            try {
                mPeerMacAddress = MacAddress.fromBytes(callbackData.serviceSpecificInfo);
            } catch (IllegalArgumentException e) {
                setFailureReason(mContext.getString(R.string.aware_status_discovery_fail));
                Log.e(TAG, "executeTestSubscriber: invalid MAC received in SSI: rx='" + new String(
                        callbackData.serviceSpecificInfo) + "'");
                return false;
            }
            mListener.onTestMsgReceived(mResources.getString(R.string.aware_status_received_mac,
                    mPeerMacAddress));
        } else {
            if (!Arrays.equals(MSG_SUB_TO_PUB, callbackData.serviceSpecificInfo)) {
                setFailureReason(mContext.getString(R.string.aware_status_receive_failure));
                Log.e(TAG, "executeTestPublisher: receive message message content mismatch: rx='"
                        + new String(callbackData.serviceSpecificInfo) + "'");
                return false;
            }
        }
        if (mPeerHandle == null) {
            setFailureReason(mContext.getString(R.string.aware_status_receive_failure));
            Log.e(TAG, "executeTestPublisher: received message but peerHandle is null!?");
            return false;
        }

        return true;
    }

    protected void setFailureReason(String reason) {
        synchronized (mLock) {
            mFailureReason = reason;
        }
    }

    @Override
    protected String getFailureReason() {
        synchronized (mLock) {
            return mFailureReason;
        }
    }

    @Override
    protected void tearDown() {
        if (mWifiAwareDiscoverySession != null) {
            mWifiAwareDiscoverySession.close();
            mWifiAwareDiscoverySession = null;
        }
        if (mWifiAwareSession != null) {
            mWifiAwareSession.close();
            mWifiAwareSession = null;
        }
        super.tearDown();
    }
}
