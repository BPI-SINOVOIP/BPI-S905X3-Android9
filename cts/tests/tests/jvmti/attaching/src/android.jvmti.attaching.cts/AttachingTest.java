/*
 * Copyright (C) 2017 The Android Open Source Project
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

package android.jvmti.attaching.cts;

import static org.junit.Assert.assertTrue;

import android.os.Debug;
import android.support.test.runner.AndroidJUnit4;

import dalvik.system.BaseDexClassLoader;

import org.junit.AfterClass;
import org.junit.FixMethodOrder;
import org.junit.Test;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;
import org.junit.runner.RunWith;
import org.junit.runners.MethodSorters;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.Callable;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

@RunWith(Parameterized.class)
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
public class AttachingTest {
    // Some static stored state, for final cleanup.
    private static Set<File> createdFiles = new HashSet<>();

    // Parameters for a test instance.

    // The string to pass as the agent parameter.
    private String agentString;
    // The classloader to pass.
    private ClassLoader classLoader;
    // The attach-success function for the last test.
    private Callable<Boolean> isAttachedFn;

    public AttachingTest(String agentString, ClassLoader classLoader,
            Callable<Boolean> isAttachedFn) {
        this.agentString = agentString;
        this.classLoader = classLoader;
        this.isAttachedFn = isAttachedFn;
    }

    @Parameters
    public static Collection<Object[]> data() {
        Collection<Object[]> params = new ArrayList<>();

        try {
            // Test that an absolute path works w/o given classloader.
            File agentExtracted = copyAgentToFile("jvmtiattachingtestagent1");
            Callable<Boolean> success = AttachingTest::isAttached1;
            params.add(new Object[] {
                agentExtracted.getAbsolutePath(),
                null,
                success,
            });
            createdFiles.add(agentExtracted);
        } catch (Exception exc) {
            throw new RuntimeException(exc);
        }

        try {
            // Test that an absolute path works w/ given classloader.
            File agentExtracted = copyAgentToFile("jvmtiattachingtestagent2");
            Callable<Boolean> success = AttachingTest::isAttached2;
            params.add(new Object[] {
                agentExtracted.getAbsolutePath(),
                AttachingTest.class.getClassLoader(),
                success,
            });
            createdFiles.add(agentExtracted);
        } catch (Exception exc) {
            throw new RuntimeException(exc);
        }

        {
            // Test that a relative path works w/ given classloader.
            Callable<Boolean> success = AttachingTest::isAttached3;
            params.add(new Object[] {
                "libjvmtiattachingtestagent3.so",
                AttachingTest.class.getClassLoader(),
                success,
            });
        }

        try {
            // The name part of an extracted lib should not work.
            File agentExtracted = copyAgentToFile("jvmtiattachingtestagent4");
            String name = agentExtracted.getName();
            Callable<Boolean> success = () -> {
                try {
                    isAttached4();
                    // Any result is a failure.
                    return false;
                } catch (UnsatisfiedLinkError e) {
                    return true;
                }
            };
            params.add(new Object[] {
                name,
                AttachingTest.class.getClassLoader(),
                success,
            });
            createdFiles.add(agentExtracted);
        } catch (Exception exc) {
            throw new RuntimeException(exc);
        }

        return params;
    }

    private static File copyAgentToFile(String lib) throws Exception {
        ClassLoader cl = AttachingTest.class.getClassLoader();
        assertTrue(cl instanceof BaseDexClassLoader);

        File copiedAgent = File.createTempFile("agent", ".so");
        try (InputStream is = new FileInputStream(
                ((BaseDexClassLoader) cl).findLibrary(lib))) {
            try (OutputStream os = new FileOutputStream(copiedAgent)) {
                byte[] buffer = new byte[64 * 1024];

                while (true) {
                    int numRead = is.read(buffer);
                    if (numRead == -1) {
                        break;
                    }
                    os.write(buffer, 0, numRead);
                }
            }
        }

        return copiedAgent;
    }

    @AfterClass
    public static void cleanupExtractedAgents() throws Exception {
        for (File f : createdFiles) {
            f.delete();
        }
        createdFiles.clear();
    }

    // Tests.

    // This will be repeated unnecessarily, but that's OK.
    @Test(expected = IOException.class)
    public void a_attachInvalidAgent() throws Exception {
        File tmpFile = File.createTempFile("badAgent", ".so");
        createdFiles.add(tmpFile);
        Debug.attachJvmtiAgent(tmpFile.getAbsolutePath(), null, classLoader);
    }

    @Test(expected = IOException.class)
    public void a_attachInvalidPath() throws Exception {
        Debug.attachJvmtiAgent(agentString + ".invalid", null, classLoader);
    }

    @Test(expected = NullPointerException.class)
    public void a_attachNullAgent() throws Exception {
        Debug.attachJvmtiAgent(null, null, classLoader);
    }

    // This will be repeated unnecessarily, but that's OK.
    @Test(expected = IllegalArgumentException.class)
    public void a_attachWithEquals() throws Exception {
        File tmpFile = File.createTempFile("=", ".so");
        createdFiles.add(tmpFile);
        Debug.attachJvmtiAgent(tmpFile.getAbsolutePath(), null, classLoader);
    }

    @Test(expected = IOException.class)
    public void a_attachWithNullOptions() throws Exception {
        Debug.attachJvmtiAgent(agentString, null, classLoader);
    }

    @Test(expected = IOException.class)
    public void a_attachWithBadOptions() throws Exception {
        Debug.attachJvmtiAgent(agentString, "b", classLoader);
    }

    @Test
    public void b_attach() throws Exception {
        try {
            Debug.attachJvmtiAgent(agentString, "a", classLoader);
        } catch (Throwable t) {
            // Ignored.
        }

        assertTrue(isAttachedFn.call());
    }

    // Functions the agents can bind to.

    native static boolean isAttached1();
    native static boolean isAttached2();
    native static boolean isAttached3();
    native static boolean isAttached4();
}
