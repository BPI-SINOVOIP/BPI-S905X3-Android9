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

package org.apache.harmony.jpda.tests.jdwp.Method;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.Method;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * JDWP Unit test for Method.VariableTable command.
 */
abstract class JDWPMethodVariableTableTestCase extends JDWPMethodTestCase {

    // All methods that must be checked against VariableTable[WithGeneric] reply.
    private final Map<String, VariableTableChecker> checkerMap =
            new HashMap<String, VariableTableChecker>();

    @Override
    protected final String getDebuggeeClassName() {
        return MethodVariableTestDebuggee.class.getName();
    }

    protected JDWPMethodVariableTableTestCase() {
        initCheckerMap();
    }

    private void initCheckerMap() {
        initStaticMethods();
        initInstanceMethods();
    }

    private void initStaticMethods() {
        {
            VariableTableChecker checker = new VariableTableChecker("static_NoParam");
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_BooleanParam");
            checker.addParameter("booleanParam", "Z", 0);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_ByteParam");
            checker.addParameter("byteParam", "B", 0);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_CharParam");
            checker.addParameter("charParam", "C", 0);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_ShortParam");
            checker.addParameter("shortParam", "S", 0);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_IntParam");
            checker.addParameter("intParam", "I", 0);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_FloatParam");
            checker.addParameter("floatParam", "F", 0);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_LongParam");
            checker.addParameter("longParam", "J", 0);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_DoubleParam");
            checker.addParameter("doubleParam", "D", 0);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_ObjectParam");
            checker.addParameter("objectParam", "Ljava/lang/Object;", 0);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_StringParam");
            checker.addParameter("stringParam", "Ljava/lang/String;", 0);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_ListOfStringsParam");
            checker.addParameter("listOfStringsParam", "Ljava/util/List;", 0);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_GenericParam");
            checker.addParameter("genericParam", "Ljava/lang/CharSequence;", 0);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_IntArrayParam");
            checker.addParameter("intArrayParam", "[I", 0);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_StringMultiArrayParam");
            checker.addParameter("stringMultiArrayParam", "[[Ljava/lang/String;", 0);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_IntLongParam");
            checker.addParameter("intParam", "I", 0);
            checker.addParameter("longParam", "J", 1);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("static_LongIntParam");
            checker.addParameter("longParam", "J", 0);
            checker.addParameter("intParam", "I", 2);
            addCheckerToMap(checker);
        }

        {
            VariableTableChecker checker = new VariableTableChecker("static_NoParamWithLocal");
            checker.addLocal("i", "I");
            checker.addLocal("l", "J");
            addCheckerToMap(checker);
        }
    }

    private void initInstanceMethods() {
        {
            VariableTableChecker checker = new VariableTableChecker("instance_NoParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("instance_BooleanParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("booleanParam", "Z", 1);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("instance_ByteParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("byteParam", "B", 1);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("instance_CharParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("charParam", "C", 1);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("instance_ShortParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("shortParam", "S", 1);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("instance_IntParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("intParam", "I", 1);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("instance_FloatParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("floatParam", "F", 1);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("instance_LongParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("longParam", "J", 1);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("instance_DoubleParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("doubleParam", "D", 1);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("instance_ObjectParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("objectParam", "Ljava/lang/Object;", 1);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("instance_StringParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("stringParam", "Ljava/lang/String;", 1);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("instance_ListOfStringsParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("listOfStringsParam", "Ljava/util/List;", 1);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("instance_GenericParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("genericParam", "Ljava/lang/CharSequence;", 1);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("instance_IntArrayParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("intArrayParam", "[I", 1);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker(
                    "instance_StringMultiArrayParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("stringMultiArrayParam", "[[Ljava/lang/String;", 1);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("instance_IntLongParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("intParam", "I", 1);
            checker.addParameter("longParam", "J", 2);
            addCheckerToMap(checker);
        }
        {
            VariableTableChecker checker = new VariableTableChecker("instance_LongIntParam");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addParameter("longParam", "J", 1);
            checker.addParameter("intParam", "I", 3);
            addCheckerToMap(checker);
        }

        {
            VariableTableChecker checker = new VariableTableChecker("instance_NoParamWithLocal");
            checker.addParameter("this", getDebuggeeClassSignature(), 0);
            checker.addLocal("i", "I");
            checker.addLocal("l", "J");
            addCheckerToMap(checker);
        }
    }

    private void addCheckerToMap(VariableTableChecker checker) {
        checkerMap.put(checker.methodName, checker);
    }

    /**
     * This testcase exercises Method.VariableTable[WithGeneric] command. <BR>
     * It runs MethodDebuggee, receives methods of debuggee. For each received
     * method sends Method.VariableTable command and checks the returned
     * VariableTable.
     *
     * @param withGenerics
     *            true to test Method.VariableTableWithGeneric, false to test
     *            Method.VariableTable.
     */
    protected void checkMethodVariableTable(boolean withGenerics) {
        final String testName = getName();
        logWriter.println(testName + " started");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        long classID = getClassIDBySignature(getDebuggeeClassSignature());

        Method[] methodsInfo = debuggeeWrapper.vmMirror.getMethods(classID);
        assertFalse("Invalid number of methods: 0", methodsInfo.length == 0);

        final byte commandCode;
        final String commandName;
        if (withGenerics) {
            // Testing Method.VariableTableWithGeneric command.
            commandCode = JDWPCommands.MethodCommandSet.VariableTableWithGenericCommand;
            commandName = "Method::VariableTableWithGeneric";
        } else {
            // Testing Method.VariableTable command.
            commandCode = JDWPCommands.MethodCommandSet.VariableTableCommand;
            commandName = "Method::VariableTable";
        }

        int checkedMethodsCount = 0;
        for (Method methodInfo : methodsInfo) {
            logWriter.println(methodInfo.toString());

            // get variable table for this class
            CommandPacket packet = new CommandPacket(JDWPCommands.MethodCommandSet.CommandSetID,
                    commandCode);
            packet.setNextValueAsClassID(classID);
            packet.setNextValueAsMethodID(methodInfo.getMethodID());
            ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
            checkReplyPacket(reply, commandName + " command");

            int argCnt = reply.getNextValueAsInt();
            logWriter.println("argCnt = " + argCnt);
            int slots = reply.getNextValueAsInt();
            logWriter.println("slots = " + slots);

            VariableTableChecker checker = getCheckerForMethod(methodInfo.getName());
            if (checker != null) {
                ++checkedMethodsCount;
                logWriter.println("# Check method \"" + checker.methodName + "\"");

                // Check argCount.
                int expectedArgsCount = checker.getArgCount();
                assertEquals("Invalid argCount", expectedArgsCount, argCnt);
            }
            int checkedVariablesCount = 0;
            for (int j = 0; j < slots; j++) {
                long codeIndex = reply.getNextValueAsLong();
                logWriter.println("codeIndex = " + codeIndex);
                String name = reply.getNextValueAsString();
                logWriter.println("name = \"" + name + "\"");
                String signature = reply.getNextValueAsString();
                logWriter.println("signature = \"" + signature + "\"");
                String genericSignature = "";
                if (withGenerics) {
                    reply.getNextValueAsString();
                    logWriter.println("genericSignature = \"" + genericSignature + "\"");
                }
                int length = reply.getNextValueAsInt();
                logWriter.println("length = " + length);
                int slot = reply.getNextValueAsInt();
                logWriter.println("slot = " + slot);

                if (checker != null) {
                    VariableInfo variableInfo = checker.getVariableInfo(name);
                    if (variableInfo != null) {
                        ++checkedVariablesCount;
                        logWriter.println("## Check variable \"" + variableInfo.name + "\"");

                        // Check signature
                        assertEquals("Invalid variable type signature",
                            variableInfo.signature, signature);

                        if (variableInfo.isParameter) {
                            // It would be nice to check the slot but that isn't specified by the
                            // JLS. So different runtimes might have different values.
                            //
                            // A parameter's scope start is == 0.
                            assertEquals("Invalid codeIndex " + codeIndex + " for parameter \"" +
                                    name + "\"", 0, codeIndex);
                        } else {
                            // It would be nice to check the slot but that isn't specified by the
                            // JLS. So different runtimes might have different values.
                            //
                            // A local variable's scope start is >= 0.
                            assertTrue("Invalid codeIndex " + codeIndex + " for local var \"" +
                                    name + "\"", codeIndex >= 0);
                        }
                    }
                }
            }

            if (checker != null) {
                // Check that we found all the variables we want to check for the current method.
                assertEquals("Not all variables have been checked for method " + checker.methodName,
                        checker.variableInfos.size(), checkedVariablesCount);
            }

        }

        // Checks that we found all the methods that we want to check.
        assertEquals("Not all methods have been checked", checkerMap.size(), checkedMethodsCount);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
        logWriter.println(testName + " ends");
    }

    private VariableTableChecker getCheckerForMethod(String methodName) {
        return checkerMap.get(methodName);
    }

    private static class VariableTableChecker {
        final String methodName;
        private final List<VariableInfo> variableInfos =
                new ArrayList<JDWPMethodVariableTableTestCase.VariableInfo>();

        public VariableTableChecker(String methodName) {
            this.methodName = methodName;
        }

        private void addVariableInfo(VariableInfo variableInfo) {
            variableInfos.add(variableInfo);
        }

        public void addParameter(String name, String signature, int slot) {
            addVariableInfo(new VariableInfo(true, name, signature, slot));
        }

        public void addLocal(String name, String signature) {
            addVariableInfo(new VariableInfo(false, name, signature, -1));
        }

        public int getArgCount() {
            int argsCount = 0;
            for (VariableInfo variableInfo : variableInfos) {
                if (variableInfo.isParameter) {
                    String sig = variableInfo.signature;
                    if (sig.equals("J") || sig.equals("D")) {
                        // long and double take 2 slots.
                        argsCount += 2;
                    } else {
                        argsCount += 1;
                    }
                }
            }
            return argsCount;
        }

        public VariableInfo getVariableInfo(String variableName) {
            for (VariableInfo v : variableInfos) {
                if (v.name.equals(variableName)) {
                    return v;
                }
            }
            return null;
        }
    }

    private static class VariableInfo {
        // Is it a parameter ?
        private final boolean isParameter;
        // The variable's name.
        private final String name;
        // The variable's signature.
        private final String signature;
        // The expected slot for parameters only.
        private final int expectedParameterSlot;

        public VariableInfo(boolean isParameter, String variableName, String variableSignature,
                            int expectedParameterSlot) {
            this.isParameter = isParameter;
            this.name = variableName;
            this.signature = variableSignature;
            this.expectedParameterSlot = expectedParameterSlot;
        }
    }
}
