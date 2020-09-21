/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.tradefed.util.net;

import com.android.tradefed.util.RunUtil;

import junit.framework.TestCase;

import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.TimeUnit;

/**
 * Functional tests for {@link HttpHelper}
 */
public class HttpHelperFuncTest extends TestCase {
    /**
     * Make sure that we get a timeout if the backend doesn't respond
     */
    public void testTimeout() throws Exception {
        final int backendDelay = 500;  // msecs
        final int backendTimeout = 200;
        final int clientTimeout = 250;  // 250ms < 500ms, hence timeout
        final int threadStartTimeout = 100;
        final CyclicBarrier barrier = new CyclicBarrier(2);

        Backend backend = new Backend(backendDelay, backendTimeout, barrier);
        IHttpHelper http = new HttpHelper();
        http.setOpTimeout(clientTimeout);

        // Kick off the backend and wait for it to get up and running
        backend.start();
        barrier.await(threadStartTimeout, TimeUnit.MILLISECONDS);
        // backendTimeout is now ticking down

        final int port = backend.getPort();  // FIXME race condition
        if (port <= 0) {
            backend.join(2 * backendTimeout);
            Throwable e = backend.getException();
            String msg = "Backend was not initialized properly";
            if (e != null) {
                msg = String.format("%s:\n%s", msg, e.toString());
            }
            fail(msg);
            return;
        }

        final String url = String.format("http://localhost:%d/", port);
        try {
            http.doGet(url);
            fail("SocketTimeoutException not thrown");
        } catch (SocketTimeoutException e) {
            // expected
        }

        backend.join(2 * backendTimeout);
    }

    /**
     * Make sure that we _do not_ time out if the backend responds expediently
     */
    public void testNoTimeout() throws Exception {
        final int backendDelay = 0;  // msecs
        final int backendTimeout = 1000;
        final int clientTimeout = 250;  // 250ms > 0ms, hence no timeout
        final int threadStartTimeout = 100;
        final CyclicBarrier barrier = new CyclicBarrier(2);

        Backend backend = new Backend(backendDelay, backendTimeout, barrier);
        IHttpHelper http = new HttpHelper();
        http.setOpTimeout(clientTimeout);

        // Kick off the backend and wait for it to get up and running
        backend.start();
        barrier.await(threadStartTimeout, TimeUnit.MILLISECONDS);
        // backendTimeout is now ticking down

        final int port = backend.getPort();  // FIXME race condition
        if (port <= 0) {
            backend.join(2 * backendTimeout);
            Throwable e = backend.getException();
            String msg = "Backend was not initialized properly";
            if (e != null) {
                msg = String.format("%s:\n%s", msg, e.toString());
            }
            fail(msg);
            return;
        }

        final String url = String.format("http://localhost:%d/", port);
        http.doGet(url);

        backend.join(2 * backendTimeout);
    }

    private class Backend extends Thread {
        public ServerSocket mSS = null;
        public Throwable mException = null;
        public static final String RESPONSE = "HTTP/1.1 200 OK\r\n" +
                "Content-Length: 30\r\n" +
                "Content-Type: text/html\r\n\r\n" +
                "<h1>Hello, this is dog.</h1>\r\n";

        private final int mDelay;
        private final int mTimeout;
        private final CyclicBarrier mBarrier;

        public Backend(int delay, int timeout, CyclicBarrier barrier) {
            super("HttpHelperFuncTest.Backend");
            mDelay = delay;
            mTimeout = timeout;
            mBarrier = barrier;
        }

        public int getPort() {
            if (mSS == null) {
                return -1;
            } else {
                return mSS.getLocalPort();
            }
        }

        public Throwable getException() {
            return mException;
        }

        @Override
        public void run() {
            try {
                mSS = new ServerSocket(0);
                mSS.setSoTimeout(mTimeout);
                mBarrier.await(2 * (mDelay + mTimeout), TimeUnit.MILLISECONDS);

                Socket sock = mSS.accept();
                RunUtil.getDefault().sleep(mDelay);
                OutputStream oStream = sock.getOutputStream();
                oStream.write(RESPONSE.getBytes());
                oStream.flush();
                oStream.close();
                sock.close();
            } catch (Exception e) {
                mException = e;
                mSS = null;
                return;
            }
        }
    }
}
