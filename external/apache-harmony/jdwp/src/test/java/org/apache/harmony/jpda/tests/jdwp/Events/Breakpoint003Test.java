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

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP Unit test for BREAKPOINT event in framework code.
 */
public class Breakpoint003Test extends JDWPEventTestCase {
    @Override
    protected String getDebuggeeClassName() {
        return Breakpoint003Debuggee.class.getName();
    }

    public void testBreakPointInFrameworkCode() {
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        int breakpoint1 = setBreakPoint("Ljava/lang/Integer;", "parseInt");
        int breakpoint2 = setBreakPoint("Ljava/lang/Long;", "toString");
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        test("testBreakPointInIntegerParseInt",
             "parseInt",
             breakpoint1,
             JDWPConstants.Tag.STRING_TAG,
             Breakpoint003Debuggee.VALUE_STRING);

        resumeDebuggee();

        test("testBreakPointInLongToString",
             "toString",
             breakpoint2,
             JDWPConstants.Tag.LONG_TAG,
             Long.valueOf(Breakpoint003Debuggee.VALUE_LONG));
    }

    private int setBreakPoint(String className, String methodName) {
        long classID = debuggeeWrapper.vmMirror.getClassID(className);
        assertTrue("Failed to find " + className + " class", classID != -1);
        int breakpointReqID = debuggeeWrapper.vmMirror.setBreakpointAtMethodBegin(
                classID, methodName);
        assertTrue("Failed to install breakpoint in " + className + "." + methodName,
                   breakpointReqID != -1);
        return breakpointReqID;
    }

    private void test(String testName,
                      String methodName,
                      int breakpointReqID,
                      byte tag,
                      Object expectedValue) {
        logWriter.println(testName + " started");

        long eventThreadID = debuggeeWrapper.vmMirror.waitForBreakpoint(breakpointReqID);
        checkThreadState(eventThreadID, JDWPConstants.ThreadStatus.RUNNING,
                JDWPConstants.SuspendStatus.SUSPEND_STATUS_SUSPENDED);

        // Get and check top frame's method name.
        CommandPacket packet = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.FramesCommand);
        packet.setNextValueAsThreadID(eventThreadID);
        packet.setNextValueAsInt(0);  // start from frame 0
        packet.setNextValueAsInt(1);  // length of frames
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "ThreadReference::FramesCommand command");
        int frames = reply.getNextValueAsInt();
        assertEquals("Invalid number of frames", frames, 1);
        long frameID = reply.getNextValueAsLong();
        Location location = reply.getNextValueAsLocation();
        String receivedMethodName = getMethodName(location.classID, location.methodID);
        assertEquals("Invalid method name", receivedMethodName, methodName);

        // Get and check top frame's incoming argument.
        packet = new CommandPacket(
                JDWPCommands.StackFrameCommandSet.CommandSetID,
                JDWPCommands.StackFrameCommandSet.GetValuesCommand);
        packet.setNextValueAsThreadID(eventThreadID);
        packet.setNextValueAsFrameID(frameID);
        packet.setNextValueAsInt(1);  // Get 1 value.
        packet.setNextValueAsInt(0);  // Slot 0.
        packet.setNextValueAsByte(tag);
        //check reply for errors
        reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "StackFrame::GetValues command");
        //check number of retrieved values
        int numberOfValues = reply.getNextValueAsInt();
        assertEquals("Invalid number of values", numberOfValues, 1);
        Value val = reply.getNextValueAsValue();
        assertTagEquals("Invalid value tag", val.getTag(), tag);
        long longVal = val.getLongValue();
        if (tag == JDWPConstants.Tag.STRING_TAG) {
            String strLocalVariable = getStringValue(longVal);
            assertEquals("Invalid String value", strLocalVariable, expectedValue);
        } else if (tag == JDWPConstants.Tag.LONG_TAG) {
            assertEquals("Invalid long value", longVal, ((Long)expectedValue).longValue());
        }

        logWriter.println(testName + " done");
    }
}
