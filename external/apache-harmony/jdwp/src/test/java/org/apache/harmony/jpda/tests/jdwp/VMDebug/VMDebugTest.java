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

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPTestConstants;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP Unit test for VMDebug functions command
 */
public class VMDebugTest extends JDWPSyncTestCase {
    // We slept for 20ms so we should have at least 10ms delta. We don't want to be too exact
    // because timers are annoying.
    public static final long EXPECTED_WAIT_TIME = 10;

    @Override
    protected String getDebuggeeClassName() {
        return "org.apache.harmony.jpda.tests.jdwp.VMDebug.VMDebugDebuggee";
    }

    private void sendDebuggerActivity() {
        logWriter.println("Sending invalid command to ensure there is recent debugger activity!");
        long stringID = JDWPTestConstants.INVALID_OBJECT_ID;
        int expectedError = JDWPConstants.Error.INVALID_OBJECT;
        logWriter.println("Send StringReference.Value command with id " + stringID);

        CommandPacket packet = new CommandPacket(
                JDWPCommands.StringReferenceCommandSet.CommandSetID,
                JDWPCommands.StringReferenceCommandSet.ValueCommand);
        packet.setNextValueAsStringID(stringID);
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);

        checkReplyPacket(reply, "StringReference::Value command", expectedError);
    }

    public void testVMDebug() {
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        // Do something that resets the debugger activity count.
        sendDebuggerActivity();
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
        // Get the results.
        VMDebugDebuggee.DebugResult res = VMDebugDebuggee.readResult(synchronizer.receiveMessage());
        if (res == null) {
            fail("unable to deserialize result data");
        } else {
            logWriter.println("Received results: " + res);
            assertFalse("no error expected", res.error_occured);
            assertTrue("expected active debugger!", res.is_debugging_enabled);
            assertTrue("expected active debugger connection!", res.is_debugger_connected);
            if (EXPECTED_WAIT_TIME > res.last_debugger_activity) {
                fail("Expected last debugger activity to be greater than 10, was " + res.last_debugger_activity);
            }
        }
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }
}
