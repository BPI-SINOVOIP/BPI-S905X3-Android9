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

package com.googlecode.android_scripting.facade.wifi;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.net.NetworkSpecifier;
import android.net.wifi.RttManager;
import android.net.wifi.RttManager.RttResult;
import android.net.wifi.aware.AttachCallback;
import android.net.wifi.aware.ConfigRequest;
import android.net.wifi.aware.DiscoverySession;
import android.net.wifi.aware.DiscoverySessionCallback;
import android.net.wifi.aware.IdentityChangedListener;
import android.net.wifi.aware.PeerHandle;
import android.net.wifi.aware.PublishConfig;
import android.net.wifi.aware.PublishDiscoverySession;
import android.net.wifi.aware.SubscribeConfig;
import android.net.wifi.aware.SubscribeDiscoverySession;
import android.net.wifi.aware.TlvBufferUtils;
import android.net.wifi.aware.WifiAwareManager;
import android.net.wifi.aware.WifiAwareNetworkSpecifier;
import android.net.wifi.aware.WifiAwareSession;
import android.os.Bundle;
import android.os.Parcelable;
import android.os.Process;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Base64;
import android.util.SparseArray;

import com.android.internal.annotations.GuardedBy;

import libcore.util.HexEncoding;

import com.googlecode.android_scripting.facade.EventFacade;
import com.googlecode.android_scripting.facade.FacadeManager;
import com.googlecode.android_scripting.jsonrpc.RpcReceiver;
import com.googlecode.android_scripting.rpc.Rpc;
import com.googlecode.android_scripting.rpc.RpcOptional;
import com.googlecode.android_scripting.rpc.RpcParameter;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

/**
 * WifiAwareManager functions.
 */
public class WifiAwareManagerFacade extends RpcReceiver {
    private final Service mService;
    private final EventFacade mEventFacade;
    private final WifiAwareStateChangedReceiver mStateChangedReceiver;

    private final Object mLock = new Object(); // lock access to the following vars

    @GuardedBy("mLock")
    private WifiAwareManager mMgr;

    @GuardedBy("mLock")
    private int mNextDiscoverySessionId = 1;
    @GuardedBy("mLock")
    private SparseArray<DiscoverySession> mDiscoverySessions = new SparseArray<>();
    private int getNextDiscoverySessionId() {
        synchronized (mLock) {
            return mNextDiscoverySessionId++;
        }
    }

    @GuardedBy("mLock")
    private int mNextSessionId = 1;
    @GuardedBy("mLock")
    private SparseArray<WifiAwareSession> mSessions = new SparseArray<>();
    private int getNextSessionId() {
        synchronized (mLock) {
            return mNextSessionId++;
        }
    }

    @GuardedBy("mLock")
    private SparseArray<Long> mMessageStartTime = new SparseArray<>();

    private static final String NS_KEY_TYPE = "type";
    private static final String NS_KEY_ROLE = "role";
    private static final String NS_KEY_CLIENT_ID = "client_id";
    private static final String NS_KEY_SESSION_ID = "session_id";
    private static final String NS_KEY_PEER_ID = "peer_id";
    private static final String NS_KEY_PEER_MAC = "peer_mac";
    private static final String NS_KEY_PMK = "pmk";
    private static final String NS_KEY_PASSPHRASE = "passphrase";

    private static String getJsonString(WifiAwareNetworkSpecifier ns) throws JSONException {
        JSONObject j = new JSONObject();

        j.put(NS_KEY_TYPE, ns.type);
        j.put(NS_KEY_ROLE, ns.role);
        j.put(NS_KEY_CLIENT_ID, ns.clientId);
        j.put(NS_KEY_SESSION_ID, ns.sessionId);
        j.put(NS_KEY_PEER_ID, ns.peerId);
        if (ns.peerMac != null) {
            j.put(NS_KEY_PEER_MAC, Base64.encodeToString(ns.peerMac, Base64.DEFAULT));
        }
        if (ns.pmk != null) {
            j.put(NS_KEY_PMK, Base64.encodeToString(ns.pmk, Base64.DEFAULT));
        }
        if (ns.passphrase != null) {
            j.put(NS_KEY_PASSPHRASE, ns.passphrase);
        }

        return j.toString();
    }

    public static NetworkSpecifier getNetworkSpecifier(JSONObject j) throws JSONException {
        if (j == null) {
            return null;
        }

        int type = 0, role = 0, clientId = 0, sessionId = 0, peerId = 0;
        byte[] peerMac = null;
        byte[] pmk = null;
        String passphrase = null;

        if (j.has(NS_KEY_TYPE)) {
            type = j.getInt((NS_KEY_TYPE));
        }
        if (j.has(NS_KEY_ROLE)) {
            role = j.getInt((NS_KEY_ROLE));
        }
        if (j.has(NS_KEY_CLIENT_ID)) {
            clientId = j.getInt((NS_KEY_CLIENT_ID));
        }
        if (j.has(NS_KEY_SESSION_ID)) {
            sessionId = j.getInt((NS_KEY_SESSION_ID));
        }
        if (j.has(NS_KEY_PEER_ID)) {
            peerId = j.getInt((NS_KEY_PEER_ID));
        }
        if (j.has(NS_KEY_PEER_MAC)) {
            peerMac = Base64.decode(j.getString(NS_KEY_PEER_MAC), Base64.DEFAULT);
        }
        if (j.has(NS_KEY_PMK)) {
            pmk = Base64.decode(j.getString(NS_KEY_PMK), Base64.DEFAULT);
        }
        if (j.has(NS_KEY_PASSPHRASE)) {
            passphrase = j.getString((NS_KEY_PASSPHRASE));
        }

        return new WifiAwareNetworkSpecifier(type, role, clientId, sessionId, peerId, peerMac, pmk,
                passphrase, Process.myUid());
    }

    private static String getStringOrNull(JSONObject j, String name) throws JSONException {
        if (j.isNull(name)) {
            return null;
        }
        return j.getString(name);
    }

    private static ConfigRequest getConfigRequest(JSONObject j) throws JSONException {
        if (j == null) {
            return null;
        }

        ConfigRequest.Builder builder = new ConfigRequest.Builder();

        if (j.has("Support5gBand")) {
            builder.setSupport5gBand(j.getBoolean("Support5gBand"));
        }
        if (j.has("MasterPreference")) {
            builder.setMasterPreference(j.getInt("MasterPreference"));
        }
        if (j.has("ClusterLow")) {
            builder.setClusterLow(j.getInt("ClusterLow"));
        }
        if (j.has("ClusterHigh")) {
            builder.setClusterHigh(j.getInt("ClusterHigh"));
        }
        if (j.has("DiscoveryWindowInterval")) {
            JSONArray interval = j.getJSONArray("DiscoveryWindowInterval");
            if (interval.length() != 2) {
                throw new JSONException(
                        "Expect 'DiscoveryWindowInterval' to be an array with 2 elements!");
            }
            int intervalValue = interval.getInt(ConfigRequest.NAN_BAND_24GHZ);
            if (intervalValue != ConfigRequest.DW_INTERVAL_NOT_INIT) {
                builder.setDiscoveryWindowInterval(ConfigRequest.NAN_BAND_24GHZ, intervalValue);
            }
            intervalValue = interval.getInt(ConfigRequest.NAN_BAND_5GHZ);
            if (intervalValue != ConfigRequest.DW_INTERVAL_NOT_INIT) {
                builder.setDiscoveryWindowInterval(ConfigRequest.NAN_BAND_5GHZ, intervalValue);
            }
        }

        return builder.build();
    }

    private static List<byte[]> getMatchFilter(JSONArray ja) throws JSONException {
        List<byte[]> la = new ArrayList<>();
        for (int i = 0; i < ja.length(); ++i) {
            la.add(Base64.decode(ja.getString(i).getBytes(StandardCharsets.UTF_8), Base64.DEFAULT));
        }
        return la;
    }

    private static PublishConfig getPublishConfig(JSONObject j) throws JSONException {
        if (j == null) {
            return null;
        }

        PublishConfig.Builder builder = new PublishConfig.Builder();

        if (j.has("ServiceName")) {
            builder.setServiceName(getStringOrNull(j, "ServiceName"));
        }

        if (j.has("ServiceSpecificInfo")) {
            String ssi = getStringOrNull(j, "ServiceSpecificInfo");
            if (ssi != null) {
                builder.setServiceSpecificInfo(ssi.getBytes());
            }
        }

        if (j.has("MatchFilter")) {
            byte[] bytes = Base64.decode(
                    j.getString("MatchFilter").getBytes(StandardCharsets.UTF_8), Base64.DEFAULT);
            List<byte[]> mf = new TlvBufferUtils.TlvIterable(0, 1, bytes).toList();
            builder.setMatchFilter(mf);

        }

        if (!j.isNull("MatchFilterList")) {
            builder.setMatchFilter(getMatchFilter(j.getJSONArray("MatchFilterList")));
        }

        if (j.has("DiscoveryType")) {
            builder.setPublishType(j.getInt("DiscoveryType"));
        }
        if (j.has("TtlSec")) {
            builder.setTtlSec(j.getInt("TtlSec"));
        }
        if (j.has("TerminateNotificationEnabled")) {
            builder.setTerminateNotificationEnabled(j.getBoolean("TerminateNotificationEnabled"));
        }
        if (j.has("RangingEnabled")) {
            builder.setRangingEnabled(j.getBoolean("RangingEnabled"));
        }


        return builder.build();
    }

    private static SubscribeConfig getSubscribeConfig(JSONObject j) throws JSONException {
        if (j == null) {
            return null;
        }

        SubscribeConfig.Builder builder = new SubscribeConfig.Builder();

        if (j.has("ServiceName")) {
            builder.setServiceName(j.getString("ServiceName"));
        }

        if (j.has("ServiceSpecificInfo")) {
            String ssi = getStringOrNull(j, "ServiceSpecificInfo");
            if (ssi != null) {
                builder.setServiceSpecificInfo(ssi.getBytes());
            }
        }

        if (j.has("MatchFilter")) {
            byte[] bytes = Base64.decode(
                    j.getString("MatchFilter").getBytes(StandardCharsets.UTF_8), Base64.DEFAULT);
            List<byte[]> mf = new TlvBufferUtils.TlvIterable(0, 1, bytes).toList();
            builder.setMatchFilter(mf);
        }

        if (!j.isNull("MatchFilterList")) {
            builder.setMatchFilter(getMatchFilter(j.getJSONArray("MatchFilterList")));
        }

        if (j.has("DiscoveryType")) {
            builder.setSubscribeType(j.getInt("DiscoveryType"));
        }
        if (j.has("TtlSec")) {
            builder.setTtlSec(j.getInt("TtlSec"));
        }
        if (j.has("TerminateNotificationEnabled")) {
            builder.setTerminateNotificationEnabled(j.getBoolean("TerminateNotificationEnabled"));
        }
        if (j.has("MinDistanceMm")) {
            builder.setMinDistanceMm(j.getInt("MinDistanceMm"));
        }
        if (j.has("MaxDistanceMm")) {
            builder.setMaxDistanceMm(j.getInt("MaxDistanceMm"));
        }

        return builder.build();
    }

    public WifiAwareManagerFacade(FacadeManager manager) {
        super(manager);
        mService = manager.getService();

        mMgr = (WifiAwareManager) mService.getSystemService(Context.WIFI_AWARE_SERVICE);

        mEventFacade = manager.getReceiver(EventFacade.class);

        mStateChangedReceiver = new WifiAwareStateChangedReceiver();
        IntentFilter filter = new IntentFilter(WifiAwareManager.ACTION_WIFI_AWARE_STATE_CHANGED);
        mService.registerReceiver(mStateChangedReceiver, filter);
    }

    @Override
    public void shutdown() {
        wifiAwareDestroyAll();
        mService.unregisterReceiver(mStateChangedReceiver);
    }

    @Rpc(description = "Does the device support the Wi-Fi Aware feature?")
    public Boolean doesDeviceSupportWifiAwareFeature() {
        return mService.getPackageManager().hasSystemFeature(PackageManager.FEATURE_WIFI_AWARE);
    }

    @Rpc(description = "Is Aware Usage Enabled?")
    public Boolean wifiIsAwareAvailable() throws RemoteException {
        synchronized (mLock) {
            return mMgr.isAvailable();
        }
    }

    @Rpc(description = "Destroy all Aware sessions and discovery sessions")
    public void wifiAwareDestroyAll() {
        synchronized (mLock) {
            for (int i = 0; i < mSessions.size(); ++i) {
                mSessions.valueAt(i).close();
            }
            mSessions.clear();

            /* discovery sessions automatically destroyed when containing Aware sessions
             * destroyed */
            mDiscoverySessions.clear();

            mMessageStartTime.clear();
        }
    }

    @Rpc(description = "Attach to Aware.")
    public Integer wifiAwareAttach(
            @RpcParameter(name = "identityCb",
                description = "Controls whether an identity callback is provided")
                @RpcOptional Boolean identityCb,
            @RpcParameter(name = "awareConfig",
                description = "The session configuration, or null for default config")
                @RpcOptional JSONObject awareConfig,
            @RpcParameter(name = "useIdInCallbackEvent",
                description =
                    "Specifies whether the callback events should be decorated with session Id")
                @RpcOptional Boolean useIdInCallbackEvent)
            throws RemoteException, JSONException {
        synchronized (mLock) {
            int sessionId = getNextSessionId();
            boolean useIdInCallbackEventName =
                (useIdInCallbackEvent != null) ? useIdInCallbackEvent : false;
            mMgr.attach(null, getConfigRequest(awareConfig),
                    new AwareAttachCallbackPostsEvents(sessionId, useIdInCallbackEventName),
                    (identityCb != null && identityCb.booleanValue())
                        ? new AwareIdentityChangeListenerPostsEvents(sessionId,
                        useIdInCallbackEventName) : null);
            return sessionId;
        }
    }

    @Rpc(description = "Destroy a Aware session.")
    public void wifiAwareDestroy(
            @RpcParameter(name = "clientId", description = "The client ID returned when a connection was created") Integer clientId)
            throws RemoteException, JSONException {
        WifiAwareSession session;
        synchronized (mLock) {
            session = mSessions.get(clientId);
        }
        if (session == null) {
            throw new IllegalStateException(
                    "Calling WifiAwareDisconnect before session (client ID " + clientId
                            + ") is ready/or already disconnected");
        }
        session.close();
    }

    @Rpc(description = "Publish.")
    public Integer wifiAwarePublish(
            @RpcParameter(name = "clientId", description = "The client ID returned when a connection was created") Integer clientId,
            @RpcParameter(name = "publishConfig") JSONObject publishConfig,
            @RpcParameter(name = "useIdInCallbackEvent",
            description =
                "Specifies whether the callback events should be decorated with session Id")
                @RpcOptional Boolean useIdInCallbackEvent)
            throws RemoteException, JSONException {
        synchronized (mLock) {
            WifiAwareSession session = mSessions.get(clientId);
            if (session == null) {
                throw new IllegalStateException(
                        "Calling WifiAwarePublish before session (client ID " + clientId
                                + ") is ready/or already disconnected");
            }
            boolean useIdInCallbackEventName =
                (useIdInCallbackEvent != null) ? useIdInCallbackEvent : false;

            int discoverySessionId = getNextDiscoverySessionId();
            session.publish(getPublishConfig(publishConfig),
                new AwareDiscoverySessionCallbackPostsEvents(discoverySessionId,
                    useIdInCallbackEventName), null);
            return discoverySessionId;
        }
    }

    @Rpc(description = "Update Publish.")
    public void wifiAwareUpdatePublish(
        @RpcParameter(name = "sessionId", description = "The discovery session ID")
            Integer sessionId,
        @RpcParameter(name = "publishConfig", description = "Publish configuration")
            JSONObject publishConfig)
        throws RemoteException, JSONException {
        synchronized (mLock) {
            DiscoverySession session = mDiscoverySessions.get(sessionId);
            if (session == null) {
                throw new IllegalStateException(
                    "Calling wifiAwareUpdatePublish before session (session ID "
                        + sessionId + ") is ready");
            }
            if (!(session instanceof PublishDiscoverySession)) {
                throw new IllegalArgumentException(
                    "Calling wifiAwareUpdatePublish with a subscribe session ID");
            }
            ((PublishDiscoverySession) session).updatePublish(getPublishConfig(publishConfig));
        }
    }

    @Rpc(description = "Subscribe.")
    public Integer wifiAwareSubscribe(
            @RpcParameter(name = "clientId", description = "The client ID returned when a connection was created") Integer clientId,
            @RpcParameter(name = "subscribeConfig") JSONObject subscribeConfig,
            @RpcParameter(name = "useIdInCallbackEvent",
                description =
                "Specifies whether the callback events should be decorated with session Id")
                @RpcOptional Boolean useIdInCallbackEvent)
            throws RemoteException, JSONException {
        synchronized (mLock) {
            WifiAwareSession session = mSessions.get(clientId);
            if (session == null) {
                throw new IllegalStateException(
                        "Calling WifiAwareSubscribe before session (client ID " + clientId
                                + ") is ready/or already disconnected");
            }
            boolean useIdInCallbackEventName =
                (useIdInCallbackEvent != null) ? useIdInCallbackEvent : false;

            int discoverySessionId = getNextDiscoverySessionId();
            session.subscribe(getSubscribeConfig(subscribeConfig),
                new AwareDiscoverySessionCallbackPostsEvents(discoverySessionId,
                    useIdInCallbackEventName), null);
            return discoverySessionId;
        }
    }

    @Rpc(description = "Update Subscribe.")
    public void wifiAwareUpdateSubscribe(
        @RpcParameter(name = "sessionId", description = "The discovery session ID")
            Integer sessionId,
        @RpcParameter(name = "subscribeConfig", description = "Subscribe configuration")
            JSONObject subscribeConfig)
        throws RemoteException, JSONException {
        synchronized (mLock) {
            DiscoverySession session = mDiscoverySessions.get(sessionId);
            if (session == null) {
                throw new IllegalStateException(
                    "Calling wifiAwareUpdateSubscribe before session (session ID "
                        + sessionId + ") is ready");
            }
            if (!(session instanceof SubscribeDiscoverySession)) {
                throw new IllegalArgumentException(
                    "Calling wifiAwareUpdateSubscribe with a publish session ID");
            }
            ((SubscribeDiscoverySession) session)
                .updateSubscribe(getSubscribeConfig(subscribeConfig));
        }
    }

    @Rpc(description = "Destroy a discovery Session.")
    public void wifiAwareDestroyDiscoverySession(
            @RpcParameter(name = "sessionId", description = "The discovery session ID returned when session was created using publish or subscribe") Integer sessionId)
            throws RemoteException {
        synchronized (mLock) {
            DiscoverySession session = mDiscoverySessions.get(sessionId);
            if (session == null) {
                throw new IllegalStateException(
                        "Calling WifiAwareTerminateSession before session (session ID "
                                + sessionId + ") is ready");
            }
            session.close();
            mDiscoverySessions.remove(sessionId);
        }
    }

    @Rpc(description = "Send peer-to-peer Aware message")
    public void wifiAwareSendMessage(
            @RpcParameter(name = "sessionId", description = "The session ID returned when session"
                    + " was created using publish or subscribe") Integer sessionId,
            @RpcParameter(name = "peerId", description = "The ID of the peer being communicated "
                    + "with. Obtained from a previous message or match session.") Integer peerId,
            @RpcParameter(name = "messageId", description = "Arbitrary handle used for "
                    + "identification of the message in the message status callbacks")
                    Integer messageId,
            @RpcParameter(name = "message") String message,
            @RpcParameter(name = "retryCount", description = "Number of retries (0 for none) if "
                    + "transmission fails due to no ACK reception") Integer retryCount)
                    throws RemoteException {
        DiscoverySession session;
        synchronized (mLock) {
            session = mDiscoverySessions.get(sessionId);
        }
        if (session == null) {
            throw new IllegalStateException(
                    "Calling WifiAwareSendMessage before session (session ID " + sessionId
                            + " is ready");
        }
        byte[] bytes = null;
        if (message != null) {
            bytes = message.getBytes();
        }

        synchronized (mLock) {
            mMessageStartTime.put(messageId, System.currentTimeMillis());
        }
        session.sendMessage(new PeerHandle(peerId), messageId, bytes, retryCount);
    }

    @Rpc(description = "Create a network specifier to be used when specifying a Aware network request")
    public String wifiAwareCreateNetworkSpecifier(
            @RpcParameter(name = "sessionId", description = "The session ID returned when session was created using publish or subscribe")
                    Integer sessionId,
            @RpcParameter(name = "peerId", description = "The ID of the peer (obtained through OnMatch or OnMessageReceived")
                    Integer peerId,
            @RpcParameter(name = "passphrase",
                description = "Passphrase of the data-path. Optional, can be empty/null.")
                @RpcOptional String passphrase,
            @RpcParameter(name = "pmk",
                description = "PMK of the data-path (base64 encoded). Optional, can be empty/null.")
                @RpcOptional String pmk)
        throws JSONException {
        DiscoverySession session;
        synchronized (mLock) {
            session = mDiscoverySessions.get(sessionId);
        }
        if (session == null) {
            throw new IllegalStateException(
                    "Calling wifiAwareCreateNetworkSpecifier before session (session ID "
                            + sessionId + " is ready");
        }
        PeerHandle peerHandle = null;
        if (peerId != null) {
            peerHandle = new PeerHandle(peerId);
        }
        byte[] pmkDecoded = null;
        if (!TextUtils.isEmpty(pmk)) {
            pmkDecoded = Base64.decode(pmk, Base64.DEFAULT);
        }

        WifiAwareNetworkSpecifier ns = new WifiAwareNetworkSpecifier(
                (peerHandle == null) ? WifiAwareNetworkSpecifier.NETWORK_SPECIFIER_TYPE_IB_ANY_PEER
                        : WifiAwareNetworkSpecifier.NETWORK_SPECIFIER_TYPE_IB,
                session instanceof SubscribeDiscoverySession
                        ? WifiAwareManager.WIFI_AWARE_DATA_PATH_ROLE_INITIATOR
                        : WifiAwareManager.WIFI_AWARE_DATA_PATH_ROLE_RESPONDER,
                session.getClientId(),
                session.getSessionId(),
                peerHandle != null ? peerHandle.peerId : 0, // 0 is an invalid peer ID
                null, // peerMac (not used in this method)
                pmkDecoded,
                passphrase,
                Process.myUid());

        return getJsonString(ns);
    }

    @Rpc(description = "Create a network specifier to be used when specifying an OOB Aware network request")
    public String wifiAwareCreateNetworkSpecifierOob(
            @RpcParameter(name = "clientId",
                    description = "The client ID")
                    Integer clientId,
            @RpcParameter(name = "role", description = "The role: INITIATOR(0), RESPONDER(1)")
                    Integer role,
            @RpcParameter(name = "peerMac",
                    description = "The MAC address of the peer")
                    String peerMac,
            @RpcParameter(name = "passphrase",
                    description = "Passphrase of the data-path. Optional, can be empty/null.")
            @RpcOptional String passphrase,
            @RpcParameter(name = "pmk",
                    description = "PMK of the data-path (base64). Optional, can be empty/null.")
            @RpcOptional String pmk) throws JSONException {
        WifiAwareSession session;
        synchronized (mLock) {
            session = mSessions.get(clientId);
        }
        if (session == null) {
            throw new IllegalStateException(
                    "Calling wifiAwareCreateNetworkSpecifierOob before session (client ID "
                            + clientId + " is ready");
        }
        byte[] peerMacBytes = null;
        if (peerMac != null) {
            peerMacBytes = HexEncoding.decode(peerMac.toCharArray(), false);
        }
        byte[] pmkDecoded = null;
        if (!TextUtils.isEmpty(pmk)) {
            pmkDecoded = Base64.decode(pmk, Base64.DEFAULT);
        }

        WifiAwareNetworkSpecifier ns = new WifiAwareNetworkSpecifier(
                (peerMacBytes == null) ?
                        WifiAwareNetworkSpecifier.NETWORK_SPECIFIER_TYPE_OOB_ANY_PEER
                        : WifiAwareNetworkSpecifier.NETWORK_SPECIFIER_TYPE_OOB,
                role,
                session.getClientId(),
                0, // 0 is an invalid session ID
                0, // 0 is an invalid peer ID
                peerMacBytes,
                pmkDecoded,
                passphrase,
                Process.myUid());

        return getJsonString(ns);
    }

    private class AwareAttachCallbackPostsEvents extends AttachCallback {
        private int mSessionId;
        private long mCreateTimestampMs;
        private boolean mUseIdInCallbackEventName;

        public AwareAttachCallbackPostsEvents(int sessionId, boolean useIdInCallbackEventName) {
            mSessionId = sessionId;
            mCreateTimestampMs = System.currentTimeMillis();
            mUseIdInCallbackEventName = useIdInCallbackEventName;
        }

        @Override
        public void onAttached(WifiAwareSession session) {
            synchronized (mLock) {
                mSessions.put(mSessionId, session);
            }

            Bundle mResults = new Bundle();
            mResults.putInt("sessionId", mSessionId);
            mResults.putLong("latencyMs", System.currentTimeMillis() - mCreateTimestampMs);
            mResults.putLong("timestampMs", System.currentTimeMillis());
            if (mUseIdInCallbackEventName) {
                mEventFacade.postEvent("WifiAwareOnAttached_" + mSessionId, mResults);
            } else {
                mEventFacade.postEvent("WifiAwareOnAttached", mResults);
            }
        }

        @Override
        public void onAttachFailed() {
            Bundle mResults = new Bundle();
            mResults.putInt("sessionId", mSessionId);
            mResults.putLong("latencyMs", System.currentTimeMillis() - mCreateTimestampMs);
            if (mUseIdInCallbackEventName) {
                mEventFacade.postEvent("WifiAwareOnAttachFailed_" + mSessionId, mResults);
            } else {
                mEventFacade.postEvent("WifiAwareOnAttachFailed", mResults);
            }
        }
    }

    private class AwareIdentityChangeListenerPostsEvents extends IdentityChangedListener {
        private int mSessionId;
        private boolean mUseIdInCallbackEventName;

        public AwareIdentityChangeListenerPostsEvents(int sessionId,
            boolean useIdInCallbackEventName) {
            mSessionId = sessionId;
            mUseIdInCallbackEventName = useIdInCallbackEventName;
        }

        @Override
        public void onIdentityChanged(byte[] mac) {
            Bundle mResults = new Bundle();
            mResults.putInt("sessionId", mSessionId);
            mResults.putString("mac", String.valueOf(HexEncoding.encode(mac)));
            mResults.putLong("timestampMs", System.currentTimeMillis());
            if (mUseIdInCallbackEventName) {
                mEventFacade.postEvent("WifiAwareOnIdentityChanged_" + mSessionId, mResults);
            } else {
                mEventFacade.postEvent("WifiAwareOnIdentityChanged", mResults);
            }
        }
    }

    private class AwareDiscoverySessionCallbackPostsEvents extends
            DiscoverySessionCallback {
        private int mDiscoverySessionId;
        private boolean mUseIdInCallbackEventName;
        private long mCreateTimestampMs;

        public AwareDiscoverySessionCallbackPostsEvents(int discoverySessionId,
                boolean useIdInCallbackEventName) {
            mDiscoverySessionId = discoverySessionId;
            mUseIdInCallbackEventName = useIdInCallbackEventName;
            mCreateTimestampMs = System.currentTimeMillis();
        }

        private void postEvent(String eventName, Bundle results) {
            String finalEventName = eventName;
            if (mUseIdInCallbackEventName) {
                finalEventName += "_" + mDiscoverySessionId;
            }

            mEventFacade.postEvent(finalEventName, results);
        }

        @Override
        public void onPublishStarted(PublishDiscoverySession discoverySession) {
            synchronized (mLock) {
                mDiscoverySessions.put(mDiscoverySessionId, discoverySession);
            }

            Bundle mResults = new Bundle();
            mResults.putInt("discoverySessionId", mDiscoverySessionId);
            mResults.putLong("latencyMs", System.currentTimeMillis() - mCreateTimestampMs);
            mResults.putLong("timestampMs", System.currentTimeMillis());
            postEvent("WifiAwareSessionOnPublishStarted", mResults);
        }

        @Override
        public void onSubscribeStarted(SubscribeDiscoverySession discoverySession) {
            synchronized (mLock) {
                mDiscoverySessions.put(mDiscoverySessionId, discoverySession);
            }

            Bundle mResults = new Bundle();
            mResults.putInt("discoverySessionId", mDiscoverySessionId);
            mResults.putLong("latencyMs", System.currentTimeMillis() - mCreateTimestampMs);
            mResults.putLong("timestampMs", System.currentTimeMillis());
            postEvent("WifiAwareSessionOnSubscribeStarted", mResults);
        }

        @Override
        public void onSessionConfigUpdated() {
            Bundle mResults = new Bundle();
            mResults.putInt("discoverySessionId", mDiscoverySessionId);
            postEvent("WifiAwareSessionOnSessionConfigUpdated", mResults);
        }

        @Override
        public void onSessionConfigFailed() {
            Bundle mResults = new Bundle();
            mResults.putInt("discoverySessionId", mDiscoverySessionId);
            postEvent("WifiAwareSessionOnSessionConfigFailed", mResults);
        }

        @Override
        public void onSessionTerminated() {
            Bundle mResults = new Bundle();
            mResults.putInt("discoverySessionId", mDiscoverySessionId);
            postEvent("WifiAwareSessionOnSessionTerminated", mResults);
        }

        private Bundle createServiceDiscoveredBaseBundle(PeerHandle peerHandle,
                byte[] serviceSpecificInfo, List<byte[]> matchFilter) {
            Bundle mResults = new Bundle();
            mResults.putInt("discoverySessionId", mDiscoverySessionId);
            mResults.putInt("peerId", peerHandle.peerId);
            mResults.putByteArray("serviceSpecificInfo", serviceSpecificInfo);
            mResults.putByteArray("matchFilter", new TlvBufferUtils.TlvConstructor(0,
                    1).allocateAndPut(matchFilter).getArray());
            ArrayList<String> matchFilterStrings = new ArrayList<>(matchFilter.size());
            for (byte[] be: matchFilter) {
                matchFilterStrings.add(Base64.encodeToString(be, Base64.DEFAULT));
            }
            mResults.putStringArrayList("matchFilterList", matchFilterStrings);
            mResults.putLong("timestampMs", System.currentTimeMillis());
            return mResults;
        }

        @Override
        public void onServiceDiscovered(PeerHandle peerHandle,
                byte[] serviceSpecificInfo, List<byte[]> matchFilter) {
            Bundle mResults = createServiceDiscoveredBaseBundle(peerHandle, serviceSpecificInfo,
                    matchFilter);
            postEvent("WifiAwareSessionOnServiceDiscovered", mResults);
        }

        @Override
        public void onServiceDiscoveredWithinRange(PeerHandle peerHandle,
                byte[] serviceSpecificInfo,
                List<byte[]> matchFilter, int distanceMm) {
            Bundle mResults = createServiceDiscoveredBaseBundle(peerHandle, serviceSpecificInfo,
                    matchFilter);
            mResults.putInt("distanceMm", distanceMm);
            postEvent("WifiAwareSessionOnServiceDiscovered", mResults);
        }

        @Override
        public void onMessageSendSucceeded(int messageId) {
            Bundle mResults = new Bundle();
            mResults.putInt("discoverySessionId", mDiscoverySessionId);
            mResults.putInt("messageId", messageId);
            synchronized (mLock) {
                Long startTime = mMessageStartTime.get(messageId);
                if (startTime != null) {
                    mResults.putLong("latencyMs",
                            System.currentTimeMillis() - startTime.longValue());
                    mMessageStartTime.remove(messageId);
                }
            }
            postEvent("WifiAwareSessionOnMessageSent", mResults);
        }

        @Override
        public void onMessageSendFailed(int messageId) {
            Bundle mResults = new Bundle();
            mResults.putInt("discoverySessionId", mDiscoverySessionId);
            mResults.putInt("messageId", messageId);
            synchronized (mLock) {
                Long startTime = mMessageStartTime.get(messageId);
                if (startTime != null) {
                    mResults.putLong("latencyMs",
                            System.currentTimeMillis() - startTime.longValue());
                    mMessageStartTime.remove(messageId);
                }
            }
            postEvent("WifiAwareSessionOnMessageSendFailed", mResults);
        }

        @Override
        public void onMessageReceived(PeerHandle peerHandle, byte[] message) {
            Bundle mResults = new Bundle();
            mResults.putInt("discoverySessionId", mDiscoverySessionId);
            mResults.putInt("peerId", peerHandle.peerId);
            mResults.putByteArray("message", message); // TODO: base64
            mResults.putString("messageAsString", new String(message));
            postEvent("WifiAwareSessionOnMessageReceived", mResults);
        }
    }

    class WifiAwareRangingListener implements RttManager.RttListener {
        private int mCallbackId;
        private int mSessionId;

        public WifiAwareRangingListener(int callbackId, int sessionId) {
            mCallbackId = callbackId;
            mSessionId = sessionId;
        }

        @Override
        public void onSuccess(RttResult[] results) {
            Bundle bundle = new Bundle();
            bundle.putInt("callbackId", mCallbackId);
            bundle.putInt("sessionId", mSessionId);

            Parcelable[] resultBundles = new Parcelable[results.length];
            for (int i = 0; i < results.length; i++) {
                resultBundles[i] = WifiRttManagerFacade.RangingListener.packRttResult(results[i]);
            }
            bundle.putParcelableArray("Results", resultBundles);

            mEventFacade.postEvent("WifiAwareRangingListenerOnSuccess", bundle);
        }

        @Override
        public void onFailure(int reason, String description) {
            Bundle bundle = new Bundle();
            bundle.putInt("callbackId", mCallbackId);
            bundle.putInt("sessionId", mSessionId);
            bundle.putInt("reason", reason);
            bundle.putString("description", description);
            mEventFacade.postEvent("WifiAwareRangingListenerOnFailure", bundle);
        }

        @Override
        public void onAborted() {
            Bundle bundle = new Bundle();
            bundle.putInt("callbackId", mCallbackId);
            bundle.putInt("sessionId", mSessionId);
            mEventFacade.postEvent("WifiAwareRangingListenerOnAborted", bundle);
        }

    }

    class WifiAwareStateChangedReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context c, Intent intent) {
            boolean isAvailable = mMgr.isAvailable();
            if (!isAvailable) {
                wifiAwareDestroyAll();
            }
            mEventFacade.postEvent(isAvailable ? "WifiAwareAvailable" : "WifiAwareNotAvailable",
                    new Bundle());
        }
    }
}
