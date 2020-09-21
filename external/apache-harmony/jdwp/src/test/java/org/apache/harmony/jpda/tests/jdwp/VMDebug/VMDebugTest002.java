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

import org.apache.harmony.jpda.tests.framework.TestOptions;
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
public class VMDebugTest002 extends JDWPSyncTestCase {
    @Override
    protected String getDebuggeeClassName() {
        return "org.apache.harmony.jpda.tests.jdwp.VMDebug.VMDebugDebuggee";
    }

    // We will need to disconnect once to clear out the VM_INIT resume that we get on startup.
    @Override
    protected void beforeConnectionSetUp() {
        settings.setAttachConnectorKind();
        if (settings.getTransportAddress() == null) {
            settings.setTransportAddress(TestOptions.DEFAULT_ATTACHING_ADDRESS);
        }
        logWriter.println("ATTACH connector kind");
        super.beforeConnectionSetUp();
    }

    /**
     * JDWP DDM Command Set constants
     *
     * TODO It would be good to get these out of the DDMTest class but that class won't be there in
     * all configurations.
     */
    public static class DDMCommandSet {
        public static final byte CommandSetID = -57; // uint8_t value is 199

        public static final byte ChunkCommand = 1;
    }

    private void sendDdmDebuggerActivity() {
        int type = 0xABCDEF01;
        logWriter.println("Send DDM.Chunk command with type " + type + " and no data");
        CommandPacket packet = new CommandPacket(
                DDMCommandSet.CommandSetID,
                DDMCommandSet.ChunkCommand);
        packet.setNextValueAsInt(type);
        packet.setNextValueAsInt(0x0);
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);

        checkReplyPacket(reply, "DDM::Chunk command");
        // Shouldn't have any data.
        assertAllDataRead(reply);
    }

    public void testVMDebug() {
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        // Close and reopen connection to clear out any activity from VM_INIT or other places.
        logWriter.println("=> CLOSE CONNECTION");
        closeConnection();
        logWriter.println("=> CONNECTION CLOSED");
        logWriter.println("=> OPEN NEW CONNECTION");
        // Don't use the openConnection helper since that will call JDWP functions to figure out the
        // type-lengths.
        openConnectionWithoutTypeLength();
        logWriter.println("=> CONNECTION OPENED");
        // Do something that resets the debugger activity count.
        sendDdmDebuggerActivity();
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
        // Get the results.
        VMDebugDebuggee.DebugResult res = VMDebugDebuggee.readResult(synchronizer.receiveMessage());
        if (res == null) {
            fail("unable to deserialize result data");
        } else {
            logWriter.println("Received results: " + res);
            assertFalse("no error expected", res.error_occured);
            assertTrue("expected debugger to be enabled!", res.is_debugging_enabled);
            assertFalse(
                "expected no active debugger connection since no non-ddms commands were sent!",
                res.is_debugger_connected);
            assertEquals("Expected no last-debugger-activity", -1L, res.last_debugger_activity);
        }
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }
}
