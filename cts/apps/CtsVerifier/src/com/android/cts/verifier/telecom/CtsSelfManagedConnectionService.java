package com.android.cts.verifier.telecom;

import android.os.Bundle;
import android.telecom.Connection;
import android.telecom.ConnectionRequest;
import android.telecom.ConnectionService;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class CtsSelfManagedConnectionService extends ConnectionService {
    static final int TIMEOUT_MILLIS = 10000;

    private CtsConnection.Listener mConnectionListener =
            new CtsConnection.Listener() {
                @Override
                void onDestroyed(CtsConnection connection) {
                    synchronized (mConnectionsLock) {
                        mConnections.remove(connection);
                    }
                }
            };

    private static CtsSelfManagedConnectionService sConnectionService;
    private static CountDownLatch sBindingLatch = new CountDownLatch(1);

    List<CtsConnection> mConnections = new ArrayList<>();
    Object mConnectionsLock = new Object();
    CountDownLatch mConnectionLatch = new CountDownLatch(1);

    public static CtsSelfManagedConnectionService getConnectionService() {
        return sConnectionService;
    }

    public static CtsSelfManagedConnectionService waitForAndGetConnectionService() {
        if (sConnectionService == null) {
            try {
                sBindingLatch.await(TIMEOUT_MILLIS, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
            }
        }
        return sConnectionService;
    }

    public CtsSelfManagedConnectionService() throws Exception {
        super();
        sConnectionService = this;
        if (sBindingLatch != null) {
            sBindingLatch.countDown();
        }
        sBindingLatch = new CountDownLatch(1);
    }

    public List<CtsConnection> getConnections() {
        synchronized (mConnectionsLock) {
            return new ArrayList<CtsConnection>(mConnections);
        }
    }

    public CtsConnection waitForAndGetConnection() {
        try {
            mConnectionLatch.await(TIMEOUT_MILLIS, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
        }
        mConnectionLatch = new CountDownLatch(1);
        synchronized (mConnectionsLock) {
            if (mConnections.size() > 0) {
                return mConnections.get(0);
            } else {
                return null;
            }
        }
    }

    @Override
    public Connection onCreateIncomingConnection(PhoneAccountHandle connectionManagerPhoneAccount,
            ConnectionRequest request) {
        return createConnection(request, true /* isIncoming */);
    }

    @Override
    public Connection onCreateOutgoingConnection(PhoneAccountHandle connectionManagerAccount,
            ConnectionRequest request) {
        return createConnection(request, false /* isIncoming */);
    }

    private Connection createConnection(ConnectionRequest request, boolean isIncoming) {
        boolean useAudioClip =
                request.getExtras().getBoolean(CtsConnection.EXTRA_PLAY_CS_AUDIO, false);
        CtsConnection connection = new CtsConnection(getApplicationContext(), isIncoming,
                mConnectionListener, useAudioClip);
        connection.setConnectionCapabilities(Connection.CAPABILITY_SUPPORT_HOLD |
                Connection.CAPABILITY_HOLD);
        connection.setAddress(request.getAddress(), TelecomManager.PRESENTATION_ALLOWED);
        connection.setExtras(request.getExtras());

        Bundle moreExtras = new Bundle();
        moreExtras.putParcelable(TelecomManager.EXTRA_PHONE_ACCOUNT_HANDLE,
                request.getAccountHandle());
        connection.putExtras(moreExtras);
        connection.setVideoState(request.getVideoState());

        synchronized (mConnectionsLock) {
            mConnections.add(connection);
        }
        if (mConnectionLatch != null) {
            mConnectionLatch.countDown();
        }
        return connection;
    }
}
