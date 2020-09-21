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
 * Created on 11.02.2005
 */
package org.apache.harmony.jpda.tests.jdwp.ClassType;

import org.apache.harmony.jpda.tests.framework.jdwp.Field;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;


/**
 * Super class of some JDWP unit tests for JDWP ClassType command set.
 */
public class JDWPClassTypeTestCase extends JDWPSyncTestCase {

    /**
     * Returns full name of debuggee class which is used by
     * testcases in this test.
     * @return full name of debuggee class.
     */
    @Override
    protected String getDebuggeeClassName() {
        return "org.apache.harmony.jpda.tests.jdwp.ClassType.ClassTypeDebuggee";
    }

    /**
     * Returns signature of debuggee class which is used by
     * testcases in this test.
     * @return signature of debuggee class.
     */
    protected String getDebuggeeSignature() {
      return "Lorg/apache/harmony/jpda/tests/jdwp/ClassType/ClassTypeDebuggee;";
    }

    /**
     * Returns for specified class array with information about fields of this class.
     * <BR>Each element of array contains: 
     * <BR>Field ID, Field name, Field signature, Field modifier bit flags; 
     * @param refType - ReferenceTypeID, defining class.
     * @return array with information about fields.
     */
    protected Field[] jdwpGetFields(long refType) {
        return debuggeeWrapper.vmMirror.getFieldsInfo(refType);
    }

}