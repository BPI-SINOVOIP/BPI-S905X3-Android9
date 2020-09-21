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

package org.apache.harmony.jpda.tests.jdwp.share.debuggee;

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

public class SyntheticMembersDebuggee extends SyncDebuggee {

    // Accessing this method will force the compiler to create a synthetic bridge method.
    private void outerMethod() {
    }

    // A non-static inner class should have a reference to its "outer this" in a synthetic field.
    public class InnerClass {
        @SuppressWarnings("unused")
        private void callOuterMethod() {
            outerMethod();
        }
    }

    @Override
    public void run() {
        logWriter.println("--> SyntheticMembersDebuggee START");

        // Force class loading to ensure the class is visible through JDWP.
        new InnerClass();

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        logWriter.println("--> Debuggee: SyntheticMembersDebuggee...");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        logWriter.println("--> SyntheticMembersDebuggee END");
    }

    public static void main(String [] args) {
        runDebuggee(SyntheticMembersDebuggee.class);
    }

}