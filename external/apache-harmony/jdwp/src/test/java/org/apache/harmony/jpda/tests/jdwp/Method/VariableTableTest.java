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
 * Created on 14.02.2005
 */
package org.apache.harmony.jpda.tests.jdwp.Method;

/**
 * JDWP Unit test for Method.VariableTable command.
 */
public class VariableTableTest extends JDWPMethodVariableTableTestCase {


    /**
     * This testcase exercises Method.VariableTable command.
     * <BR>It runs MethodDebuggee, receives methods of debuggee.
     * For each received method sends Method.VariableTable command
     * and prints returned VariableTable.
     */
    public void testVariableTableTest001() {
        checkMethodVariableTable(false);
    }
}
