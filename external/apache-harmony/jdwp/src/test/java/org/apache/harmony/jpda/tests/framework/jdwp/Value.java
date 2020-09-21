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
 * @author Aleksey V. Yantsen
 */

/**
 * Created on 10.25.2004
 */
package org.apache.harmony.jpda.tests.framework.jdwp;

import org.apache.harmony.jpda.tests.framework.TestErrorException;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;

/**
 * This class represents generic value used in JDWP packets.
 */
public class Value {

    /**
     * Creates new boolean value.
     */
    public static Value createBoolean(boolean value) {
        return new Value(value);
    }

    /**
     * Creates new byte value.
     */
    public static Value createByte(byte value) {
        return new Value(JDWPConstants.Tag.BYTE_TAG, Byte.valueOf(value));
    }

    /**
     * Creates new char value.
     */
    public static Value createChar(char value) {
        return new Value(value);
    }

    /**
     * Creates new short value.
     */
    public static Value createShort(short value) {
        return new Value(JDWPConstants.Tag.SHORT_TAG, Short.valueOf(value));
    }

    /**
     * Creates new int value.
     */
    public static Value createInt(int value) {
        return new Value(JDWPConstants.Tag.INT_TAG, Integer.valueOf(value));
    }

    /**
     * Creates new long value.
     */
    public static Value createLong(long value) {
        return new Value(JDWPConstants.Tag.LONG_TAG, Long.valueOf(value));
    }

    /**
     * Creates new float value.
     */
    public static Value createFloat(float value) {
        return new Value(JDWPConstants.Tag.FLOAT_TAG, Float.valueOf(value));
    }

    /**
     * Creates new double value.
     */
    public static Value createDouble(double value) {
        return new Value(JDWPConstants.Tag.DOUBLE_TAG, Double.valueOf(value));
    }

    /**
     * Creates void value.
     */
    public static Value createVoidValue() {
        return new Value(JDWPConstants.Tag.VOID_TAG, Long.valueOf(0));
    }

    /**
     * Creates object value.
     */
    public static Value createObjectValue(byte tag, long value) {
        if (isPrimitiveTag(tag)) {
            throw new AssertionError(JDWPConstants.Tag.getName(tag) + " is primitive");
        }
        return new Value(tag, Long.valueOf(value));
    }

    private final byte tag;

    private final Number numberValue;

    private final boolean booleanValue;

    private final char charValue;

    /**
     * Creates new value.
     */
    private Value(byte tag, Number numberValue) {
        this.tag = tag;
        this.numberValue = numberValue;
        this.booleanValue = false;
        this.charValue = 0;
    }

    /**
     * Creates new boolean value.
     */
    private Value(boolean value) {
        this.tag = JDWPConstants.Tag.BOOLEAN_TAG;
        this.booleanValue = value;
        this.numberValue = null;
        this.charValue = 0;
    }

    /**
     * Creates new char value.
     */
    private Value(char value) {
        this.tag = JDWPConstants.Tag.CHAR_TAG;
        this.charValue = value;
        this.numberValue = null;
        this.booleanValue = false;
    }

    /**
     * Returns tag of this value.
     *
     * @return Returns the tag.
     */
    public byte getTag() {
        return tag;
    }

    /**
     * Returns byte representation of this value.
     *
     * @return byte value
     */
    public byte getByteValue() {
        return numberValue.byteValue();
    }

    /**
     * Returns short representation of this value.
     *
     * @return short value
     */
    public short getShortValue() {
        return numberValue.shortValue();
    }

    /**
     * Returns int representation of this value.
     *
     * @return int value
     */
    public int getIntValue() {
        return numberValue.intValue();
    }

    /**
     * Returns long representation of this value.
     *
     * @return long value
     */
    public long getLongValue() {
        return numberValue.longValue();
    }

    /**
     * Returns float representation of this value.
     *
     * @return float value
     */
    public float getFloatValue() {
        return numberValue.floatValue();
    }

    /**
     * Returns double representation of this value.
     *
     * @return double value
     */
    public double getDoubleValue() {
        return numberValue.doubleValue();
    }

    /**
     * Returns boolean representation of this value.
     *
     * @return boolean value
     */
    public boolean getBooleanValue() {
        return booleanValue;
    }

    /**
     * Returns char representation of this value.
     *
     * @return char value
     */
    public char getCharValue() {
        return charValue;
    }

    private static boolean isPrimitiveTag(byte tag) {
        switch (tag) {
            case JDWPConstants.Tag.BOOLEAN_TAG:
            case JDWPConstants.Tag.BYTE_TAG:
            case JDWPConstants.Tag.CHAR_TAG:
            case JDWPConstants.Tag.SHORT_TAG:
            case JDWPConstants.Tag.INT_TAG:
            case JDWPConstants.Tag.LONG_TAG:
            case JDWPConstants.Tag.FLOAT_TAG:
            case JDWPConstants.Tag.DOUBLE_TAG:
            case JDWPConstants.Tag.VOID_TAG:
                return true;
            case JDWPConstants.Tag.NO_TAG:
            case JDWPConstants.Tag.ARRAY_TAG:
            case JDWPConstants.Tag.CLASS_LOADER_TAG:
            case JDWPConstants.Tag.CLASS_OBJECT_TAG:
            case JDWPConstants.Tag.OBJECT_TAG:
            case JDWPConstants.Tag.STRING_TAG:
            case JDWPConstants.Tag.THREAD_TAG:
            case JDWPConstants.Tag.THREAD_GROUP_TAG:
                return false;
            default:
                throw new TestErrorException("Illegal tag value: " + tag);
        }
    }

    /**
     * Compares with other value.
     */
    @Override
    public boolean equals(Object arg0) {
        if (!(arg0 instanceof Value))
            return false;

        Value value0 = (Value) arg0;
        if (tag != value0.tag)
            return false;

        switch (tag) {
        case JDWPConstants.Tag.BOOLEAN_TAG:
            return getBooleanValue() == value0.getBooleanValue();
        case JDWPConstants.Tag.BYTE_TAG:
            return getByteValue() == value0.getByteValue();
        case JDWPConstants.Tag.CHAR_TAG:
            return getCharValue() == value0.getCharValue();
        case JDWPConstants.Tag.DOUBLE_TAG:
            if (Double.isNaN(getDoubleValue())
                    && (Double.isNaN(value0.getDoubleValue())))
                return true;
            return getDoubleValue() == value0.getDoubleValue();
        case JDWPConstants.Tag.FLOAT_TAG:
            if (Float.isNaN(getFloatValue())
                    && (Float.isNaN(value0.getFloatValue())))
                return true;
            return getFloatValue() == value0.getFloatValue();
        case JDWPConstants.Tag.INT_TAG:
            return getIntValue() == value0.getIntValue();
        case JDWPConstants.Tag.LONG_TAG:
            return getLongValue() == value0.getLongValue();
        case JDWPConstants.Tag.SHORT_TAG:
            return getShortValue() == value0.getShortValue();
        case JDWPConstants.Tag.STRING_TAG:
        case JDWPConstants.Tag.ARRAY_TAG:
        case JDWPConstants.Tag.CLASS_LOADER_TAG:
        case JDWPConstants.Tag.CLASS_OBJECT_TAG:
        case JDWPConstants.Tag.OBJECT_TAG:
        case JDWPConstants.Tag.THREAD_GROUP_TAG:
        case JDWPConstants.Tag.THREAD_TAG:
            return getLongValue() == value0.getLongValue();
        }

        throw new TestErrorException("Illegal tag value");
    }

    /**
     * Converts this value to string representation for printing.
     */
    @Override
    public String toString() {

        switch (tag) {
        case JDWPConstants.Tag.BOOLEAN_TAG:
            return "boolean: " + getBooleanValue();
        case JDWPConstants.Tag.BYTE_TAG:
            return "byte: " + getByteValue();
        case JDWPConstants.Tag.CHAR_TAG:
            return "char: " + getCharValue();
        case JDWPConstants.Tag.DOUBLE_TAG:
            return "double: " + getDoubleValue();
        case JDWPConstants.Tag.FLOAT_TAG:
            return "float: " + getFloatValue();
        case JDWPConstants.Tag.INT_TAG:
            return "int: " + getIntValue();
        case JDWPConstants.Tag.LONG_TAG:
            return "long: " + getLongValue();
        case JDWPConstants.Tag.SHORT_TAG:
            return "short: " + getShortValue();
        case JDWPConstants.Tag.STRING_TAG:
            return "StringID: " + getLongValue();
        case JDWPConstants.Tag.ARRAY_TAG:
            return "ArrayID: " + getLongValue();
        case JDWPConstants.Tag.CLASS_LOADER_TAG:
            return "ClassLoaderID: " + getLongValue();
        case JDWPConstants.Tag.CLASS_OBJECT_TAG:
            return "ClassObjectID: " + getLongValue();
        case JDWPConstants.Tag.OBJECT_TAG:
            return "ObjectID: " + getLongValue();
        case JDWPConstants.Tag.THREAD_GROUP_TAG:
            return "ThreadGroupID: " + getLongValue();
        case JDWPConstants.Tag.THREAD_TAG:
            return "ThreadID: " + getLongValue();
        }

        throw new TestErrorException("Illegal tag value: " + tag);
    }
}
