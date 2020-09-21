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

package org.apache.harmony.jpda.tests.framework.jdwp;

/**
 * This class provides description of class method.
 */
public class Method {
    private final long methodID;
    private final String name;
    private final String signature;
    private final String genericSignature;
    private final int modBits;

    public Method(long methodID, String name, String signature, String genericSignature,
            int modBits) {
        this.methodID = methodID;
        this.name = name;
        this.signature = signature;
        this.genericSignature = genericSignature;
        this.modBits = modBits;
    }

    /**
     * @return the method ID.
     */
    public long getMethodID() {
        return methodID;
    }

    /**
     * @return the modifier bits.
     */
    public int getModBits() {
        return modBits;
    }

    /**
     * @return the method name.
     */
    public String getName() {
        return name;
    }

    /**
     * @return the method signature.
     */
    public String getSignature() {
        return signature;
    }

    /**
     * @return the method generic signature (or an empty string if there is none).
     */
    public String getGenericSignature() {
        return genericSignature;
    }

    @Override
    public String toString() {
        return "" + methodID + " " + name + " " + signature + " " + modBits;
    }
}
