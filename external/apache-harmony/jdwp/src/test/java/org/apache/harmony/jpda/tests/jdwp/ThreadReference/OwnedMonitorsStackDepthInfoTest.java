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

package org.apache.harmony.jpda.tests.jdwp.ThreadReference;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.TaggedObject;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP Unit test for ThreadReference.OwnedMonitorsStackDepthInfo command.
 */
public class OwnedMonitorsStackDepthInfoTest extends JDWPSyncTestCase {

    static final String thisCommandName = "ThreadReference.OwnedMonitorsStackDepthInfo command ";

    @Override
    protected String getDebuggeeClassName() {
        return "org.apache.harmony.jpda.tests.jdwp.ThreadReference.OwnedMonitorsStackDepthInfoDebuggee";
    }

    private int jdwpGetFrameCount(long threadID) {
      CommandPacket packet = new CommandPacket(JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                                               JDWPCommands.ThreadReferenceCommandSet.FrameCountCommand);
      packet.setNextValueAsThreadID(threadID);

      ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
      checkReplyPacket(reply, "ThreadReference::FrameCount command");
      return reply.getNextValueAsInt();
    }

    /**
     * This testcase exercises ThreadReference.OwnedMonitorsStackDepthInfo
     * command. <BR>
     * At first the test starts OwnedMonitorsStackDepthInfoDebuggee which runs
     * the tested thread 'TESTED_THREAD'. <BR>
     * Then the test performs the ThreadReference.OwnedMonitorsStackDepthInfo
     * command for the tested thread and gets list of monitor objects.
     * The returned monitor objects are equal to expected count and their stack depth are
     *  equal to expected depth. This test will perform MonitorInfo to guarrantee that returend
     *  monitors do belong to the test thread.
     */
    public void testOwnedMonitorsStackDepthInfo() {
        String thisTestName = "testOwnedMonitorsStackDepthInfo";
        logWriter.println("==> " + thisTestName + " for " + thisCommandName + ": START...");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Ensure we signal the debuggee to continue at the end of the test.
        finalSyncMessage = JPDADebuggeeSynchronizer.SGNL_CONTINUE;

        // Getting ID of the tested thread
        logWriter.println("==> testedThreadName = "
                + OwnedMonitorsStackDepthInfoDebuggee.TESTED_THREAD);
        logWriter.println("==> Get testedThreadID...");
        long testedThreadID = debuggeeWrapper.vmMirror
                .getThreadID(OwnedMonitorsStackDepthInfoDebuggee.TESTED_THREAD);

        // Compose the OwnedMonitorsStackDepthInfo command
        CommandPacket stackDepthPacket = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.OwnedMonitorsStackDepthInfoCommand);
        stackDepthPacket.setNextValueAsThreadID(testedThreadID);

        // Suspend the VM before perform command
        logWriter.println("==> testedThreadID = " + testedThreadID);
        logWriter.println("==> suspend testedThread...");
        debuggeeWrapper.vmMirror.suspendThread(testedThreadID);

        // There are various monitors held in the outermost frames, but there may be
        // implementation-specific locks held below that (since the thread will actually be
        // suspended somewhere in I/O code). See OwnedMonitorsStackDepthInfoDebuggee.run().
        int frameCount = jdwpGetFrameCount(testedThreadID);
        int expectedMonitorCount = 3;
        int[] expectedStackDepth = new int[] { frameCount - 4, frameCount - 4, frameCount - 2 };

        // Perform the command and attain the reply package
        ReplyPacket stackDepthReply = debuggeeWrapper.vmMirror.performCommand(stackDepthPacket);
        checkReplyPacket(stackDepthReply, "ThreadReference::OwnedMonitorsStackDepthInfo command");

        // Analyze the reply package
        int actualMonitorCount = stackDepthReply.getNextValueAsInt();
        logWriter.println("==> Owned monitors: " + actualMonitorCount);
        assertTrue(actualMonitorCount >= expectedMonitorCount);
        logWriter.println("==> CHECK: PASSED: actualMonitorCount >= expectedMonitorCount");

        int currentMonitor = 0;
        for (int i = 0; i < actualMonitorCount; ++i) {
            // Attain monitor object ID
            TaggedObject monitorObject = stackDepthReply.getNextValueAsTaggedObject();

            // Attain monitor stack depth
            int actualStackDepth = stackDepthReply.getNextValueAsInt();
            logWriter.println("==> Stack depth: " + actualStackDepth);
            if (expectedStackDepth[currentMonitor] != actualStackDepth) {
                continue;
            }

            /*
             *  Test the returned monitor object does belong to the test thread by MonitorInfo Command
             */
            // Compose the MonitorInfo Command
            CommandPacket monitorInfoPacket = new CommandPacket(
                    JDWPCommands.ObjectReferenceCommandSet.CommandSetID,
                    JDWPCommands.ObjectReferenceCommandSet.MonitorInfoCommand);
            monitorInfoPacket.setNextValueAsObjectID(monitorObject.objectID);

            // Perform the command and attain the reply package
            ReplyPacket monitorInfoReply = debuggeeWrapper.vmMirror.performCommand(monitorInfoPacket);
            checkReplyPacket(monitorInfoReply, "ObjectReference::MonitorInfo command");

            // Attain thread id from monitor info
            long ownerThreadID = monitorInfoReply.getNextValueAsThreadID();
            assertEquals(thisCommandName + "returned monitor is not owned by test thread", ownerThreadID, testedThreadID, null, null);

            logWriter.println("==> CHECK: PASSED: returned monitor does belong to the test thread.");
            logWriter.println("==> Monitor owner thread ID: " + ownerThreadID);

            ++currentMonitor;
        }

        assertAllDataRead(stackDepthReply);
    }


    public void testOwnedMonitorsStackDepthInfo_Unsuspended() {
        String thisTestName = "testOwnedMonitorsStackDepthInfo";
        logWriter.println("==> " + thisTestName + " for " + thisCommandName
                + ": START...");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Ensure we signal the debuggee to continue at the end of the test.
        finalSyncMessage = JPDADebuggeeSynchronizer.SGNL_CONTINUE;

        // Getting ID of the tested thread
        logWriter.println("==> testedThreadName = "
                + OwnedMonitorsStackDepthInfoDebuggee.TESTED_THREAD);
        logWriter.println("==> Get testedThreadID...");
        long testedThreadID = debuggeeWrapper.vmMirror
                .getThreadID(OwnedMonitorsStackDepthInfoDebuggee.TESTED_THREAD);

        // Compose the OwnedMonitorsStackDepthInfo command
        CommandPacket stackDepthPacket = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.OwnedMonitorsStackDepthInfoCommand);
        stackDepthPacket.setNextValueAsThreadID(testedThreadID);

        // Perform the command and attain the reply package
        ReplyPacket checkedReply = debuggeeWrapper.vmMirror
                .performCommand(stackDepthPacket);
        short errorCode = checkedReply.getErrorCode();
        if (errorCode != JDWPConstants.Error.NONE) {
            if (errorCode == JDWPConstants.Error.THREAD_NOT_SUSPENDED) {
                logWriter.println("=> CHECK PASSED: Expected error (THREAD_NOT_SUSPENDED) is returned");
                return;
            }
        }
        printErrorAndFail(thisCommandName + " should throw exception when VM is not suspended.");
    }
}
