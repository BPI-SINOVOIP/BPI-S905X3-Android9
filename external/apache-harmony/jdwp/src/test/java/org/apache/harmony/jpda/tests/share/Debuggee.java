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
 * @author Vitaly A. Provodin
 */

/**
 * Created on 31.01.2005
 */
package org.apache.harmony.jpda.tests.share;

import org.apache.harmony.jpda.tests.framework.LogWriter;
import org.apache.harmony.jpda.tests.framework.TestErrorException;

/**
 * All debuggee of unit tests must extend this class
 */
public abstract class Debuggee {

    /**
     * Instance of LogWriter implementation.
     */
    public LogWriter logWriter;

    /**
     * Instance of JPDA options used in all JPDA/JDWP tests.
     */
    public JPDATestOptions settings;

    /**
     * Executes main actions of debuggee. This method must be implemented by
     * subclasses.
     */
    public abstract void run();

    /**
     * Initializes debuggee
     */
    public void onStart() {
        settings = new JPDATestOptions();
        logWriter = new JPDALogWriter(System.out, null, settings.isVerbose());
    }

    /**
     * Executes final stage of debuggee.
     */
    public void onFinish() {
    }

    /**
     * Starts debuggee specified by <code>debuggeeClass</code>.
     * 
     * @param debuggeeClass
     *            debuggee's class
     */
    public static void runDebuggee(Class<? extends Debuggee> debuggeeClass) {
        Debuggee debuggee = null;
        try {
            debuggee = debuggeeClass.newInstance();
        } catch (Exception e) {
            throw new TestErrorException("Debuggee can not be started: "
                    + debuggeeClass.getName(), e);
        }

        debuggee.onStart();

        try {
            debuggee.run();
        } catch (Throwable e) {
            debuggee.logWriter.printError(e);
        } finally {
            debuggee.onFinish();
        }
    }
}
