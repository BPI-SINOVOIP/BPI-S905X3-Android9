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

package com.googlecode.android_scripting.event;

import com.googlecode.android_scripting.Log;
import com.googlecode.android_scripting.SimpleServer;
import com.googlecode.android_scripting.jsonrpc.JsonBuilder;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.Vector;
import java.util.concurrent.CountDownLatch;

import org.json.JSONException;

/**
 * An Event Forwarding server that forwards events from the rpc queue in realtime to listener
 * clients.
 *
 */
public class EventServer extends SimpleServer implements EventObserver {
  private static final Vector<Listener> mListeners = new Vector<Listener>();
  private InetSocketAddress address = null;

  public EventServer() {
    this(0);
  }

  public EventServer(int port) {
    address = startAllInterfaces(port);
  }

  public InetSocketAddress getAddress() {
    return address;
  }

  @Override
  public void shutdown() {
    onEventReceived(new Event("sl4a", "{\"shutdown\": \"event-server\"}"));
    for (Listener listener : mListeners) {
      mListeners.remove(listener);
      listener.lock.countDown();
    }
    super.shutdown();
  }

  @Override
  protected void handleConnection(Socket socket) throws IOException {
    Log.d("handle event connection.");
    Listener l = new Listener(socket);
    Log.v("Adding EventServer listener " + socket.getPort());
    mListeners.add(l);
    // we are running in the socket accept thread
    // wait until the event dispatcher gets us the events
    // or we die, what ever happens first
    try {
      l.lock.await();
    } catch (InterruptedException e) {
      e.printStackTrace();
    }
    try {
      l.sock.close();
    } catch (IOException e) {
      e.printStackTrace();
    }
    Log.v("Ending EventServer listener " + socket.getPort());
  }

  @Override
  public void onEventReceived(Event event) {
    Object result = null;
    try {
      result = JsonBuilder.build(event);
    } catch (JSONException e) {
      return;
    }

    Log.v("EventServer dispatching " + result);

    for (Listener listener : mListeners) {
      if (!listener.out.checkError()) {
        listener.out.write(result + "\n");
        listener.out.flush();
      } else {
        // let the socket accept thread we're done
        mListeners.remove(listener);
        listener.lock.countDown();
      }
    }
  }

  private class Listener {
    private Socket sock;
    private PrintWriter out;
    private CountDownLatch lock = new CountDownLatch(1);

    public Listener(Socket l) throws IOException {
      sock = l;
      out = new PrintWriter(l.getOutputStream(), true);
    }
  }

  @Override
  protected void handleRPCConnection(Socket sock, Integer UID, BufferedReader reader, PrintWriter writer)
      throws Exception {
  }
}
