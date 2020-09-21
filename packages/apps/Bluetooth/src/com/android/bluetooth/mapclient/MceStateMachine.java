/*
 * Copyright (C) 2016 The Android Open Source Project
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

/**
 * Bluetooth MAP MCE StateMachine
 *         (Disconnected)
 *             |    ^
 *     CONNECT |    | DISCONNECTED
 *             V    |
 *    (Connecting) (Disconnecting)
 *             |    ^
 *   CONNECTED |    | DISCONNECT
 *             V    |
 *           (Connected)
 *
 * Valid Transitions: State + Event -> Transition:
 *
 * Disconnected + CONNECT -> Connecting
 * Connecting + CONNECTED -> Connected
 * Connecting + TIMEOUT -> Disconnecting
 * Connecting + DISCONNECT/CONNECT -> Defer Message
 * Connected + DISCONNECT -> Disconnecting
 * Connected + CONNECT -> Disconnecting + Defer Message
 * Disconnecting + DISCONNECTED -> (Safe) Disconnected
 * Disconnecting + TIMEOUT -> (Force) Disconnected
 * Disconnecting + DISCONNECT/CONNECT : Defer Message
 */
package com.android.bluetooth.mapclient;

import android.app.Activity;
import android.app.PendingIntent;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothMapClient;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothUuid;
import android.bluetooth.SdpMasRecord;
import android.content.Intent;
import android.net.Uri;
import android.os.Message;
import android.telecom.PhoneAccount;
import android.telephony.SmsManager;
import android.util.Log;

import com.android.bluetooth.BluetoothMetricsProto;
import com.android.bluetooth.btservice.MetricsLogger;
import com.android.bluetooth.btservice.ProfileService;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.util.IState;
import com.android.internal.util.State;
import com.android.internal.util.StateMachine;
import com.android.vcard.VCardConstants;
import com.android.vcard.VCardEntry;
import com.android.vcard.VCardProperty;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.HashMap;
import java.util.List;

/* The MceStateMachine is responsible for setting up and maintaining a connection to a single
 * specific Messaging Server Equipment endpoint.  Upon connect command an SDP record is retrieved,
 * a connection to the Message Access Server is created and a request to enable notification of new
 * messages is sent.
 */
final class MceStateMachine extends StateMachine {
    // Messages for events handled by the StateMachine
    static final int MSG_MAS_CONNECTED = 1001;
    static final int MSG_MAS_DISCONNECTED = 1002;
    static final int MSG_MAS_REQUEST_COMPLETED = 1003;
    static final int MSG_MAS_REQUEST_FAILED = 1004;
    static final int MSG_MAS_SDP_DONE = 1005;
    static final int MSG_MAS_SDP_FAILED = 1006;
    static final int MSG_OUTBOUND_MESSAGE = 2001;
    static final int MSG_INBOUND_MESSAGE = 2002;
    static final int MSG_NOTIFICATION = 2003;
    static final int MSG_GET_LISTING = 2004;
    static final int MSG_GET_MESSAGE_LISTING = 2005;

    private static final String TAG = "MceSM";
    private static final Boolean DBG = MapClientService.DBG;
    private static final int TIMEOUT = 10000;
    private static final int MAX_MESSAGES = 20;
    private static final int MSG_CONNECT = 1;
    private static final int MSG_DISCONNECT = 2;
    private static final int MSG_CONNECTING_TIMEOUT = 3;
    private static final int MSG_DISCONNECTING_TIMEOUT = 4;
    // Folder names as defined in Bluetooth.org MAP spec V10
    private static final String FOLDER_TELECOM = "telecom";
    private static final String FOLDER_MSG = "msg";
    private static final String FOLDER_OUTBOX = "outbox";
    private static final String FOLDER_INBOX = "inbox";
    private static final String INBOX_PATH = "telecom/msg/inbox";


    // Connectivity States
    private int mPreviousState = BluetoothProfile.STATE_DISCONNECTED;
    private State mDisconnected;
    private State mConnecting;
    private State mConnected;
    private State mDisconnecting;

    private final BluetoothDevice mDevice;
    private MapClientService mService;
    private MasClient mMasClient;
    private HashMap<String, Bmessage> mSentMessageLog = new HashMap<>(MAX_MESSAGES);
    private HashMap<Bmessage, PendingIntent> mSentReceiptRequested = new HashMap<>(MAX_MESSAGES);
    private HashMap<Bmessage, PendingIntent> mDeliveryReceiptRequested =
            new HashMap<>(MAX_MESSAGES);
    private Bmessage.Type mDefaultMessageType = Bmessage.Type.SMS_CDMA;

    MceStateMachine(MapClientService service, BluetoothDevice device) {
        this(service, device, null);
    }

    @VisibleForTesting
    MceStateMachine(MapClientService service, BluetoothDevice device, MasClient masClient) {
        super(TAG);
        mMasClient = masClient;
        mService = service;

        mPreviousState = BluetoothProfile.STATE_DISCONNECTED;

        mDevice = device;
        mDisconnected = new Disconnected();
        mConnecting = new Connecting();
        mDisconnecting = new Disconnecting();
        mConnected = new Connected();

        addState(mDisconnected);
        addState(mConnecting);
        addState(mDisconnecting);
        addState(mConnected);
        setInitialState(mConnecting);
        start();
    }

    public void doQuit() {
        quitNow();
    }

    @Override
    protected void onQuitting() {
        if (mService != null) {
            mService.cleanupDevice(mDevice);
        }
    }

    synchronized BluetoothDevice getDevice() {
        return mDevice;
    }

    private void onConnectionStateChanged(int prevState, int state) {
        // mDevice == null only at setInitialState
        if (mDevice == null) {
            return;
        }
        if (DBG) {
            Log.d(TAG, "Connection state " + mDevice + ": " + prevState + "->" + state);
        }
        if (prevState != state && state == BluetoothProfile.STATE_CONNECTED) {
            MetricsLogger.logProfileConnectionEvent(BluetoothMetricsProto.ProfileId.MAP_CLIENT);
        }
        Intent intent = new Intent(BluetoothMapClient.ACTION_CONNECTION_STATE_CHANGED);
        intent.putExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE, prevState);
        intent.putExtra(BluetoothProfile.EXTRA_STATE, state);
        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, mDevice);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        mService.sendBroadcast(intent, ProfileService.BLUETOOTH_PERM);
    }

    public synchronized int getState() {
        IState currentState = this.getCurrentState();
        if (currentState.getClass() == Disconnected.class) {
            return BluetoothProfile.STATE_DISCONNECTED;
        }
        if (currentState.getClass() == Connected.class) {
            return BluetoothProfile.STATE_CONNECTED;
        }
        if (currentState.getClass() == Connecting.class) {
            return BluetoothProfile.STATE_CONNECTING;
        }
        if (currentState.getClass() == Disconnecting.class) {
            return BluetoothProfile.STATE_DISCONNECTING;
        }
        return BluetoothProfile.STATE_DISCONNECTED;
    }

    public boolean disconnect() {
        if (DBG) {
            Log.d(TAG, "Disconnect Request " + mDevice.getAddress());
        }
        sendMessage(MSG_DISCONNECT, mDevice);
        return true;
    }

    public synchronized boolean sendMapMessage(Uri[] contacts, String message,
            PendingIntent sentIntent, PendingIntent deliveredIntent) {
        if (DBG) {
            Log.d(TAG, "Send Message " + message);
        }
        if (contacts == null || contacts.length <= 0) {
            return false;
        }
        if (this.getCurrentState() == mConnected) {
            Bmessage bmsg = new Bmessage();
            // Set type and status.
            bmsg.setType(getDefaultMessageType());
            bmsg.setStatus(Bmessage.Status.READ);

            for (Uri contact : contacts) {
                // Who to send the message to.
                VCardEntry destEntry = new VCardEntry();
                VCardProperty destEntryPhone = new VCardProperty();
                if (DBG) {
                    Log.d(TAG, "Scheme " + contact.getScheme());
                }
                if (PhoneAccount.SCHEME_TEL.equals(contact.getScheme())) {
                    destEntryPhone.setName(VCardConstants.PROPERTY_TEL);
                    destEntryPhone.addValues(contact.getSchemeSpecificPart());
                    if (DBG) {
                        Log.d(TAG, "Sending to phone numbers " + destEntryPhone.getValueList());
                    }
                } else {
                    if (DBG) {
                        Log.w(TAG, "Scheme " + contact.getScheme() + " not supported.");
                    }
                    return false;
                }
                destEntry.addProperty(destEntryPhone);
                bmsg.addRecipient(destEntry);
            }

            // Message of the body.
            bmsg.setBodyContent(message);
            if (sentIntent != null) {
                mSentReceiptRequested.put(bmsg, sentIntent);
            }
            if (deliveredIntent != null) {
                mDeliveryReceiptRequested.put(bmsg, deliveredIntent);
            }
            sendMessage(MSG_OUTBOUND_MESSAGE, bmsg);
            return true;
        }
        return false;
    }

    synchronized boolean getMessage(String handle) {
        if (DBG) {
            Log.d(TAG, "getMessage" + handle);
        }
        if (this.getCurrentState() == mConnected) {
            sendMessage(MSG_INBOUND_MESSAGE, handle);
            return true;
        }
        return false;
    }

    synchronized boolean getUnreadMessages() {
        if (DBG) {
            Log.d(TAG, "getMessage");
        }
        if (this.getCurrentState() == mConnected) {
            sendMessage(MSG_GET_MESSAGE_LISTING, FOLDER_INBOX);
            return true;
        }
        return false;
    }

    private String getContactURIFromPhone(String number) {
        return PhoneAccount.SCHEME_TEL + ":" + number;
    }

    Bmessage.Type getDefaultMessageType() {
        synchronized (mDefaultMessageType) {
            return mDefaultMessageType;
        }
    }

    void setDefaultMessageType(SdpMasRecord sdpMasRecord) {
        int supportedMessageTypes = sdpMasRecord.getSupportedMessageTypes();
        synchronized (mDefaultMessageType) {
            if ((supportedMessageTypes & SdpMasRecord.MessageType.SMS_CDMA) > 0) {
                mDefaultMessageType = Bmessage.Type.SMS_CDMA;
            } else if ((supportedMessageTypes & SdpMasRecord.MessageType.SMS_GSM) > 0) {
                mDefaultMessageType = Bmessage.Type.SMS_GSM;
            }
        }
    }

    public void dump(StringBuilder sb) {
        ProfileService.println(sb, "mCurrentDevice: " + mDevice.getAddress() + " (name = "
                + mDevice.getName() + "), StateMachine: " + this.toString());
    }

    class Disconnected extends State {
        @Override
        public void enter() {
            if (DBG) {
                Log.d(TAG, "Enter Disconnected: " + getCurrentMessage().what);
            }
            onConnectionStateChanged(mPreviousState, BluetoothProfile.STATE_DISCONNECTED);
            mPreviousState = BluetoothProfile.STATE_DISCONNECTED;
            quit();
        }

        @Override
        public void exit() {
            mPreviousState = BluetoothProfile.STATE_DISCONNECTED;
        }
    }

    class Connecting extends State {
        @Override
        public void enter() {
            if (DBG) {
                Log.d(TAG, "Enter Connecting: " + getCurrentMessage().what);
            }
            onConnectionStateChanged(mPreviousState, BluetoothProfile.STATE_CONNECTING);

            BluetoothAdapter.getDefaultAdapter().cancelDiscovery();
            // When commanded to connect begin SDP to find the MAS server.
            mDevice.sdpSearch(BluetoothUuid.MAS);
            sendMessageDelayed(MSG_CONNECTING_TIMEOUT, TIMEOUT);
        }

        @Override
        public boolean processMessage(Message message) {
            if (DBG) {
                Log.d(TAG, "processMessage" + this.getName() + message.what);
            }

            switch (message.what) {
                case MSG_MAS_SDP_DONE:
                    if (DBG) {
                        Log.d(TAG, "SDP Complete");
                    }
                    if (mMasClient == null) {
                        mMasClient = new MasClient(mDevice, MceStateMachine.this,
                                (SdpMasRecord) message.obj);
                        setDefaultMessageType((SdpMasRecord) message.obj);
                    }
                    break;

                case MSG_MAS_CONNECTED:
                    transitionTo(mConnected);
                    break;

                case MSG_MAS_DISCONNECTED:
                    transitionTo(mDisconnected);
                    break;

                case MSG_CONNECTING_TIMEOUT:
                    transitionTo(mDisconnecting);
                    break;

                case MSG_CONNECT:
                case MSG_DISCONNECT:
                    deferMessage(message);
                    break;

                default:
                    Log.w(TAG, "Unexpected message: " + message.what + " from state:"
                            + this.getName());
                    return NOT_HANDLED;
            }
            return HANDLED;
        }

        @Override
        public void exit() {
            mPreviousState = BluetoothProfile.STATE_CONNECTING;
            removeMessages(MSG_CONNECTING_TIMEOUT);
        }
    }

    class Connected extends State {
        @Override
        public void enter() {
            if (DBG) {
                Log.d(TAG, "Enter Connected: " + getCurrentMessage().what);
            }
            onConnectionStateChanged(mPreviousState, BluetoothProfile.STATE_CONNECTED);

            mMasClient.makeRequest(new RequestSetPath(FOLDER_TELECOM));
            mMasClient.makeRequest(new RequestSetPath(FOLDER_MSG));
            mMasClient.makeRequest(new RequestSetPath(FOLDER_INBOX));
            mMasClient.makeRequest(new RequestGetFolderListing(0, 0));
            mMasClient.makeRequest(new RequestSetPath(false));
            mMasClient.makeRequest(new RequestSetNotificationRegistration(true));
        }

        @Override
        public boolean processMessage(Message message) {
            switch (message.what) {
                case MSG_DISCONNECT:
                    if (mDevice.equals(message.obj)) {
                        transitionTo(mDisconnecting);
                    }
                    break;

                case MSG_OUTBOUND_MESSAGE:
                    mMasClient.makeRequest(
                            new RequestPushMessage(FOLDER_OUTBOX, (Bmessage) message.obj, null,
                                    false, false));
                    break;

                case MSG_INBOUND_MESSAGE:
                    mMasClient.makeRequest(
                            new RequestGetMessage((String) message.obj, MasClient.CharsetType.UTF_8,
                                    false));
                    break;

                case MSG_NOTIFICATION:
                    processNotification(message);
                    break;

                case MSG_GET_LISTING:
                    mMasClient.makeRequest(new RequestGetFolderListing(0, 0));
                    break;

                case MSG_GET_MESSAGE_LISTING:
                    // Get latest 50 Unread messages in the last week
                    MessagesFilter filter = new MessagesFilter();
                    filter.setMessageType((byte) 0);
                    filter.setReadStatus(MessagesFilter.READ_STATUS_UNREAD);
                    Calendar calendar = Calendar.getInstance();
                    calendar.add(Calendar.DATE, -7);
                    filter.setPeriod(calendar.getTime(), null);
                    mMasClient.makeRequest(new RequestGetMessagesListing(
                            (String) message.obj, 0, filter, 0, 50, 0));
                    break;

                case MSG_MAS_REQUEST_COMPLETED:
                    if (DBG) {
                        Log.d(TAG, "Completed request");
                    }
                    if (message.obj instanceof RequestGetMessage) {
                        processInboundMessage((RequestGetMessage) message.obj);
                    } else if (message.obj instanceof RequestPushMessage) {
                        String messageHandle = ((RequestPushMessage) message.obj).getMsgHandle();
                        if (DBG) {
                            Log.d(TAG, "Message Sent......." + messageHandle);
                        }
                        // ignore the top-order byte (converted to string) in the handle for now
                        mSentMessageLog.put(messageHandle.substring(2),
                                ((RequestPushMessage) message.obj).getBMsg());
                    } else if (message.obj instanceof RequestGetMessagesListing) {
                        processMessageListing((RequestGetMessagesListing) message.obj);
                    }
                    break;

                case MSG_CONNECT:
                    if (!mDevice.equals(message.obj)) {
                        deferMessage(message);
                        transitionTo(mDisconnecting);
                    }
                    break;

                default:
                    Log.w(TAG, "Unexpected message: " + message.what + " from state:"
                            + this.getName());
                    return NOT_HANDLED;
            }
            return HANDLED;
        }

        @Override
        public void exit() {
            mPreviousState = BluetoothProfile.STATE_CONNECTED;
        }

        private void processNotification(Message msg) {
            if (DBG) {
                Log.d(TAG, "Handler: msg: " + msg.what);
            }

            switch (msg.what) {
                case MSG_NOTIFICATION:
                    EventReport ev = (EventReport) msg.obj;
                    if (DBG) {
                        Log.d(TAG, "Message Type = " + ev.getType());
                    }
                    if (DBG) {
                        Log.d(TAG, "Message handle = " + ev.getHandle());
                    }
                    switch (ev.getType()) {

                        case NEW_MESSAGE:
                            //mService.get().sendNewMessageNotification(ev);
                            mMasClient.makeRequest(new RequestGetMessage(ev.getHandle(),
                                    MasClient.CharsetType.UTF_8, false));
                            break;

                        case DELIVERY_SUCCESS:
                        case SENDING_SUCCESS:
                            notifySentMessageStatus(ev.getHandle(), ev.getType());
                            break;
                    }
            }
        }

        // Sets the specified message status to "read" (from "unread" status, mostly)
        private void markMessageRead(RequestGetMessage request) {
            if (DBG) Log.d(TAG, "markMessageRead");
            mMasClient.makeRequest(new RequestSetMessageStatus(
                    request.getHandle(), RequestSetMessageStatus.StatusIndicator.READ));
        }

        // Sets the specified message status to "deleted"
        private void markMessageDeleted(RequestGetMessage request) {
            if (DBG) Log.d(TAG, "markMessageDeleted");
            mMasClient.makeRequest(new RequestSetMessageStatus(
                    request.getHandle(), RequestSetMessageStatus.StatusIndicator.DELETED));
        }

        private void processMessageListing(RequestGetMessagesListing request) {
            if (DBG) {
                Log.d(TAG, "processMessageListing");
            }
            ArrayList<com.android.bluetooth.mapclient.Message> messageHandles = request.getList();
            if (messageHandles != null) {
                for (com.android.bluetooth.mapclient.Message handle : messageHandles) {
                    if (DBG) {
                        Log.d(TAG, "getting message ");
                    }
                    getMessage(handle.getHandle());
                }
            }
        }

        private void processInboundMessage(RequestGetMessage request) {
            Bmessage message = request.getMessage();
            if (DBG) {
                Log.d(TAG, "Notify inbound Message" + message);
            }

            if (message == null) {
                return;
            }
            if (!INBOX_PATH.equalsIgnoreCase(message.getFolder())) {
                if (DBG) {
                    Log.d(TAG, "Ignoring message received in " + message.getFolder() + ".");
                }
                return;
            }
            switch (message.getType()) {
                case SMS_CDMA:
                case SMS_GSM:
                    if (DBG) {
                        Log.d(TAG, "Body: " + message.getBodyContent());
                    }
                    if (DBG) {
                        Log.d(TAG, message.toString());
                    }
                    if (DBG) {
                        Log.d(TAG, "Recipients" + message.getRecipients().toString());
                    }

                    Intent intent = new Intent();
                    intent.setAction(BluetoothMapClient.ACTION_MESSAGE_RECEIVED);
                    intent.putExtra(BluetoothDevice.EXTRA_DEVICE, mDevice);
                    intent.putExtra(BluetoothMapClient.EXTRA_MESSAGE_HANDLE, request.getHandle());
                    intent.putExtra(android.content.Intent.EXTRA_TEXT, message.getBodyContent());
                    VCardEntry originator = message.getOriginator();
                    if (originator != null) {
                        if (DBG) {
                            Log.d(TAG, originator.toString());
                        }
                        List<VCardEntry.PhoneData> phoneData = originator.getPhoneList();
                        if (phoneData != null && phoneData.size() > 0) {
                            String phoneNumber = phoneData.get(0).getNumber();
                            if (DBG) {
                                Log.d(TAG, "Originator number: " + phoneNumber);
                            }
                            intent.putExtra(BluetoothMapClient.EXTRA_SENDER_CONTACT_URI,
                                    getContactURIFromPhone(phoneNumber));
                        }
                        intent.putExtra(BluetoothMapClient.EXTRA_SENDER_CONTACT_NAME,
                                originator.getDisplayName());
                    }
                    mService.sendBroadcast(intent);
                    break;

                case MMS:
                case EMAIL:
                default:
                    Log.e(TAG, "Received unhandled type" + message.getType().toString());
                    break;
            }
        }

        private void notifySentMessageStatus(String handle, EventReport.Type status) {
            if (DBG) {
                Log.d(TAG, "got a status for " + handle + " Status = " + status);
            }
            PendingIntent intentToSend = null;
            // ignore the top-order byte (converted to string) in the handle for now
            String shortHandle = handle.substring(2);
            if (status == EventReport.Type.SENDING_FAILURE
                    || status == EventReport.Type.SENDING_SUCCESS) {
                intentToSend = mSentReceiptRequested.remove(mSentMessageLog.get(shortHandle));
            } else if (status == EventReport.Type.DELIVERY_SUCCESS
                    || status == EventReport.Type.DELIVERY_FAILURE) {
                intentToSend = mDeliveryReceiptRequested.remove(mSentMessageLog.get(shortHandle));
            }

            if (intentToSend != null) {
                try {
                    if (DBG) {
                        Log.d(TAG, "*******Sending " + intentToSend);
                    }
                    int result = Activity.RESULT_OK;
                    if (status == EventReport.Type.SENDING_FAILURE
                            || status == EventReport.Type.DELIVERY_FAILURE) {
                        result = SmsManager.RESULT_ERROR_GENERIC_FAILURE;
                    }
                    intentToSend.send(result);
                } catch (PendingIntent.CanceledException e) {
                    Log.w(TAG, "Notification Request Canceled" + e);
                }
            } else {
                Log.e(TAG, "Received a notification on message with handle = "
                        + handle + ", but it is NOT found in mSentMessageLog! where did it go?");
            }
        }
    }

    class Disconnecting extends State {
        @Override
        public void enter() {
            if (DBG) {
                Log.d(TAG, "Enter Disconnecting: " + getCurrentMessage().what);
            }
            onConnectionStateChanged(mPreviousState, BluetoothProfile.STATE_DISCONNECTING);

            if (mMasClient != null) {
                mMasClient.makeRequest(new RequestSetNotificationRegistration(false));
                mMasClient.shutdown();
                sendMessageDelayed(MSG_DISCONNECTING_TIMEOUT, TIMEOUT);
            } else {
                // MAP was never connected
                transitionTo(mDisconnected);
            }
        }

        @Override
        public boolean processMessage(Message message) {
            switch (message.what) {
                case MSG_DISCONNECTING_TIMEOUT:
                case MSG_MAS_DISCONNECTED:
                    mMasClient = null;
                    transitionTo(mDisconnected);
                    break;

                case MSG_CONNECT:
                case MSG_DISCONNECT:
                    deferMessage(message);
                    break;

                default:
                    Log.w(TAG, "Unexpected message: " + message.what + " from state:"
                            + this.getName());
                    return NOT_HANDLED;
            }
            return HANDLED;
        }

        @Override
        public void exit() {
            mPreviousState = BluetoothProfile.STATE_DISCONNECTING;
            removeMessages(MSG_DISCONNECTING_TIMEOUT);
        }
    }

    void receiveEvent(EventReport ev) {
        if (DBG) {
            Log.d(TAG, "Message Type = " + ev.getType());
        }
        if (DBG) {
            Log.d(TAG, "Message handle = " + ev.getHandle());
        }
        sendMessage(MSG_NOTIFICATION, ev);
    }
}
