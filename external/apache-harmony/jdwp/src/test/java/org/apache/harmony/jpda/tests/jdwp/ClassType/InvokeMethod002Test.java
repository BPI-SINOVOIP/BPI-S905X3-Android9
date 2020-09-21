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
 * @author Pavel N. Vyssotski
 */

// $Id: InvokeMethod002Test.java,v 1.4 2006/06/20 11:15:16 rscherba Exp $

package org.apache.harmony.jpda.tests.jdwp.ClassType;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.Field;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.TaggedObject;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;


/**
 * JDWP unit test for ClassType.InvokeMethod command.
 */
public class InvokeMethod002Test extends JDWPSyncTestCase {
    @Override
    protected String getDebuggeeClassName() {
        return "org.apache.harmony.jpda.tests.jdwp.share.debuggee.InvokeMethodDebuggee";
    }

    /**
     * This testcase checks ClassType.InvokeMethod command.
     * <BR>Checked method is
     * <BR>public static String testMethod3(
     *       int, int[], int[][], String, String[], String[][])
     * <BR>of org.apache.harmony.jpda.tests.jdwp.share.debuggee.InvokeMethodDebuggee class.
     * <BR> The testcase expects that returned value is not null object and returned
     * exception object is null;
     */
    private void testInvokeMethod(boolean deliberately_screw_up_types) {
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Get referenceTypeID
        CommandPacket packet = new CommandPacket(
            JDWPCommands.VirtualMachineCommandSet.CommandSetID,
            JDWPCommands.VirtualMachineCommandSet.ClassesBySignatureCommand);
        String classSig = "Lorg/apache/harmony/jpda/tests/jdwp/share/debuggee/InvokeMethodDebuggee;";
        packet.setNextValueAsString(classSig);

        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "VirtualMachine::ClassesBySignature command");

        int classes = reply.getNextValueAsInt();
        assertEquals("VirtualMachine::ClassesBySignature returned invalid number of classes,",
                1, classes); //this class may be loaded only once
        byte refTypeTag = reply.getNextValueAsByte();
        long classID = reply.getNextValueAsReferenceTypeID();
        int status = reply.getNextValueAsInt();
        assertAllDataRead(reply);
        assertEquals("VirtualMachine::ClassesBySignature returned invalid TypeTag,",
                JDWPConstants.TypeTag.CLASS, refTypeTag,
                JDWPConstants.TypeTag.getName(JDWPConstants.TypeTag.CLASS),
                JDWPConstants.TypeTag.getName(refTypeTag));

        logWriter.println(" VirtualMachine.ClassesBySignature: classes="
            + classes + " refTypeTag=" + refTypeTag + " classID= " + classID
            + " status=" + status);

        // Get methodID
        long targetMethodID = getMethodID(classID, "testMethod3");

        // Set EventRequest
        packet = new CommandPacket(
            JDWPCommands.EventRequestCommandSet.CommandSetID,
            JDWPCommands.EventRequestCommandSet.SetCommand);
        packet.setNextValueAsByte(JDWPConstants.EventKind.METHOD_ENTRY);
        packet.setNextValueAsByte(JDWPConstants.SuspendPolicy.ALL);
        packet.setNextValueAsInt(1);
        packet.setNextValueAsByte((byte) 5);
        packet.setNextValueAsString("*.InvokeMethodDebuggee");

        reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "EventRequest::Set command");

        int requestID = reply.getNextValueAsInt();
        logWriter.println(" EventRequest.Set: requestID=" + requestID);
        assertAllDataRead(reply);
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        long targetThreadID = 0;
        // Wait event
        CommandPacket event = debuggeeWrapper.vmMirror.receiveEvent();
        byte suspendPolicy = event.getNextValueAsByte();
        int events = event.getNextValueAsInt();
        logWriter.println(" EVENT_THREAD event: suspendPolicy=" + suspendPolicy
            + " events=" + events);
        for (int i = 0; i < events; i++) {
            byte eventKind = event.getNextValueAsByte();
            int newRequestID = event.getNextValueAsInt();
            long threadID = event.getNextValueAsThreadID();
            //Location location =
                event.getNextValueAsLocation();
            logWriter.println("  EVENT_THREAD event " + i + ": eventKind="
                + eventKind + " requestID=" + newRequestID + " threadID="
                + threadID);
            if (newRequestID == requestID) {
                targetThreadID = threadID;
            }
        }
        assertAllDataRead(event);
        assertTrue("Invalid targetThreadID, must be != 0", targetThreadID != 0);

        //  Let's clear event request
        packet = new CommandPacket(
            JDWPCommands.EventRequestCommandSet.CommandSetID,
            JDWPCommands.EventRequestCommandSet.ClearCommand);
        packet.setNextValueAsByte(JDWPConstants.EventKind.METHOD_ENTRY);
        packet.setNextValueAsInt(requestID);

        reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "EventRequest::Clear command");
        assertAllDataRead(reply);

        logWriter.println("Read values of fields to pass them as arguments:");
        String fieldNames[] = {
            "checkInt",
            "checkIntArray",
            "checkIntArray2",
            "checkString",
            "checkStringArray",
            "checkStringArray2",
            "checkClass",
            "checkClassArray",
            "checkClassArray2"
        };
        long[] fieldIDs = {0,0,0,0,0,0,0,0,0};
        Value[] fieldValues = {null,null,null,null,null,null,null,null,null};

        if (deliberately_screw_up_types) {
            fieldNames[1] = "checkStringArray";
            fieldNames[4] = "checkIntArray";
        }

        Field[] fields = debuggeeWrapper.vmMirror.getFieldsInfo(classID);

        int fieldsCount = fields.length;
        assertTrue("Invalid fieldsCount=" + fieldsCount + ", must be >= " + fieldValues.length
                , fieldsCount >= fieldValues.length);

        for (Field fieldInfo : fields) {
            long id = fieldInfo.getFieldID();
            String name = fieldInfo.getName();
            for (int k = 0; k < fieldNames.length; k++) {
                if (fieldNames[k].equals(name)) {
                    fieldIDs[k] = id;
                    logWriter.println("  name=" + name + ", ID=" + id);
                    break;
                }
            }
        }

        for (int i = 0; i < fieldIDs.length; i++) {
            if (fieldIDs[i] == 0) {
                logWriter.println(
                    "ERROR: \"" + fieldNames[i] + "\" field not found");
                fail("\"" + fieldNames[i] + "\" field not found");
            }
        }

        packet = new CommandPacket(
            JDWPCommands.ReferenceTypeCommandSet.CommandSetID,
            JDWPCommands.ReferenceTypeCommandSet.GetValuesCommand);
        packet.setNextValueAsReferenceTypeID(classID);
        packet.setNextValueAsInt(fieldIDs.length);
        for (int i = 0; i < fieldIDs.length; i++) {
            packet.setNextValueAsFieldID(fieldIDs[i]);
        }

        reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "ReferenceType::GetValues command");

        int valuesCount = reply.getNextValueAsInt();
        for (int i = 0; i < valuesCount; i++) {
            fieldValues[i] = reply.getNextValueAsValue();
        }
        assertAllDataRead(reply);

        // Make InvokeMethod
        packet = new CommandPacket(
            JDWPCommands.ClassTypeCommandSet.CommandSetID,
            JDWPCommands.ClassTypeCommandSet.InvokeMethodCommand);
        packet.setNextValueAsClassID(classID);
        packet.setNextValueAsThreadID(targetThreadID);
        packet.setNextValueAsMethodID(targetMethodID);
        packet.setNextValueAsInt(fieldValues.length);
        for (int i = 0; i < fieldValues.length; i++) {
            packet.setNextValueAsValue(fieldValues[i]);
        }
        packet.setNextValueAsInt(0);
        logWriter.println(" Send ClassType.InvokeMethod");
        reply = debuggeeWrapper.vmMirror.performCommand(packet);

        if (deliberately_screw_up_types) {
            checkReplyPacket(reply, "ClassType::InvokeMethod command",
                             JDWPConstants.Error.ILLEGAL_ARGUMENT);
        } else {
            checkReplyPacket(reply, "ClassType::InvokeMethod command");

            Value returnValue = reply.getNextValueAsValue();
            logWriter.println(" ClassType.InvokeMethod: returnValue=" + returnValue);

            String returnedString = getStringValue(returnValue.getLongValue());
            logWriter.println(" ClassType.InvokeMethod: returnedString=" + returnedString);
            assertEquals("qwerty", returnedString);

            TaggedObject exception = reply.getNextValueAsTaggedObject();
            assertNotNull("ClassType::InvokeMethod returned null exception",
                          exception);
            assertEquals("ClassType::InvokeMethod returned invalid exception objectID,",
                         0, exception.objectID);
            assertEquals("ClassType::InvokeMethod returned invalid exception.tag",
                         JDWPConstants.Tag.OBJECT_TAG, exception.tag,
                         JDWPConstants.Tag.getName(JDWPConstants.Tag.OBJECT_TAG),
                         JDWPConstants.Tag.getName(exception.tag));
            logWriter.println(" ClassType.InvokeMethod: exception.tag=" + exception.tag +
                              " exception.objectID=" + exception.objectID);
        }

        assertAllDataRead(reply);

        //  Let's resume application
        packet = new CommandPacket(
            JDWPCommands.VirtualMachineCommandSet.CommandSetID,
            JDWPCommands.VirtualMachineCommandSet.ResumeCommand);
        reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "VirtualMachine::Resume command");
        assertAllDataRead(reply);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    public void testInvokeMethod_right_argument_types() {
        testInvokeMethod(false);
    }

    public void testInvokeMethod_wrong_argument_types() {
        testInvokeMethod(true);
    }
}
