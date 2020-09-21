/*
 * Copyright (c) 2008-2009, Motorola, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * - Neither the name of the Motorola, Inc. nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

package com.android.bluetooth.opp;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothDevicePicker;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.CharArrayBuffer;
import android.database.ContentObserver;
import android.database.Cursor;
import android.media.MediaScannerConnection;
import android.media.MediaScannerConnection.MediaScannerConnectionClient;
import android.net.Uri;
import android.os.Binder;
import android.os.Handler;
import android.os.Message;
import android.os.Process;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

import com.android.bluetooth.BluetoothObexTransport;
import com.android.bluetooth.IObexConnectionHandler;
import com.android.bluetooth.ObexServerSockets;
import com.android.bluetooth.btservice.ProfileService;
import com.android.bluetooth.sdp.SdpManager;

import com.google.android.collect.Lists;

import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;

import javax.obex.ObexTransport;

/**
 * Performs the background Bluetooth OPP transfer. It also starts thread to
 * accept incoming OPP connection.
 */

public class BluetoothOppService extends ProfileService implements IObexConnectionHandler {
    private static final boolean D = Constants.DEBUG;
    private static final boolean V = Constants.VERBOSE;

    private static final byte[] SUPPORTED_OPP_FORMAT = {
            0x01 /* vCard 2.1 */,
            0x02 /* vCard 3.0 */,
            0x03 /* vCal 1.0 */,
            0x04 /* iCal 2.0 */,
            (byte) 0xFF /* Any type of object */
    };

    private class BluetoothShareContentObserver extends ContentObserver {

        BluetoothShareContentObserver() {
            super(new Handler());
        }

        @Override
        public void onChange(boolean selfChange) {
            if (V) {
                Log.v(TAG, "ContentObserver received notification");
            }
            updateFromProvider();
        }
    }

    private static final String TAG = "BtOppService";

    /** Observer to get notified when the content observer's data changes */
    private BluetoothShareContentObserver mObserver;

    /** Class to handle Notification Manager updates */
    private BluetoothOppNotification mNotifier;

    private boolean mPendingUpdate;

    private UpdateThread mUpdateThread;

    private ArrayList<BluetoothOppShareInfo> mShares;

    private ArrayList<BluetoothOppBatch> mBatches;

    private BluetoothOppTransfer mTransfer;

    private BluetoothOppTransfer mServerTransfer;

    private int mBatchId;

    /**
     * Array used when extracting strings from content provider
     */
    private CharArrayBuffer mOldChars;
    /**
     * Array used when extracting strings from content provider
     */
    private CharArrayBuffer mNewChars;

    private boolean mListenStarted;

    private boolean mMediaScanInProgress;

    private int mIncomingRetries;

    private ObexTransport mPendingConnection;

    private int mOppSdpHandle = -1;

    boolean mAcceptNewConnections;

    private BluetoothAdapter mAdapter;

    private static final String INVISIBLE =
            BluetoothShare.VISIBILITY + "=" + BluetoothShare.VISIBILITY_HIDDEN;

    private static final String WHERE_INBOUND_SUCCESS =
            BluetoothShare.DIRECTION + "=" + BluetoothShare.DIRECTION_INBOUND + " AND "
                    + BluetoothShare.STATUS + "=" + BluetoothShare.STATUS_SUCCESS + " AND "
                    + INVISIBLE;

    private static final String WHERE_CONFIRM_PENDING_INBOUND =
            BluetoothShare.DIRECTION + "=" + BluetoothShare.DIRECTION_INBOUND + " AND "
                    + BluetoothShare.USER_CONFIRMATION + "="
                    + BluetoothShare.USER_CONFIRMATION_PENDING;

    private static final String WHERE_INVISIBLE_UNCONFIRMED =
            "(" + BluetoothShare.STATUS + ">=" + BluetoothShare.STATUS_SUCCESS + " AND " + INVISIBLE
                    + ") OR (" + WHERE_CONFIRM_PENDING_INBOUND + ")";

    private static BluetoothOppService sBluetoothOppService;

    /*
     * TODO No support for queue incoming from multiple devices.
     * Make an array list of server session to support receiving queue from
     * multiple devices
     */
    private BluetoothOppObexServerSession mServerSession;

    @Override
    protected IProfileServiceBinder initBinder() {
        return new OppBinder(this);
    }

    private static class OppBinder extends Binder implements IProfileServiceBinder {

        OppBinder(BluetoothOppService service) {
        }

        @Override
        public void cleanup() {
        }
    }

    @Override
    protected void create() {
        if (V) {
            Log.v(TAG, "onCreate");
        }
        mShares = Lists.newArrayList();
        mBatches = Lists.newArrayList();
        mBatchId = 1;
        final ContentResolver contentResolver = getContentResolver();
        new Thread("trimDatabase") {
            @Override
            public void run() {
                trimDatabase(contentResolver);
            }
        }.start();

        IntentFilter filter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
        registerReceiver(mBluetoothReceiver, filter);

        mAdapter = BluetoothAdapter.getDefaultAdapter();
        synchronized (BluetoothOppService.this) {
            if (mAdapter == null) {
                Log.w(TAG, "Local BT device is not enabled");
            }
        }
        if (V) {
            BluetoothOppPreference preference = BluetoothOppPreference.getInstance(this);
            if (preference != null) {
                preference.dump();
            } else {
                Log.w(TAG, "BluetoothOppPreference.getInstance returned null.");
            }
        }
    }

    @Override
    public boolean start() {
        if (V) {
            Log.v(TAG, "start()");
        }
        mObserver = new BluetoothShareContentObserver();
        getContentResolver().registerContentObserver(BluetoothShare.CONTENT_URI, true, mObserver);
        mNotifier = new BluetoothOppNotification(this);
        mNotifier.mNotificationMgr.cancelAll();
        mNotifier.updateNotification();
        updateFromProvider();
        setBluetoothOppService(this);
        return true;
    }

    @Override
    public boolean stop() {
        setBluetoothOppService(null);
        mHandler.sendMessage(mHandler.obtainMessage(STOP_LISTENER));
        return true;
    }

    private void startListener() {
        if (!mListenStarted) {
            if (mAdapter.isEnabled()) {
                if (V) {
                    Log.v(TAG, "Starting RfcommListener");
                }
                mHandler.sendMessage(mHandler.obtainMessage(START_LISTENER));
                mListenStarted = true;
            }
        }
    }

    @Override
    public void dump(StringBuilder sb) {
        super.dump(sb);
        if (mShares.size() > 0) {
            println(sb, "Shares:");
            for (BluetoothOppShareInfo info : mShares) {
                String dir = info.mDirection == BluetoothShare.DIRECTION_OUTBOUND ? " -> " : " <- ";
                SimpleDateFormat format = new SimpleDateFormat("MM-dd HH:mm:ss", Locale.US);
                Date date = new Date(info.mTimestamp);
                println(sb, "  " + format.format(date) + dir + info.mCurrentBytes + "/"
                        + info.mTotalBytes);
            }
        }
    }

    /**
     * Get the current instance of {@link BluetoothOppService}
     *
     * @return current instance of {@link BluetoothOppService}
     */
    @VisibleForTesting
    public static synchronized BluetoothOppService getBluetoothOppService() {
        if (sBluetoothOppService == null) {
            Log.w(TAG, "getBluetoothOppService(): service is null");
            return null;
        }
        if (!sBluetoothOppService.isAvailable()) {
            Log.w(TAG, "getBluetoothOppService(): service is not available");
            return null;
        }
        return sBluetoothOppService;
    }

    private static synchronized void setBluetoothOppService(BluetoothOppService instance) {
        if (D) {
            Log.d(TAG, "setBluetoothOppService(): set to: " + instance);
        }
        sBluetoothOppService = instance;
    }

    private static final int START_LISTENER = 1;

    private static final int MEDIA_SCANNED = 2;

    private static final int MEDIA_SCANNED_FAILED = 3;

    private static final int MSG_INCOMING_CONNECTION_RETRY = 4;

    private static final int MSG_INCOMING_BTOPP_CONNECTION = 100;

    private static final int STOP_LISTENER = 200;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case STOP_LISTENER:
                    stopListeners();
                    mListenStarted = false;
                    //Stop Active INBOUND Transfer
                    if (mServerTransfer != null) {
                        mServerTransfer.onBatchCanceled();
                        mServerTransfer = null;
                    }
                    //Stop Active OUTBOUND Transfer
                    if (mTransfer != null) {
                        mTransfer.onBatchCanceled();
                        mTransfer = null;
                    }
                    unregisterReceivers();
                    synchronized (BluetoothOppService.this) {
                        if (mUpdateThread != null) {
                            try {
                                mUpdateThread.interrupt();
                                mUpdateThread.join();
                            } catch (InterruptedException e) {
                                Log.e(TAG, "Interrupted", e);
                            }
                            mUpdateThread = null;
                        }
                    }
                    mNotifier.cancelNotifications();
                    break;
                case START_LISTENER:
                    if (mAdapter.isEnabled()) {
                        startSocketListener();
                    }
                    break;
                case MEDIA_SCANNED:
                    if (V) {
                        Log.v(TAG, "Update mInfo.id " + msg.arg1 + " for data uri= "
                                + msg.obj.toString());
                    }
                    ContentValues updateValues = new ContentValues();
                    Uri contentUri = Uri.parse(BluetoothShare.CONTENT_URI + "/" + msg.arg1);
                    updateValues.put(Constants.MEDIA_SCANNED, Constants.MEDIA_SCANNED_SCANNED_OK);
                    updateValues.put(BluetoothShare.URI, msg.obj.toString()); // update
                    updateValues.put(BluetoothShare.MIMETYPE,
                            getContentResolver().getType(Uri.parse(msg.obj.toString())));
                    getContentResolver().update(contentUri, updateValues, null, null);
                    synchronized (BluetoothOppService.this) {
                        mMediaScanInProgress = false;
                    }
                    break;
                case MEDIA_SCANNED_FAILED:
                    Log.v(TAG, "Update mInfo.id " + msg.arg1 + " for MEDIA_SCANNED_FAILED");
                    ContentValues updateValues1 = new ContentValues();
                    Uri contentUri1 = Uri.parse(BluetoothShare.CONTENT_URI + "/" + msg.arg1);
                    updateValues1.put(Constants.MEDIA_SCANNED,
                            Constants.MEDIA_SCANNED_SCANNED_FAILED);
                    getContentResolver().update(contentUri1, updateValues1, null, null);
                    synchronized (BluetoothOppService.this) {
                        mMediaScanInProgress = false;
                    }
                    break;
                case MSG_INCOMING_BTOPP_CONNECTION:
                    if (D) {
                        Log.d(TAG, "Get incoming connection");
                    }
                    ObexTransport transport = (ObexTransport) msg.obj;

                    /*
                     * Strategy for incoming connections:
                     * 1. If there is no ongoing transfer, no on-hold connection, start it
                     * 2. If there is ongoing transfer, hold it for 20 seconds(1 seconds * 20 times)
                     * 3. If there is on-hold connection, reject directly
                     */
                    if (mBatches.size() == 0 && mPendingConnection == null) {
                        Log.i(TAG, "Start Obex Server");
                        createServerSession(transport);
                    } else {
                        if (mPendingConnection != null) {
                            Log.w(TAG, "OPP busy! Reject connection");
                            try {
                                transport.close();
                            } catch (IOException e) {
                                Log.e(TAG, "close tranport error");
                            }
                        } else {
                            Log.i(TAG, "OPP busy! Retry after 1 second");
                            mIncomingRetries = mIncomingRetries + 1;
                            mPendingConnection = transport;
                            Message msg1 = Message.obtain(mHandler);
                            msg1.what = MSG_INCOMING_CONNECTION_RETRY;
                            mHandler.sendMessageDelayed(msg1, 1000);
                        }
                    }
                    break;
                case MSG_INCOMING_CONNECTION_RETRY:
                    if (mBatches.size() == 0) {
                        Log.i(TAG, "Start Obex Server");
                        createServerSession(mPendingConnection);
                        mIncomingRetries = 0;
                        mPendingConnection = null;
                    } else {
                        if (mIncomingRetries == 20) {
                            Log.w(TAG, "Retried 20 seconds, reject connection");
                            try {
                                mPendingConnection.close();
                            } catch (IOException e) {
                                Log.e(TAG, "close tranport error");
                            }
                            if (mServerSocket != null) {
                                acceptNewConnections();
                            }
                            mIncomingRetries = 0;
                            mPendingConnection = null;
                        } else {
                            Log.i(TAG, "OPP busy! Retry after 1 second");
                            mIncomingRetries = mIncomingRetries + 1;
                            Message msg2 = Message.obtain(mHandler);
                            msg2.what = MSG_INCOMING_CONNECTION_RETRY;
                            mHandler.sendMessageDelayed(msg2, 1000);
                        }
                    }
                    break;
            }
        }
    };

    private ObexServerSockets mServerSocket;

    private void startSocketListener() {
        if (D) {
            Log.d(TAG, "start Socket Listeners");
        }
        stopListeners();
        mServerSocket = ObexServerSockets.createInsecure(this);
        acceptNewConnections();
        SdpManager sdpManager = SdpManager.getDefaultManager();
        if (sdpManager == null || mServerSocket == null) {
            Log.e(TAG, "ERROR:serversocket object is NULL  sdp manager :" + sdpManager
                    + " mServerSocket:" + mServerSocket);
            return;
        }
        mOppSdpHandle =
                sdpManager.createOppOpsRecord("OBEX Object Push", mServerSocket.getRfcommChannel(),
                        mServerSocket.getL2capPsm(), 0x0102, SUPPORTED_OPP_FORMAT);
        if (D) {
            Log.d(TAG, "mOppSdpHandle :" + mOppSdpHandle);
        }
    }

    @Override
    protected void cleanup() {
        if (V) {
            Log.v(TAG, "onDestroy");
        }
        stopListeners();
        if (mBatches != null) {
            mBatches.clear();
        }
        if (mShares != null) {
            mShares.clear();
        }
        if (mHandler != null) {
            mHandler.removeCallbacksAndMessages(null);
        }
    }

    private void unregisterReceivers() {
        try {
            if (mObserver != null) {
                getContentResolver().unregisterContentObserver(mObserver);
                mObserver = null;
            }
            unregisterReceiver(mBluetoothReceiver);
        } catch (IllegalArgumentException e) {
            Log.w(TAG, "unregisterReceivers " + e.toString());
        }
    }

    /* suppose we auto accept an incoming OPUSH connection */
    private void createServerSession(ObexTransport transport) {
        mServerSession = new BluetoothOppObexServerSession(this, transport, this);
        mServerSession.preStart();
        if (D) {
            Log.d(TAG, "Get ServerSession " + mServerSession.toString() + " for incoming connection"
                    + transport.toString());
        }
    }

    private final BroadcastReceiver mBluetoothReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {
                switch (intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR)) {
                    case BluetoothAdapter.STATE_ON:
                        if (V) {
                            Log.v(TAG, "Bluetooth state changed: STATE_ON");
                        }
                        startListener();
                        // If this is within a sending process, continue the handle
                        // logic to display device picker dialog.
                        synchronized (this) {
                            if (BluetoothOppManager.getInstance(context).mSendingFlag) {
                                // reset the flags
                                BluetoothOppManager.getInstance(context).mSendingFlag = false;

                                Intent in1 = new Intent(BluetoothDevicePicker.ACTION_LAUNCH);
                                in1.putExtra(BluetoothDevicePicker.EXTRA_NEED_AUTH, false);
                                in1.putExtra(BluetoothDevicePicker.EXTRA_FILTER_TYPE,
                                        BluetoothDevicePicker.FILTER_TYPE_TRANSFER);
                                in1.putExtra(BluetoothDevicePicker.EXTRA_LAUNCH_PACKAGE,
                                        Constants.THIS_PACKAGE_NAME);
                                in1.putExtra(BluetoothDevicePicker.EXTRA_LAUNCH_CLASS,
                                        BluetoothOppReceiver.class.getName());

                                in1.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                                context.startActivity(in1);
                            }
                        }

                        break;
                    case BluetoothAdapter.STATE_TURNING_OFF:
                        if (V) {
                            Log.v(TAG, "Bluetooth state changed: STATE_TURNING_OFF");
                        }
                        mHandler.sendMessage(mHandler.obtainMessage(STOP_LISTENER));
                        break;
                }
            }
        }
    };

    private void updateFromProvider() {
        synchronized (BluetoothOppService.this) {
            mPendingUpdate = true;
            if (mUpdateThread == null) {
                mUpdateThread = new UpdateThread();
                mUpdateThread.start();
            }
        }
    }

    private class UpdateThread extends Thread {
        private boolean mIsInterrupted;

        UpdateThread() {
            super("Bluetooth Share Service");
            mIsInterrupted = false;
        }

        @Override
        public void interrupt() {
            mIsInterrupted = true;
            if (D) {
                Log.d(TAG, "OPP UpdateThread interrupted ");
            }
            super.interrupt();
        }


        @Override
        public void run() {
            Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND);

            while (!mIsInterrupted) {
                synchronized (BluetoothOppService.this) {
                    if (mUpdateThread != this) {
                        throw new IllegalStateException(
                                "multiple UpdateThreads in BluetoothOppService");
                    }
                    if (V) {
                        Log.v(TAG, "pendingUpdate is " + mPendingUpdate + " sListenStarted is "
                                + mListenStarted + " isInterrupted :" + mIsInterrupted);
                    }
                    if (!mPendingUpdate) {
                        mUpdateThread = null;
                        return;
                    }
                    mPendingUpdate = false;
                }
                Cursor cursor =
                        getContentResolver().query(BluetoothShare.CONTENT_URI, null, null, null,
                                BluetoothShare._ID);

                if (cursor == null) {
                    return;
                }

                cursor.moveToFirst();

                int arrayPos = 0;

                boolean isAfterLast = cursor.isAfterLast();

                int idColumn = cursor.getColumnIndexOrThrow(BluetoothShare._ID);
                /*
                 * Walk the cursor and the local array to keep them in sync. The
                 * key to the algorithm is that the ids are unique and sorted
                 * both in the cursor and in the array, so that they can be
                 * processed in order in both sources at the same time: at each
                 * step, both sources point to the lowest id that hasn't been
                 * processed from that source, and the algorithm processes the
                 * lowest id from those two possibilities. At each step: -If the
                 * array contains an entry that's not in the cursor, remove the
                 * entry, move to next entry in the array. -If the array
                 * contains an entry that's in the cursor, nothing to do, move
                 * to next cursor row and next array entry. -If the cursor
                 * contains an entry that's not in the array, insert a new entry
                 * in the array, move to next cursor row and next array entry.
                 */
                while (!isAfterLast || arrayPos < mShares.size() && mListenStarted) {
                    if (isAfterLast) {
                        // We're beyond the end of the cursor but there's still some
                        // stuff in the local array, which can only be junk
                        if (mShares.size() != 0) {
                            if (V) {
                                Log.v(TAG, "Array update: trimming " + mShares.get(arrayPos).mId
                                        + " @ " + arrayPos);
                            }
                        }

                        if (shouldScanFile(arrayPos)) {
                            scanFile(arrayPos);
                        }
                        deleteShare(arrayPos); // this advances in the array
                    } else {
                        int id = cursor.getInt(idColumn);

                        if (arrayPos == mShares.size()) {
                            insertShare(cursor, arrayPos);
                            if (V) {
                                Log.v(TAG, "Array update: inserting " + id + " @ " + arrayPos);
                            }
                            ++arrayPos;
                            cursor.moveToNext();
                            isAfterLast = cursor.isAfterLast();
                        } else {
                            int arrayId = 0;
                            if (mShares.size() != 0) {
                                arrayId = mShares.get(arrayPos).mId;
                            }

                            if (arrayId < id) {
                                if (V) {
                                    Log.v(TAG,
                                            "Array update: removing " + arrayId + " @ " + arrayPos);
                                }
                                if (shouldScanFile(arrayPos)) {
                                    scanFile(arrayPos);
                                }
                                deleteShare(arrayPos);
                            } else if (arrayId == id) {
                                // This cursor row already exists in the stored array.
                                updateShare(cursor, arrayPos);

                                ++arrayPos;
                                cursor.moveToNext();
                                isAfterLast = cursor.isAfterLast();
                            } else {
                                // This cursor entry didn't exist in the stored
                                // array
                                if (V) {
                                    Log.v(TAG, "Array update: appending " + id + " @ " + arrayPos);
                                }
                                insertShare(cursor, arrayPos);

                                ++arrayPos;
                                cursor.moveToNext();
                                isAfterLast = cursor.isAfterLast();
                            }
                        }
                    }
                }

                mNotifier.updateNotification();

                cursor.close();
            }
        }
    }

    private void insertShare(Cursor cursor, int arrayPos) {
        String uriString = cursor.getString(cursor.getColumnIndexOrThrow(BluetoothShare.URI));
        Uri uri;
        if (uriString != null) {
            uri = Uri.parse(uriString);
            Log.d(TAG, "insertShare parsed URI: " + uri);
        } else {
            uri = null;
            Log.e(TAG, "insertShare found null URI at cursor!");
        }
        BluetoothOppShareInfo info = new BluetoothOppShareInfo(
                cursor.getInt(cursor.getColumnIndexOrThrow(BluetoothShare._ID)), uri,
                cursor.getString(cursor.getColumnIndexOrThrow(BluetoothShare.FILENAME_HINT)),
                cursor.getString(cursor.getColumnIndexOrThrow(BluetoothShare._DATA)),
                cursor.getString(cursor.getColumnIndexOrThrow(BluetoothShare.MIMETYPE)),
                cursor.getInt(cursor.getColumnIndexOrThrow(BluetoothShare.DIRECTION)),
                cursor.getString(cursor.getColumnIndexOrThrow(BluetoothShare.DESTINATION)),
                cursor.getInt(cursor.getColumnIndexOrThrow(BluetoothShare.VISIBILITY)),
                cursor.getInt(cursor.getColumnIndexOrThrow(BluetoothShare.USER_CONFIRMATION)),
                cursor.getInt(cursor.getColumnIndexOrThrow(BluetoothShare.STATUS)),
                cursor.getLong(cursor.getColumnIndexOrThrow(BluetoothShare.TOTAL_BYTES)),
                cursor.getLong(cursor.getColumnIndexOrThrow(BluetoothShare.CURRENT_BYTES)),
                cursor.getLong(cursor.getColumnIndexOrThrow(BluetoothShare.TIMESTAMP)),
                cursor.getInt(cursor.getColumnIndexOrThrow(Constants.MEDIA_SCANNED))
                        != Constants.MEDIA_SCANNED_NOT_SCANNED);

        if (V) {
            Log.v(TAG, "Service adding new entry");
            Log.v(TAG, "ID      : " + info.mId);
            // Log.v(TAG, "URI     : " + ((info.mUri != null) ? "yes" : "no"));
            Log.v(TAG, "URI     : " + info.mUri);
            Log.v(TAG, "HINT    : " + info.mHint);
            Log.v(TAG, "FILENAME: " + info.mFilename);
            Log.v(TAG, "MIMETYPE: " + info.mMimetype);
            Log.v(TAG, "DIRECTION: " + info.mDirection);
            Log.v(TAG, "DESTINAT: " + info.mDestination);
            Log.v(TAG, "VISIBILI: " + info.mVisibility);
            Log.v(TAG, "CONFIRM : " + info.mConfirm);
            Log.v(TAG, "STATUS  : " + info.mStatus);
            Log.v(TAG, "TOTAL   : " + info.mTotalBytes);
            Log.v(TAG, "CURRENT : " + info.mCurrentBytes);
            Log.v(TAG, "TIMESTAMP : " + info.mTimestamp);
            Log.v(TAG, "SCANNED : " + info.mMediaScanned);
        }

        mShares.add(arrayPos, info);

        /* Mark the info as failed if it's in invalid status */
        if (info.isObsolete()) {
            Constants.updateShareStatus(this, info.mId, BluetoothShare.STATUS_UNKNOWN_ERROR);
        }
        /*
         * Add info into a batch. The logic is
         * 1) Only add valid and readyToStart info
         * 2) If there is no batch, create a batch and insert this transfer into batch,
         * then run the batch
         * 3) If there is existing batch and timestamp match, insert transfer into batch
         * 4) If there is existing batch and timestamp does not match, create a new batch and
         * put in queue
         */

        if (info.isReadyToStart()) {
            if (info.mDirection == BluetoothShare.DIRECTION_OUTBOUND) {
                /* check if the file exists */
                BluetoothOppSendFileInfo sendFileInfo =
                        BluetoothOppUtility.getSendFileInfo(info.mUri);
                if (sendFileInfo == null || sendFileInfo.mInputStream == null) {
                    Log.e(TAG, "Can't open file for OUTBOUND info " + info.mId);
                    Constants.updateShareStatus(this, info.mId, BluetoothShare.STATUS_BAD_REQUEST);
                    BluetoothOppUtility.closeSendFileInfo(info.mUri);
                    return;
                }
            }
            if (mBatches.size() == 0) {
                BluetoothOppBatch newBatch = new BluetoothOppBatch(this, info);
                newBatch.mId = mBatchId;
                mBatchId++;
                mBatches.add(newBatch);
                if (info.mDirection == BluetoothShare.DIRECTION_OUTBOUND) {
                    if (V) {
                        Log.v(TAG,
                                "Service create new Batch " + newBatch.mId + " for OUTBOUND info "
                                        + info.mId);
                    }
                    mTransfer = new BluetoothOppTransfer(this, newBatch);
                } else if (info.mDirection == BluetoothShare.DIRECTION_INBOUND) {
                    if (V) {
                        Log.v(TAG, "Service create new Batch " + newBatch.mId + " for INBOUND info "
                                + info.mId);
                    }
                    mServerTransfer = new BluetoothOppTransfer(this, newBatch, mServerSession);
                }

                if (info.mDirection == BluetoothShare.DIRECTION_OUTBOUND && mTransfer != null) {
                    if (V) {
                        Log.v(TAG, "Service start transfer new Batch " + newBatch.mId + " for info "
                                + info.mId);
                    }
                    mTransfer.start();
                } else if (info.mDirection == BluetoothShare.DIRECTION_INBOUND
                        && mServerTransfer != null) {
                    if (V) {
                        Log.v(TAG, "Service start server transfer new Batch " + newBatch.mId
                                + " for info " + info.mId);
                    }
                    mServerTransfer.start();
                }

            } else {
                int i = findBatchWithTimeStamp(info.mTimestamp);
                if (i != -1) {
                    if (V) {
                        Log.v(TAG, "Service add info " + info.mId + " to existing batch " + mBatches
                                .get(i).mId);
                    }
                    mBatches.get(i).addShare(info);
                } else {
                    // There is ongoing batch
                    BluetoothOppBatch newBatch = new BluetoothOppBatch(this, info);
                    newBatch.mId = mBatchId;
                    mBatchId++;
                    mBatches.add(newBatch);
                    if (V) {
                        Log.v(TAG,
                                "Service add new Batch " + newBatch.mId + " for info " + info.mId);
                    }
                }
            }
        }
    }

    private void updateShare(Cursor cursor, int arrayPos) {
        BluetoothOppShareInfo info = mShares.get(arrayPos);
        int statusColumn = cursor.getColumnIndexOrThrow(BluetoothShare.STATUS);

        info.mId = cursor.getInt(cursor.getColumnIndexOrThrow(BluetoothShare._ID));
        if (info.mUri != null) {
            info.mUri =
                    Uri.parse(stringFromCursor(info.mUri.toString(), cursor, BluetoothShare.URI));
        } else {
            Log.w(TAG, "updateShare() called for ID " + info.mId + " with null URI");
        }
        info.mHint = stringFromCursor(info.mHint, cursor, BluetoothShare.FILENAME_HINT);
        info.mFilename = stringFromCursor(info.mFilename, cursor, BluetoothShare._DATA);
        info.mMimetype = stringFromCursor(info.mMimetype, cursor, BluetoothShare.MIMETYPE);
        info.mDirection = cursor.getInt(cursor.getColumnIndexOrThrow(BluetoothShare.DIRECTION));
        info.mDestination = stringFromCursor(info.mDestination, cursor, BluetoothShare.DESTINATION);
        int newVisibility = cursor.getInt(cursor.getColumnIndexOrThrow(BluetoothShare.VISIBILITY));

        boolean confirmUpdated = false;
        int newConfirm =
                cursor.getInt(cursor.getColumnIndexOrThrow(BluetoothShare.USER_CONFIRMATION));

        if (info.mVisibility == BluetoothShare.VISIBILITY_VISIBLE
                && newVisibility != BluetoothShare.VISIBILITY_VISIBLE && (
                BluetoothShare.isStatusCompleted(info.mStatus)
                        || newConfirm == BluetoothShare.USER_CONFIRMATION_PENDING)) {
            mNotifier.mNotificationMgr.cancel(info.mId);
        }

        info.mVisibility = newVisibility;

        if (info.mConfirm == BluetoothShare.USER_CONFIRMATION_PENDING
                && newConfirm != BluetoothShare.USER_CONFIRMATION_PENDING) {
            confirmUpdated = true;
        }
        info.mConfirm =
                cursor.getInt(cursor.getColumnIndexOrThrow(BluetoothShare.USER_CONFIRMATION));
        int newStatus = cursor.getInt(statusColumn);

        if (BluetoothShare.isStatusCompleted(info.mStatus)) {
            mNotifier.mNotificationMgr.cancel(info.mId);
        }

        info.mStatus = newStatus;
        info.mTotalBytes = cursor.getLong(cursor.getColumnIndexOrThrow(BluetoothShare.TOTAL_BYTES));
        info.mCurrentBytes =
                cursor.getLong(cursor.getColumnIndexOrThrow(BluetoothShare.CURRENT_BYTES));
        info.mTimestamp = cursor.getLong(cursor.getColumnIndexOrThrow(BluetoothShare.TIMESTAMP));
        info.mMediaScanned = (cursor.getInt(cursor.getColumnIndexOrThrow(Constants.MEDIA_SCANNED))
                != Constants.MEDIA_SCANNED_NOT_SCANNED);

        if (confirmUpdated) {
            if (V) {
                Log.v(TAG, "Service handle info " + info.mId + " confirmation updated");
            }
            /* Inbounds transfer user confirmation status changed, update the session server */
            int i = findBatchWithTimeStamp(info.mTimestamp);
            if (i != -1) {
                BluetoothOppBatch batch = mBatches.get(i);
                if (mServerTransfer != null && batch.mId == mServerTransfer.getBatchId()) {
                    mServerTransfer.confirmStatusChanged();
                } //TODO need to think about else
            }
        }
        int i = findBatchWithTimeStamp(info.mTimestamp);
        if (i != -1) {
            BluetoothOppBatch batch = mBatches.get(i);
            if (batch.mStatus == Constants.BATCH_STATUS_FINISHED
                    || batch.mStatus == Constants.BATCH_STATUS_FAILED) {
                if (V) {
                    Log.v(TAG, "Batch " + batch.mId + " is finished");
                }
                if (batch.mDirection == BluetoothShare.DIRECTION_OUTBOUND) {
                    if (mTransfer == null) {
                        Log.e(TAG, "Unexpected error! mTransfer is null");
                    } else if (batch.mId == mTransfer.getBatchId()) {
                        mTransfer.stop();
                    } else {
                        Log.e(TAG, "Unexpected error! batch id " + batch.mId
                                + " doesn't match mTransfer id " + mTransfer.getBatchId());
                    }
                    mTransfer = null;
                } else {
                    if (mServerTransfer == null) {
                        Log.e(TAG, "Unexpected error! mServerTransfer is null");
                    } else if (batch.mId == mServerTransfer.getBatchId()) {
                        mServerTransfer.stop();
                    } else {
                        Log.e(TAG, "Unexpected error! batch id " + batch.mId
                                + " doesn't match mServerTransfer id "
                                + mServerTransfer.getBatchId());
                    }
                    mServerTransfer = null;
                }
                removeBatch(batch);
            }
        }
    }

    /**
     * Removes the local copy of the info about a share.
     */
    private void deleteShare(int arrayPos) {
        BluetoothOppShareInfo info = mShares.get(arrayPos);

        /*
         * Delete arrayPos from a batch. The logic is
         * 1) Search existing batch for the info
         * 2) cancel the batch
         * 3) If the batch become empty delete the batch
         */
        int i = findBatchWithTimeStamp(info.mTimestamp);
        if (i != -1) {
            BluetoothOppBatch batch = mBatches.get(i);
            if (batch.hasShare(info)) {
                if (V) {
                    Log.v(TAG, "Service cancel batch for share " + info.mId);
                }
                batch.cancelBatch();
            }
            if (batch.isEmpty()) {
                if (V) {
                    Log.v(TAG, "Service remove batch  " + batch.mId);
                }
                removeBatch(batch);
            }
        }
        mShares.remove(arrayPos);
    }

    private String stringFromCursor(String old, Cursor cursor, String column) {
        int index = cursor.getColumnIndexOrThrow(column);
        if (old == null) {
            return cursor.getString(index);
        }
        if (mNewChars == null) {
            mNewChars = new CharArrayBuffer(128);
        }
        cursor.copyStringToBuffer(index, mNewChars);
        int length = mNewChars.sizeCopied;
        if (length != old.length()) {
            return cursor.getString(index);
        }
        if (mOldChars == null || mOldChars.sizeCopied < length) {
            mOldChars = new CharArrayBuffer(length);
        }
        char[] oldArray = mOldChars.data;
        char[] newArray = mNewChars.data;
        old.getChars(0, length, oldArray, 0);
        for (int i = length - 1; i >= 0; --i) {
            if (oldArray[i] != newArray[i]) {
                return new String(newArray, 0, length);
            }
        }
        return old;
    }

    private int findBatchWithTimeStamp(long timestamp) {
        for (int i = mBatches.size() - 1; i >= 0; i--) {
            if (mBatches.get(i).mTimestamp == timestamp) {
                return i;
            }
        }
        return -1;
    }

    private void removeBatch(BluetoothOppBatch batch) {
        if (V) {
            Log.v(TAG, "Remove batch " + batch.mId);
        }
        mBatches.remove(batch);
        if (mBatches.size() > 0) {
            for (BluetoothOppBatch nextBatch : mBatches) {
                // we have a running batch
                if (nextBatch.mStatus == Constants.BATCH_STATUS_RUNNING) {
                    return;
                } else {
                    // just finish a transfer, start pending outbound transfer
                    if (nextBatch.mDirection == BluetoothShare.DIRECTION_OUTBOUND) {
                        if (V) {
                            Log.v(TAG, "Start pending outbound batch " + nextBatch.mId);
                        }
                        mTransfer = new BluetoothOppTransfer(this, nextBatch);
                        mTransfer.start();
                        return;
                    } else if (nextBatch.mDirection == BluetoothShare.DIRECTION_INBOUND
                            && mServerSession != null) {
                        // have to support pending inbound transfer
                        // if an outbound transfer and incoming socket happens together
                        if (V) {
                            Log.v(TAG, "Start pending inbound batch " + nextBatch.mId);
                        }
                        mServerTransfer = new BluetoothOppTransfer(this, nextBatch, mServerSession);
                        mServerTransfer.start();
                        if (nextBatch.getPendingShare() != null
                                && nextBatch.getPendingShare().mConfirm
                                == BluetoothShare.USER_CONFIRMATION_CONFIRMED) {
                            mServerTransfer.confirmStatusChanged();
                        }
                        return;
                    }
                }
            }
        }
    }

    private boolean scanFile(int arrayPos) {
        BluetoothOppShareInfo info = mShares.get(arrayPos);
        synchronized (BluetoothOppService.this) {
            if (D) {
                Log.d(TAG, "Scanning file " + info.mFilename);
            }
            if (!mMediaScanInProgress) {
                mMediaScanInProgress = true;
                new MediaScannerNotifier(this, info, mHandler);
                return true;
            } else {
                return false;
            }
        }
    }

    private boolean shouldScanFile(int arrayPos) {
        BluetoothOppShareInfo info = mShares.get(arrayPos);
        return BluetoothShare.isStatusSuccess(info.mStatus)
                && info.mDirection == BluetoothShare.DIRECTION_INBOUND && !info.mMediaScanned
                && info.mConfirm != BluetoothShare.USER_CONFIRMATION_HANDOVER_CONFIRMED;
    }

    // Run in a background thread at boot.
    private static void trimDatabase(ContentResolver contentResolver) {
        // remove the invisible/unconfirmed inbound shares
        int delNum = contentResolver.delete(BluetoothShare.CONTENT_URI, WHERE_INVISIBLE_UNCONFIRMED,
                null);
        if (V) {
            Log.v(TAG, "Deleted shares, number = " + delNum);
        }

        // Keep the latest inbound and successful shares.
        Cursor cursor =
                contentResolver.query(BluetoothShare.CONTENT_URI, new String[]{BluetoothShare._ID},
                        WHERE_INBOUND_SUCCESS, null, BluetoothShare._ID); // sort by id
        if (cursor == null) {
            return;
        }
        int recordNum = cursor.getCount();
        if (recordNum > Constants.MAX_RECORDS_IN_DATABASE) {
            int numToDelete = recordNum - Constants.MAX_RECORDS_IN_DATABASE;

            if (cursor.moveToPosition(numToDelete)) {
                int columnId = cursor.getColumnIndexOrThrow(BluetoothShare._ID);
                long id = cursor.getLong(columnId);
                delNum = contentResolver.delete(BluetoothShare.CONTENT_URI,
                        BluetoothShare._ID + " < " + id, null);
                if (V) {
                    Log.v(TAG, "Deleted old inbound success share: " + delNum);
                }
            }
        }
        cursor.close();
    }

    private static class MediaScannerNotifier implements MediaScannerConnectionClient {

        private MediaScannerConnection mConnection;

        private BluetoothOppShareInfo mInfo;

        private Context mContext;

        private Handler mCallback;

        MediaScannerNotifier(Context context, BluetoothOppShareInfo info, Handler handler) {
            mContext = context;
            mInfo = info;
            mCallback = handler;
            mConnection = new MediaScannerConnection(mContext, this);
            if (V) {
                Log.v(TAG, "Connecting to MediaScannerConnection ");
            }
            mConnection.connect();
        }

        @Override
        public void onMediaScannerConnected() {
            if (V) {
                Log.v(TAG, "MediaScannerConnection onMediaScannerConnected");
            }
            mConnection.scanFile(mInfo.mFilename, mInfo.mMimetype);
        }

        @Override
        public void onScanCompleted(String path, Uri uri) {
            try {
                if (V) {
                    Log.v(TAG, "MediaScannerConnection onScanCompleted");
                    Log.v(TAG, "MediaScannerConnection path is " + path);
                    Log.v(TAG, "MediaScannerConnection Uri is " + uri);
                }
                if (uri != null) {
                    Message msg = Message.obtain();
                    msg.setTarget(mCallback);
                    msg.what = MEDIA_SCANNED;
                    msg.arg1 = mInfo.mId;
                    msg.obj = uri;
                    msg.sendToTarget();
                } else {
                    Message msg = Message.obtain();
                    msg.setTarget(mCallback);
                    msg.what = MEDIA_SCANNED_FAILED;
                    msg.arg1 = mInfo.mId;
                    msg.sendToTarget();
                }
            } catch (NullPointerException ex) {
                Log.v(TAG, "!!!MediaScannerConnection exception: " + ex);
            } finally {
                if (V) {
                    Log.v(TAG, "MediaScannerConnection disconnect");
                }
                mConnection.disconnect();
            }
        }
    }

    private void stopListeners() {
        if (mAdapter != null && mOppSdpHandle >= 0 && SdpManager.getDefaultManager() != null) {
            if (D) {
                Log.d(TAG, "Removing SDP record mOppSdpHandle :" + mOppSdpHandle);
            }
            boolean status = SdpManager.getDefaultManager().removeSdpRecord(mOppSdpHandle);
            Log.d(TAG, "RemoveSDPrecord returns " + status);
            mOppSdpHandle = -1;
        }
        if (mServerSocket != null) {
            mServerSocket.shutdown(false);
            mServerSocket = null;
        }
        if (D) {
            Log.d(TAG, "stopListeners: mServerSocket is null");
        }
    }

    @Override
    public boolean onConnect(BluetoothDevice device, BluetoothSocket socket) {

        if (D) {
            Log.d(TAG, " onConnect BluetoothSocket :" + socket + " \n :device :" + device);
        }
        if (!mAcceptNewConnections) {
            Log.d(TAG, " onConnect BluetoothSocket :" + socket + " rejected");
            return false;
        }
        BluetoothObexTransport transport = new BluetoothObexTransport(socket);
        Message msg = mHandler.obtainMessage(MSG_INCOMING_BTOPP_CONNECTION);
        msg.obj = transport;
        msg.sendToTarget();
        mAcceptNewConnections = false;
        return true;
    }

    @Override
    public void onAcceptFailed() {
        Log.d(TAG, " onAcceptFailed:");
        mHandler.sendMessage(mHandler.obtainMessage(START_LISTENER));
    }

    /**
     * Set mAcceptNewConnections to true to allow new connections.
     */
    void acceptNewConnections() {
        mAcceptNewConnections = true;
    }
}
