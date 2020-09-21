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

/**
 * @author Anton V. Karnachuk
 */

/**
 * Created on 11.03.2005
 */
package org.apache.harmony.jpda.tests.jdwp.Events;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.ParsedEvent.*;
import org.apache.harmony.jpda.tests.jdwp.Events.EventDebuggee;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;


/**
 * JDWP Unit test for THREAD_START event.
 */
public class ThreadStartTest extends JDWPEventTestCase {

    /**
     * This testcase is for THREAD_START event.
     * <BR>It runs EventDebuggee and verifies that requested
     * THREAD_START event occurs.
     */
    public void testThreadStartEvent001() {
        logWriter.println("==> testThreadStartEvent001 - STARTED...");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        logWriter.println("=> set ThreadStartEvent...");
        ReplyPacket reply =
                debuggeeWrapper.vmMirror.setThreadStart(JDWPConstants.SuspendPolicy.NONE);
        checkReplyPacket(reply, "Set THREAD_START event");

        logWriter.println("=> set ThreadStartEvent - DONE");

        // start the thread
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        // wait for thread start and finish
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // The runtime may start internal threads before starting the debuggee's thread.
        // We loop until receiving a THREAD_START event for our thread.
        boolean receivedExpectedThreadStartEvent = false;
        while (!receivedExpectedThreadStartEvent) {
            logWriter.println("=> vmMirror.receiveEvent()...");
            CommandPacket event = debuggeeWrapper.vmMirror.receiveEvent();

            assertNotNull("Invalid (null) event received", event);
            logWriter.println("=> Event received!");

            ParsedEvent[] parsedEvents = ParsedEvent.parseEventPacket(event);
            logWriter.println("=> Number of events = " + parsedEvents.length);
            assertEquals("Invalid number of events,", 1, parsedEvents.length);
            logWriter.println("=> EventKind() = " + parsedEvents[0].getEventKind()
                    + " (" + JDWPConstants.EventKind.getName(parsedEvents[0].getEventKind()) + ")");
            assertEquals("Invalid event kind,",
                    JDWPConstants.EventKind.THREAD_START,
                    parsedEvents[0].getEventKind(),
                    JDWPConstants.EventKind.getName(JDWPConstants.EventKind.THREAD_START),
                    JDWPConstants.EventKind.getName(parsedEvents[0].getEventKind()));
            logWriter.println("=> EventRequestID() = " + parsedEvents[0].getRequestID());

            long threadID = ((Event_THREAD_START)parsedEvents[0]).getThreadID();
            logWriter.println("=> threadID = " + threadID);
            String threadName = debuggeeWrapper.vmMirror.getThreadName(threadID);
            logWriter.println("=> threadName = " + threadName);

            // Is it the debuggee's thread ?
            receivedExpectedThreadStartEvent = threadName.equals(EventDebuggee.testedThreadName);
        }

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
        logWriter.println("==> testThreadStartEvent001 - OK");
    }
}
