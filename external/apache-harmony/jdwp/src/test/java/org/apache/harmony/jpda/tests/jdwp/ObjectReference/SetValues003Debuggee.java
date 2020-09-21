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
 * Created on 17.03.2005
 */
package org.apache.harmony.jpda.tests.jdwp.ObjectReference;

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

public class SetValues003Debuggee extends SyncDebuggee {
    
    static String passedStatus = "PASSED";
    static String failedStatus = "FAILED";
    static String status = passedStatus;

    static SetValues003Debuggee setValues003DebuggeeObject;

    SetValues003Debuggee_ExtraClass objectField;
    static SetValues003Debuggee_ExtraClass objectFieldCopy;

    @Override
    public void run() {
        logWriter.println("--> Debuggee: SetValues003Debuggee: START");
        setValues003DebuggeeObject = new SetValues003Debuggee();
        setValues003DebuggeeObject.objectField = new SetValues003Debuggee_ExtraClass();
        objectFieldCopy = setValues003DebuggeeObject.objectField;

        logWriter.println("\n--> Debuggee: SetValues003Debuggee: Before ObjectReference::SetValues command:");
        logWriter.println("--> objectField value = " + setValues003DebuggeeObject.objectField);
        logWriter.println("--> value to set = " + setValues003DebuggeeObject);

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        logWriter.println("\n--> Debuggee: SetValues003Debuggee: After ObjectReference::SetValues command:");
        logWriter.println("--> objectField value = " + setValues003DebuggeeObject.objectField);
        if ( ! objectFieldCopy.equals(setValues003DebuggeeObject.objectField) ) {
            logWriter.println("##> Debuggee: FAILURE: Unexpected value");
            logWriter.println("##> Expected value = " + objectFieldCopy);
            status = failedStatus;
        } else {
            logWriter.println("--> Debuggee: PASSED: Expected value");
        }

        if ( status.equals(failedStatus) ) {
            logWriter.println("\n##> Debuggee: Check status = FAILED");
        } else {   
            logWriter.println("\n--> Debuggee: Check status = PASSED");
        }

        logWriter.println("--> Debuggee: Send check status for SetValues003Test...\n");
        synchronizer.sendMessage(status);

        logWriter.println("--> Debuggee: SetValues003Debuggee: FINISH");
    }

    public static void main(String [] args) {
        runDebuggee(SetValues003Debuggee.class);
    }
}

class SetValues003Debuggee_ExtraClass {
   
}
