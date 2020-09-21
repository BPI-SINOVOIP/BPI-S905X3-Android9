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

package org.apache.harmony.jpda.tests.jdwp.ReferenceType;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;


/**
 * JDWP Unit test for ReferenceType.GetValues command for static field of interface class.
 */
public class GetValues007Test extends JDWPSyncTestCase {

    @Override
    protected String getDebuggeeClassName() {
        return GetValues007Debuggee.class.getName();
    }

    /**
     * This tests the ReferenceType.GetValues command on the static field of an interface.
     * <BR>The test starts GetValues007Debuggee and checks that the ReferenceType.GetValues
     * command runs correctly for a static field declared in an interface.
     * <BR>Test checks that expected value of this field is returned.
     */
    public void testGetValues007() {
        String thisTestName = "testGetValues007";
        logWriter.println("==> " + thisTestName +
            " for ReferenceType.GetValues command: START...");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        logWriter.println("\n=> Get debuggeeRefTypeID for debuggee class = "
            + getDebuggeeClassName() + "...");
        long debuggeeRefTypeID = getClassIDBySignature(getDebuggeeClassSignature());
        logWriter.println("=> debuggeeRefTypeID = " + debuggeeRefTypeID);

        logWriter.println(
            "\n=> Get implementerRefTypeID for implementer = GetValues007Implementer...");
        long implementerRefTypeID =
            getClassIDBySignature(getClassSignature(GetValues007Interface.class));
        logWriter.println("=> implementerRefTypeID = " + implementerRefTypeID);

        logWriter.println("\n=> Get interfaceFieldID for field of interface class...");
        String interfaceFieldName = "interfaceStaticIntVar";
        long interfaceFieldID = checkField(implementerRefTypeID, interfaceFieldName);
        logWriter.println("=> interfaceFieldID = " + interfaceFieldID);

        logWriter.println("\n=> CHECK ClassType::SetValues command for implementerRefTypeID," +
            " interfaceFieldID...");
        int expectedIntValue = 2;
        CommandPacket setValuesCommand = new CommandPacket(
            JDWPCommands.ClassTypeCommandSet.CommandSetID,
            JDWPCommands.ClassTypeCommandSet.SetValuesCommand);
        setValuesCommand.setNextValueAsClassID(implementerRefTypeID);
        setValuesCommand.setNextValueAsInt(1);
        setValuesCommand.setNextValueAsFieldID(interfaceFieldID);
        setValuesCommand.setNextValueAsInt(expectedIntValue);
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(setValuesCommand);
        checkReplyPacket(reply, "ClassType::SetValues command");

        logWriter.println("\n=> CHECK ReferenceType::GetValues command for implementerRefTypeID," +
            " interfaceFieldID...");
        CommandPacket getValuesCommand = new CommandPacket(
                JDWPCommands.ReferenceTypeCommandSet.CommandSetID,
                JDWPCommands.ReferenceTypeCommandSet.GetValuesCommand);
        getValuesCommand.setNextValueAsReferenceTypeID(implementerRefTypeID);
        getValuesCommand.setNextValueAsInt(1);
        getValuesCommand.setNextValueAsFieldID(interfaceFieldID);
        ReplyPacket getValuesReply = debuggeeWrapper.vmMirror.performCommand(getValuesCommand);
        checkReplyPacket(getValuesReply, "ReferenceType::GetValues command");

        getValuesReply.getNextValueAsInt();
        Value fieldValue = getValuesReply.getNextValueAsValue();
        byte fieldTag = fieldValue.getTag();
        logWriter.println("=> Returned value tag = " + fieldTag
            + "(" + JDWPConstants.Tag.getName(fieldTag) + ")");
        assertTagEquals("Invalid value tag is returned,", JDWPConstants.Tag.INT_TAG, fieldTag);

        int intValue = fieldValue.getIntValue();
        logWriter.println("=> Returned value = " + intValue);
        // here expected value = 2 (staticIntField)
        assertEquals("Invalid int value,", expectedIntValue, intValue);

        assertAllDataRead(getValuesReply);

        logWriter.println("=> CHECK PASSED: Expected value is returned!");

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
        logWriter.println("==> " + thisTestName + " for ReferenceType::GetValues command: FINISH");
    }
}
