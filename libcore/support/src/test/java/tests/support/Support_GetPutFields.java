/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package tests.support;

import java.io.Serializable;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.IOException;

/**
 * Support class to test the methods of {@code ObjectInputStream.GetField} and
 * {@code ObjectOutputStream.PutField} to write fields to an output stream and
 * read them back from an input stream.
 */
public class Support_GetPutFields implements Serializable {

    private static final long serialVersionUID = 1L;

    public ObjectInputStream.GetField getField;
    public ObjectOutputStream.PutField putField;

    public boolean booleanValue = false;
    public byte byteValue = 0;
    public char charValue = 0;
    public double doubleValue = 0.0;
    public float floatValue = 0.0f;
    public long longValue = 0;
    public int intValue = 0;
    public short shortValue = 0;
    public SimpleClass objectValue = null;

    class SimpleClass implements Serializable {

        private static final long serialVersionUID = 1L;
        private int a;
        private String b;

        public SimpleClass(int aValue, String bValue) {
            a = aValue;
            b = bValue;
        }

        public int getA() {
            return a;
        }

        public String getB() {
            return b;
        }

        public boolean equals(Object obj) {
            if (obj == null || obj.getClass() != this.getClass()) {
                return false;
            }

            SimpleClass other = (SimpleClass) obj;
            return (a == other.getA() && b.equals(other.getB()));
        }
    }

    public void initTestValues() {
        booleanValue = true;
        byteValue = (byte) 0xbe;
        charValue = 'B';
        doubleValue = 424242.42;
        floatValue = 24.12f;
        longValue = 6700654321L;
        intValue = 77777;
        objectValue = new SimpleClass(1965, "Hello Mars");
        shortValue = 1234;
    }

    public boolean equals(Object obj) {
        if (obj == null || obj.getClass() != this.getClass()) {
            return false;
        }

        Support_GetPutFields other = (Support_GetPutFields) obj;
        return (booleanValue == other.booleanValue &&
                byteValue == other.byteValue &&
                charValue == other.charValue &&
                doubleValue == other.doubleValue &&
                floatValue == other.floatValue &&
                longValue == other.longValue &&
                intValue == other.intValue &&
                objectValue.equals(other.objectValue) &&
                shortValue == other.shortValue
                );
    }

    private void readObject(ObjectInputStream ois) throws Exception {
        getField = ois.readFields();
        booleanValue = getField.get("booleanValue", false);
        byteValue = getField.get("byteValue", (byte) 0);
        charValue = getField.get("charValue", (char) 0);
        doubleValue = getField.get("doubleValue", 0.0);
        floatValue = getField.get("floatValue", 0.0f);
        longValue = getField.get("longValue", (long) 0);
        intValue = getField.get("intValue", 0);
        objectValue = (Support_GetPutFields.SimpleClass)
                getField.get("objectValue", (Object) null);
        shortValue = getField.get("shortValue", (short) 0);
    }

    private void writeObject(ObjectOutputStream oos) throws IOException {
        putField = oos.putFields();
        putField.put("booleanValue", booleanValue);
        putField.put("byteValue", byteValue);
        putField.put("charValue", charValue);
        putField.put("doubleValue", doubleValue);
        putField.put("floatValue", floatValue);
        putField.put("longValue", longValue);
        putField.put("intValue", intValue);
        putField.put("objectValue", objectValue);
        putField.put("shortValue", shortValue);
        oos.writeFields();
    }
}
