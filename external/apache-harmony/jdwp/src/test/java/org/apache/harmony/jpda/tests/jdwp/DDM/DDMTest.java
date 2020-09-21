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

package org.apache.harmony.jpda.tests.jdwp.DDM;

import org.apache.harmony.jpda.tests.framework.TestOptions;
import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.TaggedObject;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

import java.util.Arrays;
import java.util.Random;

/**
 * JDWP unit test for DDM.Chunk command.
 */
public class DDMTest extends JDWPSyncTestCase {
    public static final int REPS = 4;

    /**
     * JDWP DDM Command Set constants
     */
    public static class DDMCommandSet {
        public static final byte CommandSetID = -57; // uint8_t value is 199

        public static final byte ChunkCommand = 1;
    }

    /**
     * Returns full name of debuggee class which is used by this test.
     * @return full name of debuggee class.
     */
    @Override
    protected String getDebuggeeClassName() {
        return "org.apache.harmony.jpda.tests.jdwp.DDM.DDMDebuggee";
    }

    @Override
    protected void beforeConnectionSetUp() {
        settings.setAttachConnectorKind();
        if (settings.getTransportAddress() == null) {
            settings.setTransportAddress(TestOptions.DEFAULT_ATTACHING_ADDRESS);
        }
        logWriter.println("ATTACH connector kind");
        super.beforeConnectionSetUp();
    }

    private CommandPacket makeCommand(int type, byte[] test_values) {
        CommandPacket packet = new CommandPacket(
                DDMCommandSet.CommandSetID,
                DDMCommandSet.ChunkCommand);
        packet.setNextValueAsInt(type);
        packet.setNextValueAsInt(test_values.length);
        for (byte b : test_values) {
          packet.setNextValueAsByte(b);
        }
        return packet;
    }

    private CommandPacket makeCommand(byte[] test_values) {
        return makeCommand(DDMDebuggee.DDM_TEST_TYPE, test_values);
    }

    /**
     * This testcase exercises DDM.Chunk command.
     * <BR>Starts <A HREF="./DDMDebuggee.html">DDMDebuggee</A> debuggee.
     * <BR>Creates new instance of array by DDM.Chunk command,
     * check it length by ArrayReference.Length command.
     */
    public void testChunk001() {
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        Random r = new Random();
        // Send a few messages to make sure that everything works correctly and the connected
        // message gets sent exactly once.
        byte[] test_values = new byte[128];
        for (int rep = 0; rep < REPS; rep++) {
          r.nextBytes(test_values);
          CommandPacket packet = makeCommand(test_values);
          logWriter.println("Send Chunk message with handler");

          byte[] expected = DDMDebuggee.calculateExpectedResult(test_values);
          ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
          checkReplyPacket(reply, "DDM::Chunk command");
          int type = reply.getNextValueAsInt();
          assertEquals("DDM::Chunk returned unexpected type", DDMDebuggee.DDM_RESULT_TYPE, type);
          int len = reply.getNextValueAsInt();
          assertEquals("DDM::Chunk returned unexpected amount of data", expected.length, len);
          byte[] res = new byte[len];
          for (int i = 0; i < len; i++) {
            res[i] = reply.getNextValueAsByte();
          }
          if (!Arrays.equals(expected, res)) {
            fail("Unexpected different value: expected " + Arrays.toString(expected) + " got " +
                Arrays.toString(res));
          }
          assertAllDataRead(reply);
        }
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        logWriter.println("Send same message without handler");
        CommandPacket packet = makeCommand(test_values);
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "DDM::Chunk command");
        assertAllDataRead(reply);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Wait for the debuggee to re-register the handler
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Actually disconnect.
        logWriter.println("");
        logWriter.println("=> CLOSE CONNECTION");
        closeConnection();
        logWriter.println("=> CONNECTION CLOSED");

        logWriter.println("Connecting and disconnecting " + REPS + " times with calls");
        for (int rep = 0; rep < REPS; rep++) {
          logWriter.println("");
          logWriter.println("=> OPEN NEW CONNECTION");
          openConnection();
          logWriter.println("=> CONNECTION OPENED");

          logWriter.println("Sending DDM.Chunk command with unused type.");
          packet = makeCommand(Integer.MAX_VALUE, test_values);
          reply = debuggeeWrapper.vmMirror.performCommand(packet);
          checkReplyPacket(reply, "DDM::Chunk command");
          assertAllDataRead(reply);

          logWriter.println("");
          logWriter.println("=> CLOSE CONNECTION");
          closeConnection();
          logWriter.println("=> CONNECTION CLOSED");
        }
        logWriter.println("Connecting and disconnecting " + REPS + " times without calls");
        for (int rep = 0; rep < REPS; rep++) {
          logWriter.println("");
          logWriter.println("=> OPEN NEW CONNECTION");
          openConnection();
          logWriter.println("=> CONNECTION OPENED");

          logWriter.println("Doing nothing!");

          logWriter.println("");
          logWriter.println("=> CLOSE CONNECTION");
          closeConnection();
          logWriter.println("=> CONNECTION CLOSED");
        }
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // Get the message saying how many times we got a disconnect/connect.
        int active_cnt = Integer.parseInt(synchronizer.receiveMessage());
        assertEquals("Connected got called an unexpected number of times",
            REPS + 1, active_cnt);
        int deactive_cnt = Integer.parseInt(synchronizer.receiveMessage());
        assertEquals("Disconnected got called an unexpected number of times",
            REPS + 1, deactive_cnt);
    }
}
