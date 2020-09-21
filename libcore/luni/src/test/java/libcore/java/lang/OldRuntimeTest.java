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

package libcore.java.lang;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.Permission;
import java.util.Arrays;
import java.util.Vector;
import tests.support.resource.Support_Resources;
import dalvik.system.VMRuntime;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;

public class OldRuntimeTest extends junit.framework.TestCase {

    Runtime r = Runtime.getRuntime();

    InputStream is;

    public void test_getRuntime() {
        // Test for method java.lang.Runtime java.lang.Runtime.getRuntime()
        assertNotNull(Runtime.getRuntime());
    }

    public void test_addShutdownHook() {
        Thread thrException = new Thread () {
            public void run() {
                try {
                    Runtime.getRuntime().addShutdownHook(this);
                    fail("IllegalStateException was not thrown.");
                } catch(IllegalStateException ise) {
                    //expected
                }
            }
        };

        try {
            Runtime.getRuntime().addShutdownHook(thrException);
        } catch (Throwable t) {
            fail(t.getMessage());
        }

        try {
            Runtime.getRuntime().addShutdownHook(thrException);
            fail("IllegalArgumentException was not thrown.");
        } catch(IllegalArgumentException  iae) {
            // expected
        }

        SecurityManager sm = new SecurityManager() {

            public void checkPermission(Permission perm) {
                if (perm.getName().equals("shutdownHooks")) {
                    throw new SecurityException();
                }
            }
        };

        // remove previously added hook so we're not depending on the priority
        // of the Exceptions to be thrown.
        Runtime.getRuntime().removeShutdownHook(thrException);

        try {
            Thread.currentThread().sleep(1000);
        } catch (InterruptedException ie) {
        }
    }

    public void test_availableProcessors() {
        assertTrue(Runtime.getRuntime().availableProcessors() > 0);
    }


    public void test_execLjava_lang_StringLjava_lang_StringArray() {
        String [] envp =  getEnv();

        checkExec(0, envp, null);
        checkExec(0, null, null);

        try {
            Runtime.getRuntime().exec((String)null, null);
            fail("NullPointerException should be thrown.");
        } catch(IOException ioe) {
            fail("IOException was thrown.");
        } catch(NullPointerException npe) {
            //expected
        }

        SecurityManager sm = new SecurityManager() {

            public void checkPermission(Permission perm) {
                if (perm.getName().equals("checkExec")) {
                    throw new SecurityException();
                }
            }

            public void checkExec(String cmd) {
                throw new SecurityException();
            }
        };

        try {
            Runtime.getRuntime().exec("", envp);
            fail("IllegalArgumentException should be thrown.");
        } catch(IllegalArgumentException iae) {
            //expected
        } catch (IOException e) {
            fail("IOException was thrown.");
        }
    }

    public void test_execLjava_lang_StringArrayLjava_lang_StringArray() {
        String [] envp =  getEnv();

        checkExec(4, envp, null);
        checkExec(4, null, null);

        try {
            Runtime.getRuntime().exec((String[])null, null);
            fail("NullPointerException should be thrown.");
        } catch(IOException ioe) {
            fail("IOException was thrown.");
        } catch(NullPointerException npe) {
            //expected
        }

        try {
            Runtime.getRuntime().exec(new String[]{"ls", null}, null);
            fail("NullPointerException should be thrown.");
        } catch(IOException ioe) {
            fail("IOException was thrown.");
        } catch(NullPointerException npe) {
            //expected
        }

        SecurityManager sm = new SecurityManager() {

            public void checkPermission(Permission perm) {
                if (perm.getName().equals("checkExec")) {
                    throw new SecurityException();
                }
            }

            public void checkExec(String cmd) {
                throw new SecurityException();
            }
        };

        try {
            Runtime.getRuntime().exec(new String[]{}, envp);
            fail("IndexOutOfBoundsException should be thrown.");
        } catch(IndexOutOfBoundsException ioob) {
            //expected
        } catch (IOException e) {
            fail("IOException was thrown.");
        }

        try {
            Runtime.getRuntime().exec(new String[]{""}, envp);
            fail("IOException should be thrown.");
        } catch (IOException e) { /* expected */ }
    }

    public void test_execLjava_lang_StringLjava_lang_StringArrayLjava_io_File() {

        String [] envp =  getEnv();

        File workFolder = Support_Resources.createTempFolder();

        checkExec(2, envp, workFolder);
        checkExec(2, null, null);

        try {
            Runtime.getRuntime().exec((String)null, null, workFolder);
            fail("NullPointerException should be thrown.");
        } catch(IOException ioe) {
            fail("IOException was thrown.");
        } catch(NullPointerException npe) {
            //expected
        }

        SecurityManager sm = new SecurityManager() {

            public void checkPermission(Permission perm) {
                if (perm.getName().equals("checkExec")) {
                    throw new SecurityException();
                }
            }

            public void checkExec(String cmd) {
                throw new SecurityException();
            }
        };

        try {
            Runtime.getRuntime().exec("",  envp, workFolder);
            fail("SecurityException should be thrown.");
        } catch(IllegalArgumentException iae) {
            //expected
        } catch (IOException e) {
            fail("IOException was thrown.");
        }
    }

    public void test_execLjava_lang_StringArrayLjava_lang_StringArrayLjava_io_File() {
        String [] envp =  getEnv();

        File workFolder = Support_Resources.createTempFolder();

        checkExec(5, envp, workFolder);
        checkExec(5, null, null);

        try {
            Runtime.getRuntime().exec((String[])null, null, workFolder);
            fail("NullPointerException should be thrown.");
        } catch(IOException ioe) {
            fail("IOException was thrown.");
        } catch(NullPointerException npe) {
            //expected
        }

        try {
            Runtime.getRuntime().exec(new String[]{"ls", null}, null, workFolder);
            fail("NullPointerException should be thrown.");
        } catch(IOException ioe) {
            fail("IOException was thrown.");
        } catch(NullPointerException npe) {
            //expected
        }

        SecurityManager sm = new SecurityManager() {

            public void checkPermission(Permission perm) {
                if (perm.getName().equals("checkExec")) {
                    throw new SecurityException();
                }
            }

            public void checkExec(String cmd) {
                throw new SecurityException();
            }
        };

        try {
            Runtime.getRuntime().exec(new String[]{""}, envp, workFolder);
            fail("IOException should be thrown.");
        } catch (IOException e) {
            //expected
        }
    }

    String [] getEnv() {
        Object [] valueSet = System.getenv().values().toArray();
        Object [] keySet = System.getenv().keySet().toArray();
        String [] envp = new String[valueSet.length];
        for(int i = 0; i < envp.length; i++) {
            envp[i] = keySet[i] + "=" + valueSet[i];
        }
        return envp;
    }

    void checkExec(int testCase, String [] envp, File file) {
        String dirName = "Test_Directory";
        String dirParentName = "Parent_Directory";
        File resources = Support_Resources.createTempFolder();
        String folder = resources.getAbsolutePath() + "/" + dirName;
        String folderWithParent = resources.getAbsolutePath() + "/"  +
                                    dirParentName + "/" + dirName;
        String command = "mkdir " + folder;
        String [] commandArguments = {"mkdir", folder};
        try {
            Process proc = null;
            switch(testCase) {
                case 0:
                    proc = Runtime.getRuntime().exec(command, envp);
                    break;
                case 1:
                    proc = Runtime.getRuntime().exec(command);
                    break;
                case 2:
                    proc = Runtime.getRuntime().exec(command, envp, file);
                    break;
                case 3:
                    proc = Runtime.getRuntime().exec(commandArguments);
                    break;
                case 4:
                    proc = Runtime.getRuntime().exec(commandArguments, envp);
                    break;
                case 5:
                    proc = Runtime.getRuntime().exec(commandArguments, envp, file);
                    break;
            }
            assertNotNull(proc);
            try {
                Thread.sleep(3000);
            } catch(InterruptedException ie) {
                fail("InterruptedException was thrown.");
            }
            File f = new File(folder);
            assertTrue(f.exists());
            if(f.exists()) {
                f.delete();
            }
        } catch(IOException io) {
            fail("IOException was thrown.");
        }
    }

    public void test_execLjava_lang_String() {
        checkExec(1, null, null);

        try {
            Runtime.getRuntime().exec((String) null);
            fail("NullPointerException was not thrown.");
        } catch(NullPointerException npe) {
            //expected
        } catch (IOException e) {
            fail("IOException was thrown.");
        }

        try {
            Runtime.getRuntime().exec("");
            fail("IllegalArgumentException was not thrown.");
        } catch(IllegalArgumentException iae) {
            //expected
        } catch (IOException e) {
            fail("IOException was thrown.");
        }
    }

    public void test_execLjava_lang_StringArray() {

        checkExec(3, null, null);

        try {
            Runtime.getRuntime().exec((String[]) null);
            fail("NullPointerException was not thrown.");
        } catch(NullPointerException npe) {
            //expected
        } catch (IOException e) {
            fail("IOException was thrown.");
        }

        try {
            Runtime.getRuntime().exec(new String[]{"ls", null});
            fail("NullPointerException was not thrown.");
        } catch(NullPointerException npe) {
            //expected
        } catch (IOException e) {
            fail("IOException was thrown.");
        }

        try {
            Runtime.getRuntime().exec(new String[]{});
            fail("IndexOutOfBoundsException was not thrown.");
        } catch(IndexOutOfBoundsException iobe) {
            //expected
        } catch (IOException e) {
            fail("IOException was thrown.");
        }

        try {
            Runtime.getRuntime().exec(new String[]{""});
            fail("IOException should be thrown.");
        } catch (IOException e) {
            //expected
        }
    }

    public void test_runFinalizersOnExit() {
        Runtime.getRuntime().runFinalizersOnExit(true);
    }

    public void test_removeShutdownHookLjava_lang_Thread() {
        Thread thr1 = new Thread () {
            public void run() {
                try {
                    Runtime.getRuntime().addShutdownHook(this);
                } catch(IllegalStateException ise) {
                    fail("IllegalStateException shouldn't be thrown.");
                }
            }
        };

        try {
            Runtime.getRuntime().addShutdownHook(thr1);
            Runtime.getRuntime().removeShutdownHook(thr1);
        } catch (Throwable t) {
            fail(t.getMessage());
        }

        Thread thr2 = new Thread () {
            public void run() {
                try {
                    Runtime.getRuntime().removeShutdownHook(this);
                    fail("IllegalStateException wasn't thrown.");
                } catch(IllegalStateException ise) {
                    //expected
                }
            }
        };

        try {
            Runtime.getRuntime().addShutdownHook(thr2);
        } catch (Throwable t) {
            fail(t.getMessage());
        }

        try {
            Thread.currentThread().sleep(1000);
        } catch (InterruptedException ie) {
        }
    }

    public void test_traceInstructions() {
        Runtime.getRuntime().traceInstructions(false);
        Runtime.getRuntime().traceInstructions(true);
        Runtime.getRuntime().traceInstructions(false);
    }

    public void test_traceMethodCalls() {
        Runtime.getRuntime().traceMethodCalls(false);
        try {
            Runtime.getRuntime().traceMethodCalls(true);
            fail();
        } catch (UnsupportedOperationException expected) {}
    }

    @SuppressWarnings("deprecation")
    public void test_getLocalizedInputStream() throws Exception {
        String simpleString = "Heart \u2f3c";
        byte[] expected = simpleString.getBytes("UTF-8");
        byte[] returned = new byte[expected.length];

        ByteArrayInputStream bais = new ByteArrayInputStream(
                simpleString.getBytes("UTF-8"));

        InputStream lcIn =
                Runtime.getRuntime().getLocalizedInputStream(bais);
        lcIn.read(returned);

        assertTrue("wrong result for String: " + simpleString,
                Arrays.equals(expected, returned));
    }

    @SuppressWarnings("deprecation")
    public void test_getLocalizedOutputStream() throws IOException {
        String simpleString = "Heart \u2f3c";
        byte[] expected = simpleString.getBytes("UTF-8");

        ByteArrayOutputStream out = new ByteArrayOutputStream();

        OutputStream lcOut =
                Runtime.getRuntime().getLocalizedOutputStream(out);
        lcOut.write(simpleString.getBytes("UTF-8"));
        lcOut.flush();
        lcOut.close();

        byte[] returned = out.toByteArray();

        assertTrue("wrong result for String: " + returned.toString() +
                " expected string: " + expected.toString(),
                Arrays.equals(expected, returned));
    }


    public void test_load() {
        try {
            Runtime.getRuntime().load("nonExistentLibrary");
            fail("UnsatisfiedLinkError was not thrown.");
        } catch(UnsatisfiedLinkError ule) {
            //expected
        }

        try {
            Runtime.getRuntime().load(null);
            fail("NullPointerException was not thrown.");
        } catch(NullPointerException npe) {
            //expected
        }
    }

    public void test_loadLibrary() {
        try {
            Runtime.getRuntime().loadLibrary("nonExistentLibrary");
            fail("UnsatisfiedLinkError was not thrown.");
        } catch(UnsatisfiedLinkError ule) {
            //expected
        }

        try {
            Runtime.getRuntime().loadLibrary(null);
            fail("NullPointerException was not thrown.");
        } catch(NullPointerException npe) {
            //expected
        }
    }

    // b/25859957
    public void test_loadDeprecated() throws Exception {
        final int savedTargetSdkVersion = VMRuntime.getRuntime().getTargetSdkVersion();
        try {
            try {
                // Call Runtime#load(String, ClassLoader) at API level 24 (N). It will fail
                // with a UnsatisfiedLinkError because requested library doesn't exits.
                VMRuntime.getRuntime().setTargetSdkVersion(24);
                Method loadMethod =
                        Runtime.class.getDeclaredMethod("load", String.class, ClassLoader.class);
                loadMethod.setAccessible(true);
                loadMethod.invoke(Runtime.getRuntime(), "nonExistentLibrary", null);
                fail();
            } catch(InvocationTargetException expected) {
                assertTrue(expected.getCause() instanceof UnsatisfiedLinkError);
            }

            try {
                // Call Runtime#load(String, ClassLoader) at API level 25. It will fail
                // with a IllegalStateException because it's deprecated.
                VMRuntime.getRuntime().setTargetSdkVersion(25);
                Method loadMethod =
                        Runtime.class.getDeclaredMethod("load", String.class, ClassLoader.class);
                loadMethod.setAccessible(true);
                loadMethod.invoke(Runtime.getRuntime(), "nonExistentLibrary", null);
                fail();
            } catch(InvocationTargetException expected) {
                assertTrue(expected.getCause() instanceof UnsupportedOperationException);
            }
        } finally {
            VMRuntime.getRuntime().setTargetSdkVersion(savedTargetSdkVersion);
        }
    }

    // b/25859957
    public void test_loadLibraryDeprecated() throws Exception {
        final int savedTargetSdkVersion = VMRuntime.getRuntime().getTargetSdkVersion();
        try {
            try {
                // Call Runtime#loadLibrary(String, ClassLoader) at API level 24 (N). It will fail
                // with a UnsatisfiedLinkError because requested library doesn't exits.
                VMRuntime.getRuntime().setTargetSdkVersion(24);
                Method loadMethod =
                        Runtime.class.getDeclaredMethod("loadLibrary", String.class, ClassLoader.class);
                loadMethod.setAccessible(true);
                loadMethod.invoke(Runtime.getRuntime(), "nonExistentLibrary", null);
                fail();
            } catch(InvocationTargetException expected) {
                assertTrue(expected.getCause() instanceof UnsatisfiedLinkError);
            }

            try {
                // Call Runtime#load(String, ClassLoader) at API level 25. It will fail
                // with a IllegalStateException because it's deprecated.

                VMRuntime.getRuntime().setTargetSdkVersion(25);
                Method loadMethod =
                        Runtime.class.getDeclaredMethod("loadLibrary", String.class, ClassLoader.class);
                loadMethod.setAccessible(true);
                loadMethod.invoke(Runtime.getRuntime(), "nonExistentLibrary", null);
                fail();
            } catch(InvocationTargetException expected) {
                assertTrue(expected.getCause() instanceof UnsupportedOperationException);
            }
        } finally {
            VMRuntime.getRuntime().setTargetSdkVersion(savedTargetSdkVersion);
        }
    }
}
