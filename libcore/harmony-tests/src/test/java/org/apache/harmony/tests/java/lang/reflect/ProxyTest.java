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

package org.apache.harmony.tests.java.lang.reflect;

import tests.support.Support_Proxy_I1;
import tests.support.Support_Proxy_I2;
import tests.support.Support_Proxy_ParentException;
import tests.support.Support_Proxy_SubException;
import java.io.IOException;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.lang.reflect.UndeclaredThrowableException;
import java.security.AllPermission;
import java.security.ProtectionDomain;
import java.util.ArrayList;

public class ProxyTest extends junit.framework.TestCase {

    /*
     * When multiple interfaces define the same method, the list of thrown
     * exceptions are those which can be mapped to another exception in the
     * other method:
     *
     * String foo(String s) throws SubException, LinkageError;
     *
     * UndeclaredThrowableException wrappers any checked exception which is not
     * in the merged list. So ParentException would be wrapped, BUT LinkageError
     * would not be since its not an Error/RuntimeException.
     *
     * interface I1 { String foo(String s) throws ParentException, LinkageError; }
     * interface I2 { String foo(String s) throws SubException, Error; }
     */

    interface Broken1 {
        public float method(float _number0, float _number1);
    }

    class Broken1Invoke implements InvocationHandler {
        public Object invoke(Object proxy, Method method, Object[] args)
                throws Throwable {
            return args[1];
        }
    }

    class ProxyCoonstructorTest extends Proxy {
        protected ProxyCoonstructorTest(InvocationHandler h) {
            super(h);
        }
    }

    /**
     * java.lang.reflect.Proxy#getProxyClass(java.lang.ClassLoader,
     *        java.lang.Class[])
     */
    public void test_getProxyClassLjava_lang_ClassLoader$Ljava_lang_Class() {
        Class proxy = Proxy.getProxyClass(Support_Proxy_I1.class
                .getClassLoader(), new Class[] { Support_Proxy_I1.class });

        assertTrue("Did not create a Proxy subclass ",
                proxy.getSuperclass() == Proxy.class);
        assertTrue("Does not believe its a Proxy class ", Proxy
                .isProxyClass(proxy));

        assertTrue("Does not believe it's a Proxy class ", Proxy
                .isProxyClass(Proxy.getProxyClass(null,
                        new Class[] { Comparable.class })));

        try {
            Proxy.getProxyClass(Support_Proxy_I1.class.getClassLoader(),
                    (Class<?>[]) null);
            fail("NPE expected");
        } catch (NullPointerException expected) {
        }

        try {
            Proxy.getProxyClass(Support_Proxy_I1.class.getClassLoader(),
                    new Class<?>[] {Support_Proxy_I1.class, null});
            fail("NPE expected");
        } catch (NullPointerException expected) {
        }
    }

    /**
     * java.lang.reflect.Proxy#Proxy(java.lang.reflect.InvocationHandler)
     */
    public void test_ProxyLjava_lang_reflect_InvocationHandler() {
        assertNotNull(new ProxyCoonstructorTest(new InvocationHandler() {
            public Object invoke(Object proxy, Method method, Object[] args)
                    throws Throwable {
                return null;
            }
        }));
    }



    /**
     * java.lang.reflect.Proxy#newProxyInstance(java.lang.ClassLoader,
     *        java.lang.Class[], java.lang.reflect.InvocationHandler)
     */
    public void test_newProxyInstanceLjava_lang_ClassLoader$Ljava_lang_ClassLjava_lang_reflect_InvocationHandler()
            throws Exception {
        Object p = Proxy.newProxyInstance(Support_Proxy_I1.class
                .getClassLoader(), new Class[] { Support_Proxy_I1.class,
                Support_Proxy_I2.class }, new InvocationHandler() {
            public Object invoke(Object proxy, Method method, Object[] args)
                    throws Throwable {
                if (method.getName().equals("equals"))
                    return new Boolean(proxy == args[0]);
                if (method.getName().equals("array"))
                    return new int[] { (int) ((long[]) args[0])[1], -1 };
                if (method.getName().equals("string")) {
                    if ("".equals(args[0]))
                        throw new Support_Proxy_SubException();
                    if ("clone".equals(args[0]))
                        throw new Support_Proxy_ParentException();
                    if ("error".equals(args[0]))
                        throw new ArrayStoreException();
                    if ("any".equals(args[0]))
                        throw new IllegalAccessException();
                }
                return null;
            }
        });

        Support_Proxy_I1 proxy = (Support_Proxy_I1) p;
        assertTrue("Failed identity test ", proxy.equals(proxy));
        assertTrue("Failed not equals test ", !proxy.equals(""));
        int[] result = (int[]) proxy.array(new long[] { 100L, -200L });
        assertEquals("Failed primitive type conversion test ", -200, result[0]);

        try {
            proxy.string("");
            fail("Problem converting exception");
        } catch (Support_Proxy_SubException e) {
        }

        try {
            proxy.string("clone");
            fail("Problem converting exception");
        } catch (UndeclaredThrowableException e) {
            assertSame(Support_Proxy_ParentException.class, e.getCause().getClass());
        }

        try {
            proxy.string("error");
            fail("Problem converting exception");
        } catch (ArrayStoreException e) {
        }

        try {
            proxy.string("any");
            fail("Problem converting exception");
        } catch (UndeclaredThrowableException e) {
            assertSame(IllegalAccessException.class, e.getCause().getClass());
        }

        Broken1 proxyObject = (Broken1) Proxy.newProxyInstance(Broken1.class
                .getClassLoader(), new Class[] { Broken1.class },
                new Broken1Invoke());

        float brokenResult = proxyObject.method(2.1f, 5.8f);
        assertTrue("Invalid invoke result", brokenResult == 5.8f);
    }

    /**
     * java.lang.reflect.Proxy#isProxyClass(java.lang.Class)
     */
    public void test_isProxyClassLjava_lang_Class() {
        Class proxy = Proxy.getProxyClass(Support_Proxy_I1.class
                .getClassLoader(), new Class[] { Support_Proxy_I1.class });

        class Fake extends Proxy {
            Fake() {
                super(null);
            }
        }

        Proxy fake = new Proxy(new InvocationHandler() {
            public Object invoke(Object proxy, Method method, Object[] args)
                    throws Throwable {
                return null;
            }
        }) {
        };

        assertTrue("Does not believe its a Proxy class ", Proxy
                .isProxyClass(proxy));
        assertTrue("Proxy subclasses do not count ", !Proxy
                .isProxyClass(Fake.class));
        assertTrue("Is not a runtime generated Proxy class ", !Proxy
                .isProxyClass(fake.getClass()));
        try{
             Proxy.isProxyClass(null);
             fail("NPE was not thrown");
        } catch (NullPointerException expected){
        }
    }

    /**
     * java.lang.reflect.Proxy#getInvocationHandler(java.lang.Object)
     */
    public void test_getInvocationHandlerLjava_lang_Object() {
        InvocationHandler handler = new InvocationHandler() {
            public Object invoke(Object proxy, Method method, Object[] args)
                    throws Throwable {
                return null;
            }
        };

        Object p = Proxy.newProxyInstance(Support_Proxy_I1.class
                .getClassLoader(), new Class[] { Support_Proxy_I1.class },
                handler);

        assertTrue("Did not return invocation handler ", Proxy
                .getInvocationHandler(p) == handler);
        try {
            Proxy.getInvocationHandler("");
            fail("Did not detect non proxy object");
        } catch (IllegalArgumentException e) {
        }
    }

    //Regression Test for HARMONY-2355
    public void test_newProxyInstance_withCompatibleReturnTypes() {
        Object o = Proxy
                .newProxyInstance(this.getClass().getClassLoader(),
                        new Class[] { ITestReturnObject.class,
                                ITestReturnString.class },
                        new TestProxyHandler(new TestProxyImpl()));
        assertNotNull(o);
    }

    public void test_newProxyInstance_withNonCompatibleReturnTypes() {
        try {
            Proxy.newProxyInstance(this.getClass().getClassLoader(),
                    new Class[] { ITestReturnInteger.class,
                            ITestReturnString.class }, new TestProxyHandler(
                            new TestProxyImpl()));
            fail("should throw IllegalArgumentException");
        } catch (IllegalArgumentException e) {
            // expected
        }

    }

    @SuppressWarnings("unchecked")
    public void test_ProxyClass_withParentAndSubInThrowList() throws SecurityException, NoSuchMethodException {
        TestParentIntf myImpl = new MyImplWithParentAndSubInThrowList();
        Class<?> c = Proxy.getProxyClass(myImpl.getClass().getClassLoader(), myImpl.getClass().getInterfaces());
        Method m = c.getMethod("test", (Class<?>[]) null);
        Class<?>[] exceptions = m.getExceptionTypes();
        ArrayList<Class> exps = new ArrayList<Class>();
        for (Class<?> exp : exceptions) {
            exps.add(exp);
        }
        assertTrue(exps.contains(Exception.class));
    }

    public static interface ITestReturnObject {
        Object f();
    }

    public static interface ITestReturnString {
        String f();
    }

    public static interface ITestReturnInteger {
        Integer f();
    }

    public static class TestProxyImpl implements ITestReturnObject,
            ITestReturnString {
        public String f() {
            // do nothing
            return null;
        }
    }

    public static class TestProxyHandler implements InvocationHandler {
        private Object proxied;

        public TestProxyHandler(Object object) {
            proxied = object;
        }

        public Object invoke(Object object, Method method, Object[] args)
                throws Throwable {
            // do nothing
            return method.invoke(proxied, args);
        }

    }

    class MyImplWithParentAndSubInThrowList implements TestSubIntf {
        public void test() throws Exception {
            throw new Exception();
        }
    }


    protected void setUp() {
    }

    protected void tearDown() {
    }
}

interface PkgIntf {
}

interface TestParentIntf {
    //IOException is a subclass of Exception
    public void test() throws IOException, Exception;
}

interface TestSubIntf extends TestParentIntf {
}
