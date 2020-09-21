/* GENERATED SOURCE. DO NOT MODIFY. */
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
/*
 *******************************************************************************
 * Copyright (C) 2005-2016, International Business Machines Corporation and         *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package android.icu.dev.test.serializable;

import java.io.IOException;
import java.lang.reflect.Modifier;
import java.util.List;

import org.junit.Test;
import org.junit.runner.RunWith;

import android.icu.dev.test.TestFmwk;
import android.icu.dev.test.serializable.SerializableTestUtility.Handler;

import junitparams.JUnitParamsRunner;
import junitparams.Parameters;
import android.icu.testsharding.MainTestShard;

/**
 * @author sgill
 * @author emader
 *
 */
@MainTestShard
@RunWith(JUnitParamsRunner.class)
public class CoverageTest extends TestFmwk {

    @Test
    @Parameters(method="generateClassList")
    public void testSerialization(String className) throws ClassNotFoundException, IOException {
        Class c = Class.forName(className);
        int m = c.getModifiers();

        Handler classHandler = SerializableTestUtility.getHandler(className);
        if (classHandler == null) {
            if (!Modifier.isAbstract(m)) {
                errln("Missing test handler. Update the list of tests in SerializableTest.java to include a test case for " + className);
            }
            return;
        }
        Object[] testObjects = classHandler.getTestObjects();
        byte[] serializedBytes = SerializableTestUtility.getSerializedBytes(testObjects);
        Object[] serializedObjects = SerializableTestUtility.getSerializedObjects(serializedBytes);
        for (int i = 0; i < testObjects.length; i++) {
            if (!classHandler.hasSameBehavior(serializedObjects[i], testObjects[i])) {
                errln("Input object " + className + "(" + i + ") failed behavior test.");
            }
        }
    }

    List<String> generateClassList() throws IOException {
        return SerializableTestUtility.getSerializationClassList(this);
    }

}
