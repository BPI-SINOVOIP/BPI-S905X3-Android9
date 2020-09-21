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
 * Created on 10.02.2005
 */
package org.apache.harmony.jpda.tests.jdwp.Method;

import java.util.List;
import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

@SuppressWarnings("unused")
public class MethodVariableTestDebuggee extends SyncDebuggee {
    // Static methods.
    public static void static_NoParam() {
    }

    public static void static_BooleanParam(boolean booleanParam) {
    }

    public static void static_ByteParam(byte byteParam) {
    }

    public static void static_CharParam(char charParam) {
    }

    public static void static_ShortParam(short shortParam) {
    }

    public static void static_IntParam(int intParam) {
    }

    public static void static_FloatParam(float floatParam) {
    }

    public static void static_LongParam(long longParam) {
    }

    public static void static_DoubleParam(double doubleParam) {
    }

    public static void static_ObjectParam(Object objectParam) {
    }

    public static void static_StringParam(String stringParam) {
    }

    public static void static_ListOfStringsParam(List<String> listOfStringsParam) {
    }

    public static <T extends CharSequence> void static_GenericParam(T genericParam) {
    }

    public static void static_IntArrayParam(int[] intArrayParam) {
    }

    public static void static_StringMultiArrayParam(String[][] stringMultiArrayParam) {
    }

    public static void static_IntLongParam(int intParam, long longParam) {
    }

    public static void static_LongIntParam(long longParam, int intParam) {
    }

    public static void static_NoParamWithLocal() {
      int i = 5;
      System.out.println(i);
      long l = 8;
      System.out.println(l);
    }

    // Instance methods.
    public void instance_NoParam() {
    }

    public void instance_BooleanParam(boolean booleanParam) {
    }

    public void instance_ByteParam(byte byteParam) {
    }

    public void instance_CharParam(char charParam) {
    }

    public void instance_ShortParam(short shortParam) {
    }

    public void instance_IntParam(int intParam) {
    }

    public void instance_FloatParam(float floatParam) {
    }

    public void instance_LongParam(long longParam) {
    }

    public void instance_DoubleParam(double doubleParam) {
    }

    public void instance_ObjectParam(Object objectParam) {
    }

    public void instance_StringParam(String stringParam) {
    }

    public void instance_ListOfStringsParam(List<String> listOfStringsParam) {
    }

    public <T extends CharSequence> void instance_GenericParam(T genericParam) {
    }

    public void instance_IntArrayParam(int[] intArrayParam) {
    }

    public void instance_StringMultiArrayParam(String[][] stringMultiArrayParam) {
    }

    public void instance_IntLongParam(int intParam, long longParam) {
    }

    public void instance_LongIntParam(long longParam, int intParam) {
    }

    public void instance_NoParamWithLocal() {
      int i = 5;
      System.out.println(i);
      long l = 8;
      System.out.println(l);
    }

    @Override
    public void run() {
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        logWriter.println("Hello World!");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    public static void main(String[] args) {
        runDebuggee(MethodVariableTestDebuggee.class);
    }

}