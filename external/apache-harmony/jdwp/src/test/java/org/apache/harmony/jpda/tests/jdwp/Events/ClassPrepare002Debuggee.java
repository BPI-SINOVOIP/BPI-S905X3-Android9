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

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

/**
 * Debuggee for ClassPrepare002Test unit test.
 */
public class ClassPrepare002Debuggee extends SyncDebuggee {

    public static void main(String[] args) {
        runDebuggee(ClassPrepare002Debuggee.class);
    }

    @Override
    public void run() {
        logWriter.println("ClassPrepare002Debuggee started");

        // Load TestClassA *without* initializing the class and notify the test.
        try {
            Class.forName("org.apache.harmony.jpda.tests.jdwp.Events.TestClassA", false,
                    getClass().getClassLoader());
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        // Wait for the test to finish.
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        logWriter.println("ClassPrepare002Debuggee finished");
    }
}

/**
 * The class that will be investigated by the debugger for its static fields
 * (ReferenceType.GetValues command).
 */
class TestClassA {
    static TestClassB field = new TestClassB();
}

/**
 * The class that will cause the CLASS PREPARE event when the debugger investigates static fields
 * of the other class above.
 */
class TestClassB {
}
