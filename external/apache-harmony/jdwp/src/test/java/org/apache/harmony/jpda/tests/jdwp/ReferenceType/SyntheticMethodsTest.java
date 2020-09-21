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
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;
import org.apache.harmony.jpda.tests.jdwp.share.debuggee.SyntheticMembersDebuggee;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP Unit test for ReferenceType.Methods and ReferenceType.MethodsWithGeneric commands.
 */
public class SyntheticMethodsTest extends JDWPSyncTestCase {

    private static final int METHOD_SYNTHETIC_FLAG = 0xf0000000;

    @Override
    protected String getDebuggeeClassName() {
        return SyntheticMembersDebuggee.class.getName();
    }

    /**
     * This testcase exercises ReferenceType.Methods command.
     *
     * The test starts SyntheticMembersDebuggee class and verify that its non-static inner
     * class has at least one synthetic method (an access method).
     */
    public void testSyntheticMethods() {
        runTestSyntheticMethods(false);
    }

    /**
     * This testcase exercises ReferenceType.MethodsWithGeneric command.
     *
     * The test starts SyntheticMembersDebuggee class and verify that its non-static inner
     * class has at least one synthetic method (an access method).
     */
    public void testSyntheticMethodsWithGeneric() {
        runTestSyntheticMethods(true);
    }

    private void runTestSyntheticMethods(boolean withGeneric) {
        String thisTestName = getName();
        final byte commandCode;
        final String commandName;
        if (withGeneric) {
            commandCode = JDWPCommands.ReferenceTypeCommandSet.MethodsWithGenericCommand;
            commandName = "ReferenceType.MethodsWithGeneric command";
        } else {
            commandCode = JDWPCommands.ReferenceTypeCommandSet.MethodsCommand;
            commandName = "ReferenceType.Methods command";
        }
        logWriter.println("==> " + thisTestName + " for " + commandName + ": START...");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        String signature = getDebuggeeClassSignature();
        long refTypeID = getClassIDBySignature(signature);

        logWriter.println("=> Debuggee class = " + getDebuggeeClassName());
        logWriter.println("=> referenceTypeID for debuggee class = " + refTypeID);
        logWriter.println("=> CHECK: send " + commandName + " and check reply...");

        CommandPacket methodsCommand = new CommandPacket(
                JDWPCommands.ReferenceTypeCommandSet.CommandSetID, commandCode);
        methodsCommand.setNextValueAsReferenceTypeID(refTypeID);
        ReplyPacket methodsReply = debuggeeWrapper.vmMirror.performCommand(methodsCommand);
        checkReplyPacket(methodsReply, commandName);

        int returnedMethodsNumber = methodsReply.getNextValueAsInt();
        logWriter.println("=> Returned methods number = " + returnedMethodsNumber);

        logWriter.println("=> CHECK for all expected methods...");
        boolean foundSyntheticMethod = false;
        for (int i = 0; i < returnedMethodsNumber; i++) {
            long methodID = methodsReply.getNextValueAsMethodID();
            String methodName = methodsReply.getNextValueAsString();
            String methodSignature = methodsReply.getNextValueAsString();
            logWriter.println("\n=> Method ID = " + methodID);
            logWriter.println("=> Method name = " + methodName);
            logWriter.println("=> Method signature = " + methodSignature);
            if (withGeneric) {
                String methodGenericSignature = methodsReply.getNextValueAsString();
                logWriter.println("=> Method generic signature = " + methodGenericSignature);
            }
            int methodModifiers = methodsReply.getNextValueAsInt();
            logWriter.println("=> Method modifiers = 0x" + Integer.toHexString(methodModifiers));
            if ((methodModifiers & METHOD_SYNTHETIC_FLAG) == METHOD_SYNTHETIC_FLAG) {
                // We found a synthetic method.
                foundSyntheticMethod = true;
            }
        }
        assertAllDataRead(methodsReply);

        assertTrue("Did not find any synthetic method", foundSyntheticMethod);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
        logWriter.println("==> " + thisTestName + " for " + commandName + ": FINISH");
    }
}
