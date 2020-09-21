/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package org.apache.harmony.jpda.tests.jdwp.VMDebug;

import java.lang.reflect.Method;
import java.io.*;
import java.util.*;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

/**
 * This class provides a test for VMDebug functions.
 */
public class VMDebugDebuggee extends SyncDebuggee {
    public static final long SLEEP_TIME = 20;

    public static final class DebugResult implements Serializable {
        public boolean error_occured  = false;
        public boolean is_debugger_connected = false;
        public boolean is_debugging_enabled = false;
        public long last_debugger_activity = -1;

        public DebugResult(boolean is_error, boolean is_debugger_connected, boolean is_debugger_enabled, long last_debugger_activity) {
            this.error_occured = is_error;
            this.is_debugger_connected = is_debugger_connected;
            this.is_debugging_enabled = is_debugger_enabled;
            this.last_debugger_activity = last_debugger_activity;
        }

        @Override
        public String toString() {
            return "DebugResult { error: " + error_occured
                               + ", connected: " + is_debugger_connected
                               + ", enabled: " + is_debugging_enabled
                               + ", last_activity: " + last_debugger_activity + " }";
        }
    }

    private void sendResult(boolean is_error, boolean is_debugger_connected, boolean is_debugger_enabled, long last_debugger_activity) {
        ByteArrayOutputStream bs  = new ByteArrayOutputStream();
        try (ObjectOutputStream st = new ObjectOutputStream(bs)) {
            st.writeObject(new DebugResult(is_error, is_debugger_connected, is_debugger_enabled, last_debugger_activity));
            st.flush();
        } catch (Exception e) {
            logWriter.println("Failed to serialize debug result!");
        }
        synchronizer.sendMessage(Base64.getEncoder().encodeToString(bs.toByteArray()));
    }

    public static DebugResult readResult(String args) throws IllegalArgumentException {
        try (ObjectInputStream ois = new ObjectInputStream(new ByteArrayInputStream(Base64.getDecoder().decode(args)))) {
            return (DebugResult) ois.readObject();
        } catch (Exception e) {
            return null;
        }
    }

    @Override
    public void run() {
        // Tell the test we have started.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        // The debugger will have done something before sending this so the last activity is reset.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        boolean error_occured  = false;
        boolean is_debugger_connected = false;
        boolean is_debugging_enabled = false;
        long last_debugger_activity = -1;
        try {
            // Wait 20 milliseconds so that last_debugger_activity will be non-zero and definitely
            // more than 10.
            Thread.sleep(SLEEP_TIME);

            Class<?> vmdebug = Class.forName("dalvik.system.VMDebug");
            Method isDebuggerConnectedMethod = vmdebug.getDeclaredMethod("isDebuggerConnected");
            Method isDebuggingEnabledMethod = vmdebug.getDeclaredMethod("isDebuggingEnabled");
            Method lastDebuggerActivityMethod = vmdebug.getDeclaredMethod("lastDebuggerActivity");

            is_debugger_connected = (boolean)isDebuggerConnectedMethod.invoke(null);
            is_debugging_enabled = (boolean)isDebuggingEnabledMethod.invoke(null);
            last_debugger_activity = (long)lastDebuggerActivityMethod.invoke(null);
        } catch (NoSuchMethodException e) {
            error_occured = true;
            logWriter.println("Unable to find one of the VMDebug methods!" + e);
        } catch (ClassNotFoundException e) {
            error_occured = true;
            logWriter.println("Could not find VMDebug");
        } catch (Exception e) {
            error_occured = true;
            logWriter.println("Other exception occured " + e);
        }
        sendResult(
            error_occured, is_debugger_connected, is_debugging_enabled, last_debugger_activity);
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    public static void main(String [] args) {
        runDebuggee(VMDebugDebuggee.class);
    }
}

