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
package com.android.tradefed.util.sl4a;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.HashMap;
import java.util.Map;

/**
 * Sl4A client to interact via RPC with SL4A scripting layer.
 */
public class Sl4aClient implements AutoCloseable {

    private static final String INIT = "initiate";
    public static final String IS_SL4A_RUNNING_CMD =
            "ps -e | grep \"S com.googlecode.android_scripting\"";
    public static final String IS_SL4A_RUNNING_CMD_OLD =
            "ps | grep \"S com.googlecode.android_scripting\"";
    public static final String SL4A_LAUNCH_CMD =
            "am start -a com.googlecode.android_scripting.action.LAUNCH_SERVER " +
            "--ei com.googlecode.android_scripting.extra.USE_SERVICE_PORT %s " +
            "com.googlecode.android_scripting/.activity.ScriptingLayerServiceLauncher";
    public static final String STOP_SL4A_CMD = "am force-stop com.googlecode.android_scripting";

    private static final int UNKNOWN_ID = -1;

    private ITestDevice mDevice;
    private int mHostPort;
    private int mDeviceSidePort;
    private Socket mSocket;
    private Long mCounter = 1L;
    private int mUid = UNKNOWN_ID;

    private Sl4aEventDispatcher mEventDispatcher;

    /**
     * Creates the Sl4A client.
     *
     * @param device the {ITestDevice} that the client will be for.
     * @param hostPort the port on the host machine to connect to the sl4a client.
     * @param devicePort the device port used to communicate to.
     */
    public Sl4aClient(ITestDevice device, int hostPort, int devicePort) {
        mDevice = device;
        mHostPort = hostPort;
        mDeviceSidePort = devicePort;
    }

    /**
     * Creates the Sl4A client.
     *
     * @param device the {ITestDevice} that the client will be for.
     * @param sl4aApkFile file path to hte sl4a apk to install, or null if already installed.
     */
    public Sl4aClient(ITestDevice device, File sl4aApkFile) throws DeviceNotAvailableException {
        installSl4a(device, sl4aApkFile);
        ServerSocket s = null;
        int port = -1;
        try {
            s = new ServerSocket(0);
            s.setReuseAddress(true);
            port = s.getLocalPort();
            s.close();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        mDevice = device;
        mHostPort = port;
        mDeviceSidePort = 9998;
    }

    private static void installSl4a(ITestDevice device, File sl4aApkFile)
            throws DeviceNotAvailableException {
        if (sl4aApkFile != null) {
            if (!sl4aApkFile.exists()) {
                throw new RuntimeException(String.format("Sl4A apk '%s' was not found.",
                        sl4aApkFile.getAbsoluteFile()));
            }
            String res = device.installPackage(sl4aApkFile, true);
            if (res != null) {
                throw new RuntimeException(String.format("Error when installing the Sl4A apk: %s",
                        res));
            }
        }
    }

    /**
     * Convenience method to create and start a client ready to use.
     *
     * @param device the {ITestDevice} that the client will be for.
     * @param sl4aApkFile file path to hte sl4a apk to install, or null if already installed.
     * @return an {@link Sl4aClient} instance that has been started.
     * @throws DeviceNotAvailableException
     */
    public static Sl4aClient startSL4A(ITestDevice device, File sl4aApkFile)
            throws DeviceNotAvailableException {
        installSl4a(device, sl4aApkFile);
        ServerSocket s = null;
        int port = -1;
        try {
            s = new ServerSocket(0);
            s.setReuseAddress(true);
            port = s.getLocalPort();
            s.close();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        // even after being closed, socket may remain in TIME_WAIT state
        // reuse address allows to connect to it even in this state.
        Sl4aClient sl4aClient = new Sl4aClient(device, port, 9998);
        sl4aClient.startSl4A();
        return sl4aClient;
    }

    /**
     * Return the default runutil instance. Exposed for testing.
     */
    protected IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    /**
     * Starts the sl4a client on the device side.
     * Assume the sl4a apk is installed.
     */
    public void startSl4A() throws DeviceNotAvailableException {
        mDevice.executeShellCommand(String.format(SL4A_LAUNCH_CMD, mDeviceSidePort));
        // Allow some times for the process to start.
        getRunUtil().sleep(2000);
        if (isSl4ARunning() == false) {
            throw new RuntimeException("sl4a is not running.");
        }
        open();
    }

    /**
     * Return true if the sl4a device side client is running.
     */
    public boolean isSl4ARunning() throws DeviceNotAvailableException {
        // Grep for process with a preceding S which means it is truly started.
        // Some devices running older version do not support ps -e command, use ps instead
        // Right now there is no easy way to find out which system support -e option
        String out1 = mDevice.executeShellCommand(IS_SL4A_RUNNING_CMD_OLD);
        String out2 = mDevice.executeShellCommand(IS_SL4A_RUNNING_CMD);
        if (out1 == null || out2 == null) {
            CLog.i("Null string return");
            return false;
        } else if (out1.trim().isEmpty() && out2.trim().isEmpty()) {
            CLog.i("Empty return");
            return false;
        } else {
            return true;
        }
    }

    /** Helper to actually starts the connection host to device for sl4a. */
    public void open() {
        try {
            mDevice.executeAdbCommand("forward", "tcp:" + mHostPort, "tcp:" + mDeviceSidePort);
            String res = mDevice.executeAdbCommand("forward", "--list");
            CLog.d("forwardings: %s", res);
            mSocket = new Socket("localhost", mHostPort);
            CLog.i("is sl4a socket connected: %s", mSocket.isConnected());
            String rep = sendCommand(Sl4aClient.INIT);
            CLog.i("response sl4a INIT: %s", rep);
            JSONObject init = new JSONObject(rep);
            mUid = init.getInt("uid");
            startEventDispatcher();
        } catch (IOException | DeviceNotAvailableException | JSONException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Starts the event dispatcher. Exposed for testing.
     */
    protected void startEventDispatcher() throws DeviceNotAvailableException {
        if (isSl4ARunning() == true) {
            mEventDispatcher = new Sl4aEventDispatcher(this, 5000);
            mEventDispatcher.start();
        } else {
            throw new RuntimeException("sl4a is not running.");
        }
    }

    /**
     * Helper for initial handshake with SL4A client device side.
     */
    private String sendCommand(String cmd) throws IOException {
        Map<String, String> info = new HashMap<>();
        info.put("cmd", cmd);
        info.put("uid", mUid +"");
        JSONObject message = new JSONObject(info);
        PrintWriter out = new PrintWriter(mSocket.getOutputStream(), true);
        out.print(message.toString());
        out.print('\n');
        CLog.i("flushing");
        out.flush();
        CLog.i("sent");
        BufferedReader in = new BufferedReader(new InputStreamReader(mSocket.getInputStream()));
        CLog.i("reading");
        String response = in.readLine();
        return response;
    }

    /**
     * Helper to send a message through the sl4a socket.
     *
     * @param message the JSON object to be sent through the socket.
     * @return the response of the request.
     * @throws IOException
     */
    private synchronized Object sendThroughSocket(String message) throws IOException {
        CLog.d("preparing sending: '%s'", message.toString());
        PrintWriter out = new PrintWriter(mSocket.getOutputStream(), false);
        out.print(message.toString());
        out.print('\n');
        out.flush();
        BufferedReader in = new BufferedReader(new InputStreamReader(mSocket.getInputStream()));
        String response = in.readLine();
        CLog.d("response: '%s'", response);
        try {
            JSONObject resp = new JSONObject(response);
            if (!resp.isNull("error")) {
                throw new IOException(String.format("RPC error: %s", resp.get("error")));
            }
            if (resp.isNull("result")) {
                return null;
            }
            // TODO: verify id is matching
            return resp.get("result");
        } catch (JSONException e) {
            throw new IOException(e);
        }
    }

    /**
     * Close the sl4a connection to device side and Kills any running instance of sl4a.
     * If no instance is running then nothing is done.
     */
    @Override
    public void close() {
        try {
            if (mEventDispatcher != null) {
                mEventDispatcher.cancel();
            }
            if (mSocket != null) {
                mSocket.close();
            }
            mDevice.executeShellCommand(STOP_SL4A_CMD);
            mDevice.executeAdbCommand("forward", "--remove", "tcp:" + mHostPort);
        } catch (IOException | DeviceNotAvailableException e) {
            CLog.e(e);
        }
    }

    /**
     * Execute an RPC call on the sl4a layer.
     *
     * @param methodName the name of the method to be called on device side.
     * @param args the arg list to be used on the method.
     * @return the result of the request.
     * @throws IOException if the requested method does not exists.
     */
    public Object rpcCall(String methodName, Object... args) throws IOException {
        JSONArray argsFormatted = new JSONArray();
        if (args != null) {
            for (Object arg : args) {
                argsFormatted.put(arg);
            }
        }
        JSONObject message = new JSONObject();
        try {
            message.put("id", mCounter);
            message.put("method", methodName);
            message.put("params", argsFormatted);
        } catch (JSONException e) {
            CLog.e(e);
            throw new IOException("Failed to format the message", e);
        }
        mCounter++;
        return sendThroughSocket(message.toString());
    }

    /**
     * Return the event dispatcher to wait for events.
     */
    public Sl4aEventDispatcher getEventDispatcher() {
        return mEventDispatcher;
    }
}
