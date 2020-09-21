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
 * Debuggee for Breakpoint003Test unit test.
 */
public class Breakpoint003Debuggee extends SyncDebuggee {
    public static final String VALUE_STRING = "1234";
    public static final long VALUE_LONG = 0x12a05f200L;

    private int callFrameworkMethod1(String str) {
        return Integer.parseInt(str);
    }

    private static String callFrameworkMethod2(long val) {
        return Long.toString(val);
    }

    @Override
    public void run() {
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        logWriter.println("Breakpoint003Debuggee started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        callFrameworkMethod1(VALUE_STRING);
        callFrameworkMethod2(VALUE_LONG);

        logWriter.println("Breakpoint003Debuggee finished");
    }

    public static void main(String[] args) {
        runDebuggee(Breakpoint003Debuggee.class);
    }

}
