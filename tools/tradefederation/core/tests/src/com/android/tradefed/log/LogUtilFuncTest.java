/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.tradefed.log;

import com.android.ddmlib.Log;
import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.IGlobalConfiguration;
import com.android.tradefed.log.LogUtil.CLog;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * A set of functional testcases for {@link LogUtil}
 */
public class LogUtilFuncTest extends TestCase {
    private static final String CLASS_NAME = "LogUtilFuncTest";
    private static final String STRING = "hallo!";
    private ITerribleFailureHandler mMockWtfHandler;
    private IGlobalConfiguration mMockGlobalConfig;

    @Override
    protected void setUp() throws Exception, ConfigurationException {
        mMockWtfHandler = EasyMock.createMock(ITerribleFailureHandler.class);
        mMockGlobalConfig = EasyMock.createMock(IGlobalConfiguration.class);
    }

    public void testCLog_v() {
        Log.v(CLASS_NAME, "this is the real Log.v");
        CLog.v("this is CLog.v");
        CLog.v("this is CLog.v with a format string: %s has length %d", STRING, STRING.length());
    }

    public void testCLog_d() {
        Log.d(CLASS_NAME, "this is the real Log.d");
        CLog.d("this is CLog.d");
        CLog.d("this is CLog.d with a format string: %s has length %d", STRING, STRING.length());
    }

    public void testCLog_i() {
        Log.i(CLASS_NAME, "this is the real Log.i");
        CLog.i("this is CLog.i");
        CLog.i("this is CLog.i with a format string: %s has length %d", STRING, STRING.length());
    }

    public void testCLog_w() {
        Log.w(CLASS_NAME, "this is the real Log.w");
        CLog.w("this is CLog.w");
        CLog.w("this is CLog.w with a format string: %s has length %d", STRING, STRING.length());
    }

    public void testCLog_e() {
        Log.e(CLASS_NAME, "this is the real Log.e");
        CLog.e("this is CLog.e");
        CLog.e("this is CLog.e with a format string: %s has length %d", STRING, STRING.length());
    }

    /**
     * Verify that all variants of calling CLog.wtf() results in a wtf handler being called
     */
    public void testCLog_wtf() {
        // force CLog.wtf to use mock version of Global Config instead,
        // and set getWtfHandler() to return a mock wtf handler
        CLog.setGlobalConfigInstance(mMockGlobalConfig);
        EasyMock.expect(mMockGlobalConfig.getWtfHandler()).andReturn(mMockWtfHandler).anyTimes();
        EasyMock.replay(mMockGlobalConfig);

        // expect onTerribleFailure to be called once per CLog.wtf() call
        EasyMock.expect(mMockWtfHandler.onTerribleFailure(EasyMock.<String> anyObject(),
                EasyMock.<Throwable> anyObject())).andReturn(true).times(4);
        EasyMock.replay(mMockWtfHandler);

        CLog.wtf("this is CLog.wtf");
        CLog.wtf(new Throwable("this is CLog.wtf as a throwable"));
        CLog.wtf("this is CLog.wtf with a format string: %s has length %d",
                STRING, STRING.length());
        CLog.wtf("this is CLog.wtf with a throwable", new Throwable("this is my throwable"));
        EasyMock.verify(mMockWtfHandler);
    }

    /**
     * Verify scenario where no wtf handler has been configured when CLog.wtf() is called
     */
    public void testCLog_wtf_wtfHandlerNotSet() {
        // force CLog.wtf to use mock version of Global Config instead,
        // and set getWtfHandler() to return null (simulating no wtf handler being set)
        CLog.setGlobalConfigInstance(mMockGlobalConfig);
        EasyMock.expect(mMockGlobalConfig.getWtfHandler()).andReturn(null);
        EasyMock.replay(mMockGlobalConfig);

        // by not setting any expect on mMockWtfHandler,
        // EasyMock will verify that 0 calls are made to wtf handler
        EasyMock.replay(mMockWtfHandler);

        CLog.wtf("this is CLog.wtf without any handler set");
        EasyMock.verify(mMockWtfHandler);
    }

    /**
     * Verify that getClassName can get the desired class name from the stack trace.
     */
    public void testCLog_getClassName() {
        String klass = CLog.getClassName(1);
        assertTrue(CLASS_NAME.equals(klass));
    }

    /**
     * Verify that findCallerClassName is able to find this class's name from the stack trace.
     */
    public void testCLog_findCallerClassName() {
        String klass = CLog.findCallerClassName();
        assertEquals(CLASS_NAME, klass);
    }

    /**
     * Verify that findCallerClassName() is able to find the calling class even if it is
     * deeper in the stack trace.
     */
    public void testCLog_findCallerClassName_callerDeeperInStackTrace() {
        Throwable t = new Throwable();

        // take a real stack trace, but prepend it with some fake frames
        List<StackTraceElement> list = new ArrayList<StackTraceElement>(
            Arrays.asList(t.getStackTrace()));
        for (int i = 0; i < 5; i++) {
            list.add(0, new StackTraceElement(
                    CLog.class.getName(), "fakeMethod" + i, "fakefile", 1));
        }
        t.setStackTrace(list.toArray(new StackTraceElement[list.size()]));

        String klass = CLog.findCallerClassName(t);
        assertEquals(CLASS_NAME, klass);
    }

    /**
     * Verify that findCallerClassName() returns "Unknown" when there's an empty stack trace
     */
    public void testCLog_findCallerClassName_emptyStackTrace() {
        Throwable t = new Throwable();

        StackTraceElement[] emptyStackTrace = new StackTraceElement[0];
        t.setStackTrace(emptyStackTrace);

        String klass = CLog.findCallerClassName(t);
        assertEquals("Unknown", klass);
    }

    /**
     * Verify that parseClassName() is able to parse the class name out of different formats of
     * stack trace frames
     */
    public void testCLog_parseClassName() {
        assertEquals("OuterClass", CLog.parseClassName(
                "com.android.tradefed.log.OuterClass$InnerClass"));
        assertEquals("OuterClass", CLog.parseClassName("com.android.tradefed.log.OuterClass"));
        assertEquals("SimpleClassNameOnly", CLog.parseClassName("SimpleClassNameOnly"));
    }

    public void testLogAndDisplay_specialSerial() {
        CLog.logAndDisplay(LogLevel.VERBOSE, "[fe80::ba27:ebff:feb3:e8%em1]:5555");
    }
}
