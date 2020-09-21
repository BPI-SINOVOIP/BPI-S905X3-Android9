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
 * Created on 24.02.2005
 */
package org.apache.harmony.jpda.tests.jdwp.ReferenceType;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;


/**
 * JDWP Unit test for ReferenceType.SourceDebugExtension command.
 */
public class SourceDebugExtensionTest extends JDWPSyncTestCase {

    static final int testStatusPassed = 0;
    static final int testStatusFailed = -1;
    static final String thisCommandName = "ReferenceType.SourceDebugExtension command";
    static final String debuggeeSignature =
            "Lorg/apache/harmony/jpda/tests/jdwp/ReferenceType/SourceDebugExtensionDebuggee;";
    static final String expectedSourceDebugExtension = "SMAP\nhelloworld_jsp.java\nJSP\n*S JSP\n" +
            "*F\n+ 0 helloworld.jsp\nhelloworld.jsp\n*L\n1,5:53\n6:58,3\n7,4:61\n*E\n";

    @Override
    protected String getDebuggeeClassName() {
        return "org.apache.harmony.jpda.tests.jdwp.ReferenceType.SourceDebugExtensionDebuggee";
    }


    private void doTest(String testName, String classSignature, int expectedErrorCode) {
        //check capability, relevant for this test
        logWriter.println("=> Check capability: canGetSourceDebugExtension");
        if (!debuggeeWrapper.vmMirror.canGetSourceDebugExtension()) {
            logWriter.println("##WARNING: this VM doesn't possess capability: canGetSourceDebugExtension");
            return;
        }

        logWriter.println("==> " + testName + " for " + thisCommandName + ": START...");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        long refTypeID = getClassIDBySignature(classSignature);
        logWriter.println("=> Class with SourceDebugExtension = " + classSignature);
        logWriter.println("=> referenceTypeID for class with SourceDebugExtension = " + refTypeID);
        logWriter.println("=> CHECK: send " + thisCommandName + " and check reply...");

        CommandPacket checkedCommand = new CommandPacket(
                JDWPCommands.ReferenceTypeCommandSet.CommandSetID,
                JDWPCommands.ReferenceTypeCommandSet.SourceDebugExtensionCommand);
        checkedCommand.setNextValueAsReferenceTypeID(refTypeID);

        ReplyPacket checkedReply = debuggeeWrapper.vmMirror.performCommand(checkedCommand);
        checkedCommand = null;

        short errorCode = checkedReply.getErrorCode();
        assertEquals(expectedErrorCode, errorCode);

        switch ( errorCode ) {
            case JDWPConstants.Error.NONE:
                logWriter.println("=> No any ERROR is returned");
                String sourceDebugExtension = checkedReply.getNextValueAsString();
                logWriter.println("=> Returned SourceDebugExtension = " + sourceDebugExtension);
                assertEquals(expectedSourceDebugExtension, sourceDebugExtension);
                break;
            case JDWPConstants.Error.ABSENT_INFORMATION:
                logWriter.println("=> ABSENT_INFORMATION is returned");
                break;
            default:
                logWriter.println("\n## FAILURE: " + thisCommandName + " returns unexpected ERROR = "
                    + errorCode + "(" + JDWPConstants.Error.getName(errorCode) + ")");
                fail(thisCommandName + " returns unexpected ERROR = "
                    + errorCode + "(" + JDWPConstants.Error.getName(errorCode) + ")");
        }

        assertAllDataRead(checkedReply);

        logWriter.println("=> CHECK PASSED: Received expected result");

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
        logWriter.println("==> " + testName + " for " + thisCommandName + ": FINISH");
    }

    /**
     * This testcase exercises ReferenceType.SourceDebugExtension command.
     *
     * <BR>The test starts a SourceDebugExtensionDebuggee instance, which instantiates a
     * SourceDebugExtensionMockClass instance. The SourceDebugExtensionMockClass comes from a
     * class file generated by a JSP to Java bytecode compiler. The testcase requests
     * referenceTypeId for this class by VirtualMachine.ClassesBySignature command, then
     * performs ReferenceType.SourceDebugExtension to get the JSR45 metadata for the
     * class. The testcase checks that no any unexpected ERROR is returned and that
     * the JSR45 metadata matches the expected value.
     */
    public void testSourceDebugExtension001() {
        doTest("testSourceDebugExtension001",
               "Lorg/apache/harmony/jpda/tests/jdwp/Events/SourceDebugExtensionMockClass;",
               JDWPConstants.Error.NONE);
    }

    /**
     * This testcase exercises ReferenceType.SourceDebugExtension command.
     *
     * The class queried is a primitive type which on ART does not
     * have an associated DEX cache.
     */
    public void testSourceDebugExtension002() {
        doTest("testSourceDebugExtension001", "I", JDWPConstants.Error.ABSENT_INFORMATION);
    }

    /**
     * This testcase exercises ReferenceType.SourceDebugExtension command.
     *
     * The class queried is a primitive array which on ART does not
     * have an associated DEX cache.
     */
    public void testSourceDebugExtension003() {
        doTest("testSourceDebugExtension003", "[I", JDWPConstants.Error.ABSENT_INFORMATION);
    }

    /**
     * This testcase exercises ReferenceType.SourceDebugExtension command.
     *
     * The class queried is an array which on ART does not have an
     * associated DEX cache.
     */
    public void testSourceDebugExtension004() {
        doTest("testSourceDebugExtension004", "[Ljava/lang/String;",
               JDWPConstants.Error.ABSENT_INFORMATION);
    }

    /**
     * This testcase exercises ReferenceType.SourceDebugExtension command.
     *
     * The test queries all classes to find a proxy class and then
     * tries he SourceDebugExtension command on the proxy class. The
     * debuggee instantiates a proxy so we know there is at least one.
     */
    public void testSourceDebugExtension005() {
        // Identifying the name of a proxy class is tricky so get all class names and walk them:

        logWriter.println("==> Send VirtualMachine::AllClasses command...");
        CommandPacket packet = new CommandPacket(
            JDWPCommands.VirtualMachineCommandSet.CommandSetID,
            JDWPCommands.VirtualMachineCommandSet.AllClassesCommand);
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "VirtualMachine::AllClasses command");

        long typeID;
        String signature;
        int status;
        int classes = reply.getNextValueAsInt();
        int count = 0;
        for (int i = 0; i < classes; i++) {
            reply.getNextValueAsByte();
            typeID = reply.getNextValueAsReferenceTypeID();
            signature = reply.getNextValueAsString();
            status = reply.getNextValueAsInt();
            if (signature.contains("$Proxy")) {
                doTest("testSourceDebugExtension005", signature,
                       JDWPConstants.Error.ABSENT_INFORMATION);
                return;
            }
        }

        logWriter.println("\n## FAILURE: testSourceDebugExtension005 did not find a proxy class");
        fail("testSourceDebugExtension005 did not find a proxy class");
    }
}
