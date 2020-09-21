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
 * @author Anatoly F. Bondarenko
 */

/**
 * Created on 04.03.2005
 */
package org.apache.harmony.jpda.tests.jdwp.ObjectReference;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPTestConstants;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;


/**
 * JDWP Unit test for ObjectReference.DisableCollection command.
 */
public class DisableCollectionTest extends JDWPSyncTestCase {

    static final String thisCommandName = "ObjectReference::DisableCollection command";

    static final String debuggeeSignature = "Lorg/apache/harmony/jpda/tests/jdwp/ObjectReference/DisableCollectionDebuggee;";

    @Override
    protected String getDebuggeeClassName() {
        return "org.apache.harmony.jpda.tests.jdwp.ObjectReference.DisableCollectionDebuggee";
    }

    /**
     * This testcase exercises ObjectReference.DisableCollection command.
     * <BR>The test starts DisableCollectionDebuggee class, gets
     * objectID as value of static field of this class which (field) represents
     * checked object. Then for this objectID test executes
     * ObjectReference.DisableCollection command for checked object. After that
     * Debuggee tries to unload checked object and checks if checked object is
     * unloaded. <BR>If so the test fails, otherwise it passes.
     */
    public void testDisableCollection001() {
        String thisTestName = "testDisableCollection001";
        logWriter.println("==> " + thisTestName + " for " + thisCommandName
                + ": START...");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        finalSyncMessage = "TO_FINISH";

        long refTypeID = getClassIDBySignature(debuggeeSignature);

        logWriter.println("=> Debuggee class = " + getDebuggeeClassName());
        logWriter.println("=> referenceTypeID for Debuggee class = "
                + refTypeID);

        long checkedFieldID = checkField(refTypeID, "checkedObject");

        logWriter
                .println("=> Send ReferenceType::GetValues command for received fieldID and get ObjectID to check...");

        CommandPacket getValuesCommand = new CommandPacket(
                JDWPCommands.ReferenceTypeCommandSet.CommandSetID,
                JDWPCommands.ReferenceTypeCommandSet.GetValuesCommand);
        getValuesCommand.setNextValueAsReferenceTypeID(refTypeID);
        getValuesCommand.setNextValueAsInt(1);
        getValuesCommand.setNextValueAsFieldID(checkedFieldID);

        ReplyPacket getValuesReply = debuggeeWrapper.vmMirror
                .performCommand(getValuesCommand);
        getValuesCommand = null;
        checkReplyPacket(getValuesReply, "ReferenceType::GetValues command");

        int returnedValuesNumber = getValuesReply.getNextValueAsInt();
        logWriter
                .println("=> Returned values number = " + returnedValuesNumber);
        assertEquals(
                "Invalid number of values returned from ReferenceType::GetValues command,",
                1, returnedValuesNumber);

        Value checkedObjectFieldValue = getValuesReply.getNextValueAsValue();
        byte checkedObjectFieldTag = checkedObjectFieldValue.getTag();
        logWriter.println("=> Returned field value tag for checked object= "
                + checkedObjectFieldTag + "("
                + JDWPConstants.Tag.getName(checkedObjectFieldTag) + ")");
        assertEquals("Invalid value tag for checked object,",
                JDWPConstants.Tag.OBJECT_TAG, checkedObjectFieldTag,
                JDWPConstants.Tag.getName(JDWPConstants.Tag.OBJECT_TAG),
                JDWPConstants.Tag.getName(checkedObjectFieldTag));

        long checkedObjectID = checkedObjectFieldValue.getLongValue();
        logWriter.println("=> Returned checked ObjectID = " + checkedObjectID);

        logWriter.println("\n=> CHECK: send " + thisCommandName
                + " for checked ObjectID...");

        CommandPacket checkedCommand = new CommandPacket(
                JDWPCommands.ObjectReferenceCommandSet.CommandSetID,
                JDWPCommands.ObjectReferenceCommandSet.DisableCollectionCommand);
        checkedCommand.setNextValueAsObjectID(checkedObjectID);

        ReplyPacket checkedReply = debuggeeWrapper.vmMirror
                .performCommand(checkedCommand);
        checkedCommand = null;
        checkReplyPacket(checkedReply, thisCommandName);

        logWriter.println("=> CHECK: Reply is received without any error");
        logWriter
                .println("=> Send to Debuggee signal to continue and try to unload checked ObjectID...");
        finalSyncMessage = null;
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
        String messageFromDebuggee = synchronizer
                .receiveMessageWithoutException("DisableCollectionDebuggee(#2)");
        logWriter.println("==> debuggeeMsg = |" + messageFromDebuggee + "|");
        if (messageFromDebuggee.equals("Checked Object is UNLOADed!")) {
            logWriter.println("\n## FAILURE: Checked Object is UNLOADed after "
                    + thisCommandName);
            fail("Checked Object is UNLOADed after " + thisCommandName);
        } else {
            logWriter
                    .println("\n=> PASSED: Checked Object is NOT UNLOADed after "
                            + thisCommandName);
        }

        logWriter.println("=> Send to Debuggee signal to funish ...");
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
        logWriter.println("==> " + thisTestName + " for " + thisCommandName
                + ": FINISH");

        assertAllDataRead(checkedReply);
    }

    /**
     * This testcase exercises ObjectReference.DisableCollection command.
     * <BR>The test starts DisableCollectionDebuggee class. Then attempts to
     * disable collection for an invalid objectID and checks INVALID_OBJECT is
     * returned.
     */
    public void testDisableCollection_invalid() {
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        disableCollection(JDWPTestConstants.INVALID_OBJECT_ID,
            JDWPConstants.Error.INVALID_OBJECT);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    /**
     * This testcase exercises ObjectReference.DisableCollection command.
     * <BR>The test starts DisableCollectionDebuggee class. Then attempts to
     * disable collection for "null" object (id=0) and checks INVALID_OBJECT is
     * returned.
     */
    public void testDisableCollection_null() {
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        disableCollection(JDWPTestConstants.NULL_OBJECT_ID,
            JDWPConstants.Error.INVALID_OBJECT);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    private void disableCollection(long objectID, int expectedErrorCode) {
      CommandPacket command = new CommandPacket(
          JDWPCommands.ObjectReferenceCommandSet.CommandSetID,
          JDWPCommands.ObjectReferenceCommandSet.DisableCollectionCommand);
      command.setNextValueAsObjectID(objectID);

      ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(command);
      checkReplyPacket(reply, thisCommandName, expectedErrorCode);
    }
}
