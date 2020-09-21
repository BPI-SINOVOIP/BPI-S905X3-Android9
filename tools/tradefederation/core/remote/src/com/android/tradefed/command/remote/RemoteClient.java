/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.tradefed.command.remote;

import com.android.ddmlib.Log;

import org.json.JSONException;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.List;

/**
 * Class for sending remote commands to another TF process.
 * <p/>
 * Currently uses JSON-encoded data sent via sockets.
 */
public class RemoteClient implements IRemoteClient {

    // choose an arbitrary default port that according to the interweb is not used by another
    // popular program
    public static final int DEFAULT_PORT = 30103;

    private static final String TAG = RemoteClient.class.getSimpleName();
    private final Socket mSocket;
    private final PrintWriter mWriter;
    private final BufferedReader mReader;

    /**
     * Initialize the {@RemoteClient}, and instruct it to connect to the given port
     * on localhost.
     *
     * @param port the tcp/ip port number
     * @throws IOException
     * @throws UnknownHostException
     */
    private RemoteClient(int port) throws UnknownHostException, IOException {
        this(InetAddress.getLocalHost().getHostName(), port);
    }

    /**
     * Initialize the {@RemoteClient}, and instruct it to connect to the given hostname and port.
     *
     * @param hostName to connect to
     * @param port the tcp/ip port number
     * @throws IOException
     * @throws UnknownHostException
     */
    private RemoteClient(String hostName, int port) throws UnknownHostException, IOException {
        mSocket = new Socket(hostName, port);
        mWriter = new PrintWriter(mSocket.getOutputStream(), true);
        mReader = new BufferedReader(new InputStreamReader(mSocket.getInputStream()));
    }

    /**
     * Send the given operation to the remote TF.
     *
     * @param op the {@link RemoteOperation} to send
     * @throws RemoteException if failed to perform operation
     */
    private synchronized <T> T sendOperation(RemoteOperation<T> op) throws RemoteException {
       try {
           Log.d(TAG, String.format("Sending remote op %s", op.getType()));
           mWriter.println(op.pack());
           String response = mReader.readLine();
           if (response == null) {
               throw new RemoteException("no response from remote manager");
           }
           return op.unpackResponseFromString(response);
       } catch (IOException e) {
           throw new RemoteException(e.getMessage(), e);
       } catch (JSONException e) {
           throw new RemoteException(e.getMessage(), e);
       }
    }

    /**
     * Helper method to create a {@link RemoteClient} connected to given port
     *
     * @param port the tcp/ip port
     * @return the {@link RemoteClient}
     * @throws RemoteException if failed to connect
     */
    public static IRemoteClient connect(int port) throws RemoteException {
        try {
            return new RemoteClient(port);
        } catch (IOException e) {
            throw new RemoteException(e);
        }
    }

    /**
     * Helper method to create a {@link RemoteClient} connected to given host and port
     *
     * @param hostname the host name
     * @param port the tcp/ip port
     * @return the {@link RemoteClient}
     * @throws RemoteException if failed to connect
     */
    public static IRemoteClient connect(String hostname, int port) throws RemoteException {
        try {
            return new RemoteClient(hostname, port);
        } catch (IOException e) {
            throw new RemoteException(e);
        }
    }

    /**
     * Helper method to create a {@link RemoteClient} connected to default port
     *
     * @return the {@link RemoteClient}
     * @throws RemoteException if failed to connect
     */
    public static IRemoteClient connect() throws RemoteException {
        return connect(DEFAULT_PORT);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void sendAllocateDevice(String serial) throws RemoteException {
        sendOperation(new AllocateDeviceOp(serial));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void sendFreeDevice(String serial) throws RemoteException {
        sendOperation(new FreeDeviceOp(serial));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void sendAddCommand(long totalTime, String... commandArgs) throws RemoteException {
        sendOperation(new AddCommandOp(totalTime, commandArgs));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void sendAddCommandFile(String commandFile, List<String> extraArgs)
            throws RemoteException {
        sendOperation(new AddCommandFileOp(commandFile, extraArgs));
    }

    /**
     * {@inheritDoc}
     */
    @Deprecated
    @Override
    public void sendClose() throws RemoteException {
        sendOperation(new CloseOp());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void sendStartHandover(int port) throws RemoteException {
        sendOperation(new StartHandoverOp(port));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void sendHandoverInitComplete() throws RemoteException {
        sendOperation(new HandoverInitCompleteOp());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void sendHandoverComplete() throws RemoteException {
        sendOperation(new HandoverCompleteOp());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<DeviceDescriptor> sendListDevices() throws RemoteException {
        return sendOperation(new ListDevicesOp());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void sendExecCommand(String serial, String[] commandArgs) throws RemoteException {
        sendOperation(new ExecCommandOp(serial, commandArgs));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void sendGetLastCommandResult(String serial, ICommandResultHandler handler)
            throws RemoteException {
        CommandResult r = sendOperation(new GetLastCommandResultOp(serial));
        switch (r.getStatus()) {
            case EXECUTING:
                handler.stillRunning();
                break;
            case INVOCATION_ERROR:
                handler.failure(r.getInvocationErrorDetails(), r.getFreeDeviceState(),
                        r.getRunMetrics());
                break;
            case INVOCATION_SUCCESS:
                handler.success(r.getRunMetrics());
                break;
            case NO_ACTIVE_COMMAND:
                handler.noActiveCommand();
                break;
            case NOT_ALLOCATED:
                handler.notAllocated();
                break;
            default:
                throw new RemoteException("unrecognized status " + r.getStatus().name());
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void close() {
        if (mSocket != null) {
            try {
                mSocket.close();
            } catch (IOException e) {
                Log.w(TAG, String.format("exception closing socket: %s", e.toString()));
            }
        }
        if (mWriter != null) {
            mWriter.close();
        }
    }
}
