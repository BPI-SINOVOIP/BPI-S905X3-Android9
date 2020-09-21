/*
 * Copyright (C) 2014 Square, Inc.
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
package com.squareup.okhttp.internal.http;

import com.squareup.okhttp.DelegatingServerSocketFactory;
import com.squareup.okhttp.DelegatingSocketFactory;
import com.squareup.okhttp.OkHttpClient;
import com.squareup.okhttp.OkUrlFactory;
import com.squareup.okhttp.mockwebserver.MockResponse;
import com.squareup.okhttp.mockwebserver.MockWebServer;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.TimeUnit;

import okio.Buffer;
import org.junit.Before;
import org.junit.Test;

import javax.net.ServerSocketFactory;
import javax.net.SocketFactory;

import static org.junit.Assert.fail;

public final class DisconnectTest {

  // The size of the socket buffers in bytes.
  private static final int SOCKET_BUFFER_SIZE = 256 * 1024;

  private MockWebServer server;
  private OkHttpClient client;

  @Before public void setUp() throws Exception {
    server = new MockWebServer();
    client = new OkHttpClient();

    // Sockets on some platforms can have large buffers that mean writes do not block when
    // required. These socket factories explicitly set the buffer sizes on sockets created.
    server.setServerSocketFactory(
        new DelegatingServerSocketFactory(ServerSocketFactory.getDefault()) {
          @Override
          protected ServerSocket configureServerSocket(ServerSocket serverSocket)
              throws IOException {
            serverSocket.setReceiveBufferSize(SOCKET_BUFFER_SIZE);
            return serverSocket;
          }
        });
    client.setSocketFactory(new DelegatingSocketFactory(SocketFactory.getDefault()) {
      @Override
      protected Socket configureSocket(Socket socket) throws IOException {
        socket.setSendBufferSize(SOCKET_BUFFER_SIZE);
        socket.setReceiveBufferSize(SOCKET_BUFFER_SIZE);
        return socket;
      }
    });
  }

  @Test public void interruptWritingRequestBody() throws Exception {
    int requestBodySize = 2 * 1024 * 1024; // 2 MiB

    server.enqueue(new MockResponse()
        .throttleBody(64 * 1024, 125, TimeUnit.MILLISECONDS)); // 500 Kbps
    server.start();

    HttpURLConnection connection = new OkUrlFactory(client).open(server.getUrl("/"));
    disconnectLater(connection, 500);

    connection.setDoOutput(true);
    connection.setFixedLengthStreamingMode(requestBodySize);
    OutputStream requestBody = connection.getOutputStream();
    byte[] buffer = new byte[1024];
    try {
      for (int i = 0; i < requestBodySize; i += buffer.length) {
        requestBody.write(buffer);
        requestBody.flush();
      }
      fail("Expected connection to be closed");
    } catch (IOException expected) {
    }

    connection.disconnect();
  }

  @Test public void interruptReadingResponseBody() throws Exception {
    int responseBodySize = 2 * 1024 * 1024; // 2 MiB

    server.enqueue(new MockResponse()
        .setBody(new Buffer().write(new byte[responseBodySize]))
        .throttleBody(64 * 1024, 125, TimeUnit.MILLISECONDS)); // 500 Kbps
    server.start();

    HttpURLConnection connection = new OkUrlFactory(client).open(server.getUrl("/"));
    disconnectLater(connection, 500);

    InputStream responseBody = connection.getInputStream();
    byte[] buffer = new byte[1024];
    try {
      while (responseBody.read(buffer) != -1) {
      }
      fail("Expected connection to be closed");
    } catch (IOException expected) {
    }

    connection.disconnect();
  }

  private void disconnectLater(final HttpURLConnection connection, final int delayMillis) {
    Thread interruptingCow = new Thread() {
      @Override public void run() {
        try {
          sleep(delayMillis);
          connection.disconnect();
        } catch (InterruptedException e) {
          throw new RuntimeException(e);
        }
      }
    };
    interruptingCow.start();
  }
}
