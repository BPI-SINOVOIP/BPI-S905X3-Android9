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

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.command.ICommandScheduler;
import com.android.tradefed.command.remote.CommandResult.Status;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.FreeDeviceState;
import com.android.tradefed.device.IDeviceManager;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.StreamUtil;

import com.google.common.annotations.VisibleForTesting;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.net.SocketTimeoutException;

/**
 * Class that receives {@link com.android.tradefed.command.remote.RemoteOperation}s via a socket.
 * <p/>
 * Currently accepts only one remote connection at one time, and processes incoming commands
 * serially.
 * <p/>
 * Usage:
 * <pre>
 * RemoteManager r = new RemoteManager(deviceMgr, scheduler);
 * r.connect();
 * r.start();
 * int port = r.getPort();
 * ... inform client of port to use. Shuts down when instructed by client or on #cancel()
 * </pre>
 */
@OptionClass(alias = "remote-manager")
public class RemoteManager extends Thread {

    private ServerSocket mServerSocket = null;
    private boolean mCancel = false;
    private final IDeviceManager mDeviceManager;
    private final ICommandScheduler mScheduler;

    @Option(name = "start-remote-mgr",
            description = "Whether or not to start a remote manager on boot.")
    private static boolean mStartRemoteManagerOnBoot = false;

    @Option(name = "auto-handover",
            description = "Whether or not to start handover if there is another instance of " +
                          "Tradefederation running on the machine")
    private static boolean mAutoHandover = false;

    @Option(name = "remote-mgr-port",
            description = "The remote manager port to use.")
    private static int mRemoteManagerPort = RemoteClient.DEFAULT_PORT;

    @Option(name = "remote-mgr-socket-timeout-ms",
            description = "Timeout for when accepting connections with the remote manager socket.")
    private static int mSocketTimeout = 2000;

    public boolean getStartRemoteMgrOnBoot() {
        return mStartRemoteManagerOnBoot;
    }

    public int getRemoteManagerPort() {
        return mRemoteManagerPort;
    }

    public void setRemoteManagerPort(int port) {
        mRemoteManagerPort = port;
    }

    public void setRemoteManagerTimeout(int timeout) {
        mSocketTimeout = timeout;
    }

    public boolean getAutoHandover() {
        return mAutoHandover;
    }

    public RemoteManager() {
        super("RemoteManager");
        mDeviceManager = null;
        mScheduler = null;
    }

    /**
     * Creates a {@link RemoteManager}.
     *
     * @param manager the {@link IDeviceManager} to use to allocate and free devices.
     * @param scheduler the {@link ICommandScheduler} to use to schedule commands.
     */
    public RemoteManager(IDeviceManager manager, ICommandScheduler scheduler) {
        super("RemoteManager");
        mDeviceManager = manager;
        mScheduler = scheduler;
    }

    /**
     * Attempts to init server and connect it to a port.
     * @return true if we successfully connect the server to the default port.
     */
    public boolean connect() {
        return connect(mRemoteManagerPort);
    }

    /**
     * Attemps to connect to any free port.
     * @return true if we successfully connected to the port, false otherwise.
     */
    public boolean connectAnyPort() {
        return connect(0);
    }

    /**
     * Attempts to connect server to a given port.
     * @return true if we successfully connect to the port, false otherwise.
     */
    protected boolean connect(int port) {
        mServerSocket = openSocket(port);
        return mServerSocket != null;
    }

    /**
     * Attempts to open server socket at given port.
     * @param port to open the socket at.
     * @return the ServerSocket or null if attempt failed.
     */
    private ServerSocket openSocket(int port) {
        try {
            return new ServerSocket(port);
        } catch (IOException e) {
            // avoid printing a scary stack that is due to handover.
            CLog.w(
                    "Failed to open server socket: %s. Probably due to another instance of TF "
                            + "running.",
                    e.getMessage());
            return null;
        }
    }


    /**
     * The main thread body of the remote manager.
     * <p/>
     * Creates a server socket, and waits for client connections.
     */
    @Override
    public void run() {
        if (mServerSocket == null) {
            CLog.e("Started remote manager thread without connecting");
            return;
        }
        try {
            // Set a timeout as we don't want to be blocked listening for connections,
            // we could receive a request for cancel().
            mServerSocket.setSoTimeout(mSocketTimeout);
            processClientConnections(mServerSocket);
        } catch (SocketException e) {
            CLog.e("Error when setting socket timeout");
            CLog.e(e);
        } finally {
            freeAllDevices();
            closeSocket(mServerSocket);
        }
    }

    /**
     * Gets the socket port the remote manager is listening on, blocking for a short time if
     * necessary.
     * <p/>
     * {@link #start()} should be called before this method.
     * @return the port the remote manager is listening on, or -1 if no port is setup.
     */
    public synchronized int getPort() {
        if (mServerSocket == null) {
            try {
                wait(10*1000);
            } catch (InterruptedException e) {
                // ignore
            }
        }
        if (mServerSocket == null) {
            return -1;
        }
        return mServerSocket.getLocalPort();
    }

    private void processClientConnections(ServerSocket serverSocket) {
        while (!mCancel) {
            Socket clientSocket = null;
            BufferedReader in = null;
            PrintWriter out = null;
            try {
                clientSocket = serverSocket.accept();
                in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
                out = new PrintWriter(clientSocket.getOutputStream(), true);
                processClientOperations(in, out);
            } catch (SocketTimeoutException e) {
                // ignore.
            } catch (IOException e) {
                CLog.e("Failed to accept connection");
                CLog.e(e);
            } finally {
                closeReader(in);
                closeWriter(out);
                closeSocket(clientSocket);
            }
        }
    }

    /**
     * Process {@link com.android.tradefed.command.remote.RemoteClient} operations.
     *
     * @param in the {@link BufferedReader} coming from the client socket.
     * @param out the {@link PrintWriter} to write to the client socket.
     * @throws IOException
     */
    @VisibleForTesting
    void processClientOperations(BufferedReader in, PrintWriter out) throws IOException {
        String line = null;
        while ((line = in.readLine()) != null && !mCancel) {
            JSONObject result = new JSONObject();
            RemoteOperation<?> rc;
            Thread postOp = null;
            try {
                rc = RemoteOperation.createRemoteOpFromString(line);
                switch (rc.getType()) {
                    case ADD_COMMAND:
                        processAdd((AddCommandOp)rc, result);
                        break;
                    case ADD_COMMAND_FILE:
                        processAddCommandFile((AddCommandFileOp)rc, result);
                        break;
                    case CLOSE:
                        processClose((CloseOp)rc, result);
                        break;
                    case ALLOCATE_DEVICE:
                        processAllocate((AllocateDeviceOp)rc, result);
                        break;
                    case FREE_DEVICE:
                        processFree((FreeDeviceOp)rc, result);
                        break;
                    case START_HANDOVER:
                        postOp = processStartHandover((StartHandoverOp)rc, result);
                        break;
                    case HANDOVER_INIT_COMPLETE:
                        processHandoverInitComplete((HandoverInitCompleteOp)rc, result);
                        break;
                    case HANDOVER_COMPLETE:
                        postOp = processHandoverComplete((HandoverCompleteOp)rc, result);
                        break;
                    case LIST_DEVICES:
                        processListDevices((ListDevicesOp)rc, result);
                        break;
                    case EXEC_COMMAND:
                        processExecCommand((ExecCommandOp)rc, result);
                        break;
                    case GET_LAST_COMMAND_RESULT:
                        processGetLastCommandResult((GetLastCommandResultOp)rc, result);
                        break;
                    default:
                        result.put(RemoteOperation.ERROR, "Unrecognized operation");
                        break;
                }
            } catch (RemoteException e) {
                addErrorToResult(result, e);
            } catch (JSONException e) {
                addErrorToResult(result, e);
            } catch (RuntimeException e) {
                addErrorToResult(result, e);
            }
            sendAck(result, out);
            if (postOp != null) {
                postOp.start();
            }
        }
    }

    private void addErrorToResult(JSONObject result, Exception e) {
        try {
            CLog.e("Failed to handle remote command");
            CLog.e(e);
            result.put(RemoteOperation.ERROR, "Failed to handle remote command: " +
                    e.toString());
        } catch (JSONException e1) {
            CLog.e("Failed to build json remote response");
            CLog.e(e1);
        }
    }

    private void processListDevices(ListDevicesOp rc, JSONObject result) {
        try {
            rc.packResponseIntoJson(mDeviceManager.listAllDevices(), result);
        } catch (JSONException e) {
            addErrorToResult(result, e);
        }
    }

    @VisibleForTesting
    DeviceTracker getDeviceTracker() {
        return DeviceTracker.getInstance();
    }

    private Thread processStartHandover(StartHandoverOp c, JSONObject result) {
        final int port = c.getPort();
        CLog.logAndDisplay(LogLevel.INFO, "Performing handover to remote TF at port %d", port);
        // handle the handover as an async operation
        Thread t = new Thread("handover thread") {
            @Override
            public void run() {
                if (!mScheduler.handoverShutdown(port)) {
                    // TODO: send handover failed
                }
            }
        };
        return t;
    }

    private void processHandoverInitComplete(HandoverInitCompleteOp c, JSONObject result) {
        CLog.logAndDisplay(LogLevel.INFO, "Received handover complete.");
        mScheduler.handoverInitiationComplete();
    }

    private Thread processHandoverComplete(HandoverCompleteOp c, JSONObject result) {
        // handle the handover as an async operation
        Thread t = new Thread("handover thread") {
            @Override
            public void run() {
                mScheduler.completeHandover();
            }
        };
        return t;
    }

    private void processAllocate(AllocateDeviceOp c, JSONObject result) throws JSONException {
        ITestDevice allocatedDevice = mDeviceManager.forceAllocateDevice(c.getDeviceSerial());
        if (allocatedDevice != null) {
            CLog.logAndDisplay(LogLevel.INFO, "Remotely allocating device %s", c.getDeviceSerial());
            getDeviceTracker().allocateDevice(allocatedDevice);
        } else {
            String msg = "Failed to allocate device " + c.getDeviceSerial();
            CLog.e(msg);
            result.put(RemoteOperation.ERROR, msg);
        }
    }

    private void processFree(FreeDeviceOp c, JSONObject result) throws JSONException {
        if (FreeDeviceOp.ALL_DEVICES.equals(c.getDeviceSerial())) {
            freeAllDevices();
        } else {
            ITestDevice d = getDeviceTracker().freeDevice(c.getDeviceSerial());
            if (d != null) {
                CLog.logAndDisplay(LogLevel.INFO,
                        "Remotely freeing device %s",
                                c.getDeviceSerial());
                mDeviceManager.freeDevice(d, FreeDeviceState.AVAILABLE);
            } else {
                String msg = "Could not find device to free " + c.getDeviceSerial();
                CLog.w(msg);
                result.put(RemoteOperation.ERROR, msg);
            }
        }
    }

    private void processAdd(AddCommandOp c, JSONObject result) throws JSONException {
        CLog.logAndDisplay(LogLevel.INFO, "Adding command '%s'", ArrayUtil.join(" ",
                (Object[])c.getCommandArgs()));
        try {
            if (!mScheduler.addCommand(c.getCommandArgs(), c.getTotalTime())) {
                result.put(RemoteOperation.ERROR, "Failed to add command");
            }
        } catch (ConfigurationException e) {
            CLog.e("Failed to add command");
            CLog.e(e);
            result.put(RemoteOperation.ERROR, "Config error: " + e.toString());
        }
    }

    private void processAddCommandFile(AddCommandFileOp c, JSONObject result) throws JSONException {
        CLog.logAndDisplay(LogLevel.INFO, "Adding command file '%s %s'", c.getCommandFile(),
                ArrayUtil.join(" ", c.getExtraArgs()));
        try {
            mScheduler.addCommandFile(c.getCommandFile(), c.getExtraArgs());
        } catch (ConfigurationException e) {
            CLog.e("Failed to add command");
            CLog.e(e);
            result.put(RemoteOperation.ERROR, "Config error: " + e.toString());
        }
    }

    private void processExecCommand(ExecCommandOp c, JSONObject result) throws JSONException {
        ITestDevice device = getDeviceTracker().getDeviceForSerial(c.getDeviceSerial());
        if (device == null) {
            String msg = String.format("Could not find remotely allocated device with serial %s",
                    c.getDeviceSerial());
            CLog.e(msg);
            result.put(RemoteOperation.ERROR, msg);
            return;
        }
        ExecCommandTracker commandResult =
                getDeviceTracker().getLastCommandResult(c.getDeviceSerial());
        if (commandResult != null &&
            commandResult.getCommandResult().getStatus() == Status.EXECUTING) {
            String msg = String.format("Another command is already executing on %s",
                    c.getDeviceSerial());
            CLog.e(msg);
            result.put(RemoteOperation.ERROR, msg);
            return;
        }
        CLog.logAndDisplay(LogLevel.INFO, "Executing command '%s'", ArrayUtil.join(" ",
                (Object[])c.getCommandArgs()));
        try {
            ExecCommandTracker tracker = new ExecCommandTracker();
            mScheduler.execCommand(tracker, device, c.getCommandArgs());
            getDeviceTracker().setCommandTracker(c.getDeviceSerial(), tracker);
        } catch (ConfigurationException e) {
            CLog.e("Failed to exec command");
            CLog.e(e);
            result.put(RemoteOperation.ERROR, "Config error: " + e.toString());
        }
    }

    private void processGetLastCommandResult(GetLastCommandResultOp c, JSONObject json)
            throws JSONException {
        ITestDevice device = getDeviceTracker().getDeviceForSerial(c.getDeviceSerial());
        ExecCommandTracker tracker = getDeviceTracker().getLastCommandResult(c.getDeviceSerial());
        if (device == null) {
            c.packResponseIntoJson(new CommandResult(CommandResult.Status.NOT_ALLOCATED), json);
        } else if (tracker == null) {
            c.packResponseIntoJson(new CommandResult(CommandResult.Status.NO_ACTIVE_COMMAND),
                    json);
        } else {
            c.packResponseIntoJson(tracker.getCommandResult(), json);
        }
    }

    private void processClose(CloseOp rc, JSONObject result) {
        cancel();
    }

    private void freeAllDevices() {
        for (ITestDevice d : getDeviceTracker().freeAll()) {
            CLog.logAndDisplay(LogLevel.INFO,
                    "Freeing device %s no longer in use by remote tradefed",
                            d.getSerialNumber());
            mDeviceManager.freeDevice(d, FreeDeviceState.AVAILABLE);
        }
    }

    private void sendAck(JSONObject result, PrintWriter out) {
        out.println(result.toString());
    }

    /**
     * Request to cancel the remote manager.
     */
    public synchronized void cancel() {
        if (!mCancel) {
            mCancel  = true;
            CLog.logAndDisplay(LogLevel.INFO, "Closing remote manager at port %d", getPort());
        }
    }

    /**
     * Convenience method to request a remote manager shutdown and wait for it to complete.
     */
    public void cancelAndWait() {
        cancel();
        try {
            join();
        } catch (InterruptedException e) {
            CLog.e(e);
        }
    }

    private void closeSocket(ServerSocket serverSocket) {
        StreamUtil.close(serverSocket);
    }

    private void closeSocket(Socket clientSocket) {
        StreamUtil.close(clientSocket);
    }

    private void closeReader(BufferedReader in) {
        StreamUtil.close(in);
    }

    private void closeWriter(PrintWriter out) {
        StreamUtil.close(out);
    }

    /**
     * @return <code>true</code> if a cancel has been requested
     */
    public boolean isCanceled() {
        return mCancel;
    }
}
