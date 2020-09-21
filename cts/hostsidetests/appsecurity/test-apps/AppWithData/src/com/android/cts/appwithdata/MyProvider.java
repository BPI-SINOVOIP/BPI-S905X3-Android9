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

package com.android.cts.appwithdata;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.TrafficStats;
import android.net.Uri;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.system.Os;
import android.util.Log;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

public class MyProvider extends ContentProvider {
    private static final String TAG = "CTS";

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        throw new UnsupportedOperationException();
    }

    @Override
    public String getType(Uri uri) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Bundle call(String method, String arg, Bundle extras) {
        Log.v(TAG, "call() " + method);
        switch (method) {
            case "start": {
                server = new EchoServer();
                server.start();
                extras = new Bundle();
                extras.putInt("port", server.server.getLocalPort());
                return extras;
            }
            case "stop": {
                server.halt();
                return null;
            }
            case "tag": {
                final int uid = Os.getuid();
                Log.v(TAG, "My UID is " + uid);
                TrafficStats.setThreadStatsUid(uid);
                if (TrafficStats.getThreadStatsUid() != uid) {
                    throw new AssertionError("TrafficStats UID mismatch!");
                }
                try {
                    final ParcelFileDescriptor pfd = extras.getParcelable("fd");
                    TrafficStats.tagFileDescriptor(pfd.getFileDescriptor());
                } catch (IOException e) {
                    throw new RuntimeException(e);
                } finally {
                    TrafficStats.clearThreadStatsUid();
                }
                return null;
            }
            default: {
                throw new UnsupportedOperationException();
            }
        }
    }

    private EchoServer server;

    private static class EchoServer extends Thread {
        final ServerSocket server;
        volatile boolean halted = false;

        public EchoServer() {
            try {
                server = new ServerSocket(0);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }

        public void halt() {
            halted = true;
            try {
                server.close();
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }

        @Override
        public void run() {
            while (!halted) {
                try {
                    final Socket socket = server.accept();
                    socket.setTcpNoDelay(true);
                    final int val = socket.getInputStream().read();
                    socket.getOutputStream().write(val);
                    socket.getOutputStream().flush();
                    socket.close();
                } catch (IOException e) {
                    Log.w(TAG, e);
                }
            }
        }
    }
}
