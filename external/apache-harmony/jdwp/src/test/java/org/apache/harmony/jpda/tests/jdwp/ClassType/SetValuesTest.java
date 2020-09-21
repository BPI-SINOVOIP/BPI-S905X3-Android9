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
 * Created on 10.02.2005
 */
package org.apache.harmony.jpda.tests.jdwp.ClassType;

import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.Field;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;



/**
 * JDWP unit test for ClassType.SetValues command.
 */

public class SetValuesTest extends JDWPClassTypeTestCase {
    /**
     * This testcase exercises ClassType.SetValues command.
     * <BR>Starts <A HREF="ClassTypeDebuggee.html">ClassTypeDebuggee</A>.
     * <BR>Receives all fields from debuggee with ReferenceType.fields command.
     * Then sets values for these fields with ClassType.SetValues command
     * and checks set values using ReferenceType.GetValues command.
     */
    public void testSetValues001() {
        logWriter.println("testSetValues001 started");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        long classID = getClassIDBySignature(getDebuggeeSignature());

        Field[] fields = jdwpGetFieldIDs(classID);

        for (int i = 0; i < fields.length; i++) {
            Field field = fields[i];
            //logWriter.println(field.toString());
            testField(classID, field);
        }
    }

    private void testField(long classID, Field fieldInfo) {

        //System.err.println("testField: "+fieldInfo.toString());
        // if field has primitive type
        if (fieldInfo.getSignature().length()>=1) {
            switch (fieldInfo.getSignature().charAt(0)) {
            case 'B': // byte
                testField(classID, fieldInfo, Value.createByte(Byte.MIN_VALUE));
                testField(classID, fieldInfo, Value.createByte(Byte.MAX_VALUE));
                testField(classID, fieldInfo, Value.createByte((byte)0));
                break;
            case 'C': // char
                testField(classID, fieldInfo, Value.createChar(Character.MAX_VALUE));
                testField(classID, fieldInfo, Value.createChar(Character.MIN_VALUE));
                break;
            case 'F': // float
                testField(classID, fieldInfo, Value.createFloat(Float.MIN_VALUE));
                testField(classID, fieldInfo, Value.createFloat(Float.MAX_VALUE));
                testField(classID, fieldInfo, Value.createFloat(Float.NaN));
                testField(classID, fieldInfo, Value.createFloat(Float.NEGATIVE_INFINITY));
                testField(classID, fieldInfo, Value.createFloat(Float.POSITIVE_INFINITY));
                testField(classID, fieldInfo, Value.createFloat(0));
                break;
            case 'D': // double
                testField(classID, fieldInfo, Value.createDouble(Double.MIN_VALUE));
                testField(classID, fieldInfo, Value.createDouble(Double.MAX_VALUE));
                testField(classID, fieldInfo, Value.createDouble(Double.NaN));
                testField(classID, fieldInfo, Value.createDouble(Double.NEGATIVE_INFINITY));
                testField(classID, fieldInfo, Value.createDouble(Double.POSITIVE_INFINITY));
                testField(classID, fieldInfo, Value.createDouble(0));
                break;
            case 'I': // int
                testField(classID, fieldInfo, Value.createInt(Integer.MIN_VALUE));
                testField(classID, fieldInfo, Value.createInt(Integer.MAX_VALUE));
                testField(classID, fieldInfo, Value.createInt(0));
                break;
            case 'J': // long
                testField(classID, fieldInfo, Value.createLong(Long.MIN_VALUE));
                testField(classID, fieldInfo, Value.createLong(Long.MAX_VALUE));
                testField(classID, fieldInfo, Value.createLong(0));
                break;
            case 'S': // short
                testField(classID, fieldInfo, Value.createShort(Short.MIN_VALUE));
                testField(classID, fieldInfo, Value.createShort(Short.MAX_VALUE));
                testField(classID, fieldInfo, Value.createShort((short)0));
                break;
            case 'Z': // boolean
                testField(classID, fieldInfo, Value.createBoolean(Boolean.FALSE.booleanValue()));
                testField(classID, fieldInfo, Value.createBoolean(Boolean.TRUE.booleanValue()));
                break;
            }
        }
    }

    private void testField(long classID, Field fieldInfo, Value value) {

        logWriter.println("\n==> testField: ");
        logWriter.println("    classID = " + classID);
        logWriter.println("    fieldInfo = " + fieldInfo);
        logWriter.println("    value to set = " + value);
        CommandPacket packet = new CommandPacket(
                JDWPCommands.ClassTypeCommandSet.CommandSetID,
                JDWPCommands.ClassTypeCommandSet.SetValuesCommand);
        packet.setNextValueAsClassID(classID);
        packet.setNextValueAsInt(1);
        packet.setNextValueAsFieldID(fieldInfo.getFieldID());

        packet.setNextValueAsUntaggedValue(value);

        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "ClassType::SetValues command");

        packet = new CommandPacket(
                JDWPCommands.ReferenceTypeCommandSet.CommandSetID,
                JDWPCommands.ReferenceTypeCommandSet.GetValuesCommand);
        packet.setNextValueAsReferenceTypeID(classID);
        packet.setNextValueAsInt(1);
        packet.setNextValueAsFieldID(fieldInfo.getFieldID());

        reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "ClassType::GetValues command");

        int fields = reply.getNextValueAsInt();
        assertEquals("ClassType::SetValues returne invalid fields number,", 1, fields);
        Value returnedValue = reply.getNextValueAsValue();
        assertEquals("ClassType::SetValues returne invalid returned value,", value, returnedValue);
        logWriter.println("==> testField: OK");
    }

    private Field[] jdwpGetFieldIDs(long classID) {
        return debuggeeWrapper.vmMirror.getFieldsInfo(classID);
    }

}
