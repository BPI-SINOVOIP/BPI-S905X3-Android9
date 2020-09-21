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

package org.apache.harmony.jpda.tests.jdwp.Events;

import java.io.IOException;
import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.Event;
import org.apache.harmony.jpda.tests.framework.jdwp.EventPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent.Event_CLASS_PREPARE;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP unit test for CLASS_PREPARE events sent from the debugger thread.
 */
public class ClassPrepare002Test extends JDWPEventTestCase {
    @Override
    protected String getDebuggeeClassName() {
        return ClassPrepare002Debuggee.class.getName();
    }

    /**
     * Tests that the CLASS_PREPARE event can be sent from the runtime's debugger thread.
     *
     * This test starts by requesting a CLASS_PREPARE event then sends a ReferenceType.GetValues
     * command to cause the debugger thread to report a CLASS_PREPARE event. After checking the
     * content of the event packet, we clear the event request then resume the debuggee.
     */
    public void testClassPrepareCausedByDebugger() {
        logWriter.println("testClassPrepareCausedByDebugger started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // The TestClassA must be loaded but not initialized.
        long classIDOfA = getClassIDBySignature(getClassSignature(TestClassA.class));

        // Restrict class prepare only to the tested class.
        String classPrepareClassName = "org.apache.harmony.jpda.tests.jdwp.Events.TestClassB";

        // Request CLASS_PREPARE event. The event will be caused by the debugger thread when
        // handling the ReferenceType.GetValues command we're going to send just after.
        final byte eventKind = JDWPConstants.EventKind.CLASS_PREPARE;
        final byte suspendPolicy = JDWPConstants.SuspendPolicy.EVENT_THREAD;
        Event event = Event.builder(eventKind, suspendPolicy)
                .setClassMatch(classPrepareClassName)
                .build();
        ReplyPacket eventReply = debuggeeWrapper.vmMirror.setEvent(event);
        checkReplyPacket(eventReply, "Failed to set CLASS_PREPARE event");
        int classPrepareRequestId = eventReply.getNextValueAsInt();
        assertAllDataRead(eventReply);

        // Send a ReferenceType.GetValues command to force class initialization. This will trigger
        // the CLASS_PREPARE event from the debugger thread.
        logWriter.println("=> CHECK: send ReferenceType.GetValues");
        long fieldId = checkField(classIDOfA, "field");
        CommandPacket getValuesCommand = new CommandPacket(
                JDWPCommands.ReferenceTypeCommandSet.CommandSetID,
                JDWPCommands.ReferenceTypeCommandSet.GetValuesCommand);
        getValuesCommand.setNextValueAsReferenceTypeID(classIDOfA);
        getValuesCommand.setNextValueAsInt(1);
        getValuesCommand.setNextValueAsFieldID(fieldId);

        // Send command but does not wait for reply because this will cause a CLASS_PREPARE event
        // that will suspend every thread.
        int rtGetValuesCommandId = -1;
        try {
            rtGetValuesCommandId = debuggeeWrapper.vmMirror.sendCommand(getValuesCommand);
        } catch (IOException e) {
            fail("Failed to send ReferenceType.GetValues command");
        }

        // Wait for the class prepare event.
        EventPacket eventPacket = debuggeeWrapper.vmMirror
                .receiveCertainEvent(JDWPConstants.EventKind.CLASS_PREPARE);
        ParsedEvent[] parsedEvents = ParsedEvent.parseEventPacket(eventPacket);
        assertNotNull(parsedEvents);
        logWriter.println("Received " + parsedEvents.length + " event(s)");
        assertEquals(1, parsedEvents.length);

        // Check event is the expected one.
        ParsedEvent parsedEvent = parsedEvents[0];
        // Only expect CLASS_PREPARE event.
        assertEquals(JDWPConstants.EventKind.CLASS_PREPARE, parsedEvent.getEventKind());
        // CLASS_PREPARE events caused by the debugger must have an ALL
        // suspend policy.
        assertEquals(JDWPConstants.SuspendPolicy.ALL, parsedEvent.getSuspendPolicy());
        // Check that its the event that we have requested.
        assertEquals(classPrepareRequestId, parsedEvent.getRequestID());
        Event_CLASS_PREPARE classPrepareEvent = (Event_CLASS_PREPARE) parsedEvent;
        // The JDWP spec says that if the event was caused by the debugger thread, the thread ID
        // must be 0.
        assertEquals(0, classPrepareEvent.getThreadID());
        // Check that it is the expected class.
        assertEquals(getClassSignature(classPrepareClassName), classPrepareEvent.getSignature());

        // Check that we received the reply of the command that caused the CLASS_PREPARE event.
        logWriter.println("=> CHECK: receive ReferenceType.GetValues reply (" +
                rtGetValuesCommandId + ")");
        ReplyPacket getValuesReply = null;
        try {
            getValuesReply = debuggeeWrapper.vmMirror.receiveReply(rtGetValuesCommandId);
        } catch (InterruptedException | IOException e) {
            fail("Failed to receive ReferenceType.GetValues reply");
        }
        checkReplyPacket(getValuesReply, "Failed to receive ReferenceType.GetValues reply");

        // Clear the CLASS_PREPARE event request.
        debuggeeWrapper.vmMirror.clearEvent(JDWPConstants.EventKind.CLASS_PREPARE,
                classPrepareRequestId);

        // Resume all threads suspended by the CLASS_PREPARE event.
        debuggeeWrapper.vmMirror.resume();

        // Let the debuggee finish.
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        logWriter.println("testClassPrepareCausedByDebugger finished");
    }
}
