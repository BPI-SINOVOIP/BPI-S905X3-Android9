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

import com.android.tradefed.log.LogUtil.CLog;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;

/**
 * Helper class to simulate the device sl4a layer that respond to RPC queries.
 */
public class FakeSocketServerHelper extends Thread {
    private ServerSocket mSocket;
    private boolean mCanceled = false;

    public FakeSocketServerHelper() throws IOException {
        setName(getClass().getCanonicalName());
        mSocket = new ServerSocket(0);
    }

    public int getPort() {
        return mSocket.getLocalPort();
    }

    public void close() throws IOException {
        mCanceled = true;
        if (mSocket != null) {
            mSocket.close();
        }
    }

    @Override
    public void run() {
        Socket client = null;
        BufferedReader in = null;
        PrintWriter out = null;
        try {
            client = mSocket.accept();
            in = new BufferedReader(
                    new InputStreamReader(client.getInputStream()));
            out = new PrintWriter(client.getOutputStream(), true);
        } catch (IOException e1) {
            return;
        }
        while (!mCanceled) {
            try {
                String response = in.readLine();
                if (response != null) {
                    if (response.contains("initiate")) {
                        out.print("{\"uid\":1,\"status\":true}");
                        out.print('\n');
                        out.flush();
                    } else if (response.contains("getBoolean")) {
                        out.print("{\"id\":2,\"result\":true,\"error\":null}");
                        out.print('\n');
                        out.flush();
                    }
                }
            } catch (IOException e) {
                CLog.e(e);
                mCanceled = true;
            }
        }
    }
}
