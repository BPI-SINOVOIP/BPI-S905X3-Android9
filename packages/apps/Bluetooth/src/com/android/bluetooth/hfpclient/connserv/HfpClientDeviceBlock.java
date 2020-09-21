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
package com.android.bluetooth.hfpclient.connserv;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHeadsetClient;
import android.bluetooth.BluetoothHeadsetClientCall;
import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.telecom.Connection;
import android.telecom.DisconnectCause;
import android.telecom.PhoneAccount;
import android.telecom.TelecomManager;
import android.util.Log;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

// Helper class that manages the call handling for one device. HfpClientConnectionService holdes a
// list of such blocks and routes traffic from the UI.
//
// Lifecycle of a Device Block is managed entirely by the Service which creates it. In essence it
// has only the active state otherwise the block should be GCed.
public class HfpClientDeviceBlock {
    private final String mTAG;
    private static final boolean DBG = false;
    private final Context mContext;
    private final BluetoothDevice mDevice;
    private final PhoneAccount mPhoneAccount;
    private final Map<UUID, HfpClientConnection> mConnections = new HashMap<>();
    private final TelecomManager mTelecomManager;
    private final HfpClientConnectionService mConnServ;
    private HfpClientConference mConference;

    private BluetoothHeadsetClient mHeadsetProfile;

    HfpClientDeviceBlock(HfpClientConnectionService connServ, BluetoothDevice device,
            BluetoothHeadsetClient headsetProfile) {
        mConnServ = connServ;
        mContext = connServ;
        mDevice = device;
        mTAG = "HfpClientDeviceBlock." + mDevice.getAddress();
        mPhoneAccount = HfpClientConnectionService.createAccount(mContext, device);
        mTelecomManager = (TelecomManager) mContext.getSystemService(Context.TELECOM_SERVICE);

        // Register the phone account since block is created only when devices are connected
        mTelecomManager.registerPhoneAccount(mPhoneAccount);
        mTelecomManager.enablePhoneAccount(mPhoneAccount.getAccountHandle(), true);
        mTelecomManager.setUserSelectedOutgoingPhoneAccount(mPhoneAccount.getAccountHandle());
        mHeadsetProfile = headsetProfile;

        // Read the current calls and add them to telecom if already present
        if (mHeadsetProfile != null) {
            List<BluetoothHeadsetClientCall> calls = mHeadsetProfile.getCurrentCalls(mDevice);
            if (DBG) {
                Log.d(mTAG, "Got calls " + calls);
            }
            if (calls == null) {
                // We can get null as a return if we are not connected. Hence there may
                // be a race in getting the broadcast and HFP Client getting
                // disconnected before broadcast gets delivered.
                Log.w(mTAG, "Got connected but calls were null, ignoring the broadcast");
                return;
            }

            for (BluetoothHeadsetClientCall call : calls) {
                handleCall(call);
            }
        } else {
            Log.e(mTAG, "headset profile is null, ignoring broadcast.");
        }
    }

    synchronized Connection onCreateIncomingConnection(BluetoothHeadsetClientCall call) {
        HfpClientConnection connection = mConnections.get(call.getUUID());
        if (connection != null) {
            connection.onAdded();
            return connection;
        } else {
            Log.e(mTAG, "Call " + call + " ignored: connection does not exist");
            return null;
        }
    }

    Connection onCreateOutgoingConnection(Uri address) {
        HfpClientConnection connection = buildConnection(null, address);
        if (connection != null) {
            connection.onAdded();
        }
        return connection;
    }

    synchronized Connection onCreateUnknownConnection(BluetoothHeadsetClientCall call) {
        Uri number = Uri.fromParts(PhoneAccount.SCHEME_TEL, call.getNumber(), null);
        HfpClientConnection connection = mConnections.get(call.getUUID());

        if (connection != null) {
            connection.onAdded();
            return connection;
        } else {
            Log.e(mTAG, "Call " + call + " ignored: connection does not exist");
            return null;
        }
    }

    synchronized void onConference(Connection connection1, Connection connection2) {
        if (mConference == null) {
            mConference = new HfpClientConference(mPhoneAccount.getAccountHandle(), mDevice,
                    mHeadsetProfile);
        }

        if (connection1.getConference() == null) {
            mConference.addConnection(connection1);
        }

        if (connection2.getConference() == null) {
            mConference.addConnection(connection2);
        }
    }

    // Remove existing calls and the phone account associated, the object will get garbage
    // collected soon
    synchronized void cleanup() {
        Log.d(mTAG, "Resetting state for device " + mDevice);
        disconnectAll();
        mTelecomManager.unregisterPhoneAccount(mPhoneAccount.getAccountHandle());
    }

    // Handle call change
    synchronized void handleCall(BluetoothHeadsetClientCall call) {
        if (DBG) {
            Log.d(mTAG, "Got call " + call.toString(true));
        }

        HfpClientConnection connection = findConnectionKey(call);

        // We need to have special handling for calls that mysteriously convert from
        // DISCONNECTING -> ACTIVE/INCOMING state. This can happen for PTS (b/31159015).
        // We terminate the previous call and create a new one here.
        if (connection != null && isDisconnectingToActive(connection, call)) {
            connection.close(DisconnectCause.ERROR);
            mConnections.remove(call.getUUID());
            connection = null;
        }

        if (connection != null) {
            connection.updateCall(call);
            connection.handleCallChanged();
        }

        if (connection == null) {
            // Create the connection here, trigger Telecom to bind to us.
            buildConnection(call, null);

            // Depending on where this call originated make it an incoming call or outgoing
            // (represented as unknown call in telecom since). Since BluetoothHeadsetClientCall is a
            // parcelable we simply pack the entire object in there.
            Bundle b = new Bundle();
            if (call.getState() == BluetoothHeadsetClientCall.CALL_STATE_DIALING
                    || call.getState() == BluetoothHeadsetClientCall.CALL_STATE_ALERTING
                    || call.getState() == BluetoothHeadsetClientCall.CALL_STATE_ACTIVE
                    || call.getState() == BluetoothHeadsetClientCall.CALL_STATE_HELD) {
                // This is an outgoing call. Even if it is an active call we do not have a way of
                // putting that parcelable in a seaprate field.
                b.putParcelable(TelecomManager.EXTRA_OUTGOING_CALL_EXTRAS, call);
                mTelecomManager.addNewUnknownCall(mPhoneAccount.getAccountHandle(), b);
            } else if (call.getState() == BluetoothHeadsetClientCall.CALL_STATE_INCOMING
                    || call.getState() == BluetoothHeadsetClientCall.CALL_STATE_WAITING) {
                // This is an incoming call.
                b.putParcelable(TelecomManager.EXTRA_INCOMING_CALL_EXTRAS, call);
                b.putBoolean(TelecomManager.EXTRA_CALL_EXTERNAL_RINGER, call.isInBandRing());
                mTelecomManager.addNewIncomingCall(mPhoneAccount.getAccountHandle(), b);
            }
        } else if (call.getState() == BluetoothHeadsetClientCall.CALL_STATE_TERMINATED) {
            if (DBG) {
                Log.d(mTAG, "Removing call " + call);
            }
            mConnections.remove(call.getUUID());
        }

        updateConferenceableConnections();
    }

    // Find the connection specified by the key, also update the key with ID if present.
    private synchronized HfpClientConnection findConnectionKey(BluetoothHeadsetClientCall call) {
        if (DBG) {
            Log.d(mTAG, "findConnectionKey local key set " + mConnections.toString());
        }
        return mConnections.get(call.getUUID());
    }

    // Disconnect all calls
    private void disconnectAll() {
        for (HfpClientConnection connection : mConnections.values()) {
            connection.onHfpDisconnected();
        }

        mConnections.clear();

        if (mConference != null) {
            mConference.destroy();
            mConference = null;
        }
    }

    private boolean isDisconnectingToActive(HfpClientConnection prevConn,
            BluetoothHeadsetClientCall newCall) {
        if (DBG) {
            Log.d(mTAG, "prevConn " + prevConn.isClosing() + " new call " + newCall.getState());
        }
        if (prevConn.isClosing()
                && newCall.getState() != BluetoothHeadsetClientCall.CALL_STATE_TERMINATED) {
            return true;
        }
        return false;
    }

    private synchronized HfpClientConnection buildConnection(BluetoothHeadsetClientCall call,
            Uri number) {
        if (mHeadsetProfile == null) {
            Log.e(mTAG,
                    "Cannot create connection for call " + call + " when Profile not available");
            return null;
        }

        if (call == null && number == null) {
            Log.e(mTAG, "Both call and number cannot be null.");
            return null;
        }

        if (DBG) {
            Log.d(mTAG, "Creating connection on " + mDevice + " for " + call + "/" + number);
        }

        HfpClientConnection connection = null;
        if (call != null) {
            connection = new HfpClientConnection(mConnServ, mDevice, mHeadsetProfile, call);
        } else {
            connection = new HfpClientConnection(mConnServ, mDevice, mHeadsetProfile, number);
        }

        if (connection.getState() != Connection.STATE_DISCONNECTED) {
            mConnections.put(connection.getUUID(), connection);
        }

        return connection;
    }

    // Updates any conferencable connections.
    private void updateConferenceableConnections() {
        boolean addConf = false;
        if (DBG) {
            Log.d(mTAG, "Existing connections: " + mConnections + " existing conference "
                    + mConference);
        }

        // If we have an existing conference call then loop through all connections and update any
        // connections that may have switched from conference -> non-conference.
        if (mConference != null) {
            for (Connection confConn : mConference.getConnections()) {
                if (!((HfpClientConnection) confConn).inConference()) {
                    if (DBG) {
                        Log.d(mTAG, "Removing connection " + confConn + " from conference.");
                    }
                    mConference.removeConnection(confConn);
                }
            }
        }

        // If we have connections that are not already part of the conference then add them.
        // NOTE: addConnection takes care of duplicates (by mem addr) and the lifecycle of a
        // connection is maintained by the UUID.
        for (Connection otherConn : mConnections.values()) {
            if (((HfpClientConnection) otherConn).inConference()) {
                // If this is the first connection with conference, create the conference first.
                if (mConference == null) {
                    mConference = new HfpClientConference(mPhoneAccount.getAccountHandle(), mDevice,
                            mHeadsetProfile);
                }
                if (mConference.addConnection(otherConn)) {
                    if (DBG) {
                        Log.d(mTAG, "Adding connection " + otherConn + " to conference.");
                    }
                    addConf = true;
                }
            }
        }

        // If we have no connections in the conference we should simply end it.
        if (mConference != null && mConference.getConnections().size() == 0) {
            if (DBG) {
                Log.d(mTAG, "Conference has no connection, destroying");
            }
            mConference.setDisconnected(new DisconnectCause(DisconnectCause.LOCAL));
            mConference.destroy();
            mConference = null;
        }

        // If we have a valid conference and not previously added then add it.
        if (mConference != null && addConf) {
            if (DBG) {
                Log.d(mTAG, "Adding conference to stack.");
            }
            mConnServ.addConference(mConference);
        }
    }

}
