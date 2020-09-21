/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */
package android.jvmti.cts;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;

import android.content.pm.PackageManager;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

/**
 * Check redefineClasses-related functionality.
 */
public class JvmtiRunTestBasedTest extends JvmtiTestBase {

    private PrintStream oldOut, oldErr;
    private ByteArrayOutputStream bufferedOut, bufferedErr;

    @Before
    public void setUp() throws Exception {
        oldOut = System.out;
        oldErr = System.err;

        System.setOut(new PrintStream(bufferedOut = new ByteArrayOutputStream(), true));
        System.setErr(new PrintStream(bufferedErr = new ByteArrayOutputStream(), true));
    }

    @After
    public void tearDown() {
        System.setOut(oldOut);
        System.setErr(oldErr);
    }

    protected int getTestNumber() throws Exception {
        return mActivity.getPackageManager().getApplicationInfo(mActivity.getPackageName(),
                PackageManager.GET_META_DATA).metaData.getInt("android.jvmti.cts.run_test_nr");
    }

    // Some tests are very sensitive to state of the thread they are running on. To support this we
    // can have tests run on newly created threads. This defaults to false.
    protected boolean needNewThread() throws Exception {
        return mActivity
            .getPackageManager()
            .getApplicationInfo(mActivity.getPackageName(), PackageManager.GET_META_DATA)
            .metaData
            .getBoolean("android.jvmti.cts.needs_new_thread", /*defaultValue*/false);
    }

    @Test
    public void testRunTest() throws Exception {
        final int nr = getTestNumber();

        // Load the test class.
        Class<?> testClass = Class.forName("art.Test" + nr);
        final Method runMethod = testClass.getDeclaredMethod("run");
        if (needNewThread()) {
          // Make sure the thread the test is running on has the right name. Some tests are
          // sensitive to this. Ideally we would also avoid having a try-catch too but that is more
          // trouble than it's worth.
          final Throwable[] final_throw = new Throwable[] { null };
          Thread main_thread = new Thread(
              () -> {
                try {
                  runMethod.invoke(null);
                } catch (IllegalArgumentException e) {
                  throw new Error("Exception thrown", e);
                } catch (InvocationTargetException e) {
                  throw new Error("Exception thrown", e);
                } catch (NullPointerException e) {
                  throw new Error("Exception thrown", e);
                } catch (IllegalAccessException e) {
                  throw new Error("Exception thrown", e);
                }
              }, "main");
          main_thread.setUncaughtExceptionHandler((thread, e) -> { final_throw[0] = e; });

          main_thread.start();
          main_thread.join();

          if (final_throw[0] != null) {
            throw new InvocationTargetException(final_throw[0], "Remote exception occurred.");
          }
        } else {
          runMethod.invoke(null);
        }

        // Load the expected txt file.
        InputStream expectedStream = getClass().getClassLoader()
                .getResourceAsStream("results." + nr + ".expected.txt");
        compare(expectedStream, bufferedOut);

        if (bufferedErr.size() > 0) {
            throw new IllegalStateException(
                    "Did not expect System.err output: " + bufferedErr.toString());
        }
    }

    // Very primitive diff. Doesn't do any smart things...
    private void compare(InputStream expectedStream, ByteArrayOutputStream resultStream)
            throws Exception {
        // This isn't really optimal in any way.
        BufferedReader expectedReader = new BufferedReader(new InputStreamReader(expectedStream));
        BufferedReader resultReader = new BufferedReader(
                new InputStreamReader(new ByteArrayInputStream(resultStream.toByteArray())));
        StringBuilder resultBuilder = new StringBuilder();
        boolean failed = false;
        for (;;) {
            String expString = expectedReader.readLine();
            String resString = resultReader.readLine();

            if (expString == null && resString == null) {
                // Done.
                break;
            }

            if (expString != null && resString != null && expString.equals(resString)) {
                resultBuilder.append("  ");
                resultBuilder.append(expString);
                resultBuilder.append('\n');
                continue;
            }

            failed = true;
            if (expString != null) {
                resultBuilder.append("- ");
                resultBuilder.append(expString);
                resultBuilder.append('\n');
            }
            if (resString != null) {
                resultBuilder.append("+ ");
                resultBuilder.append(resString);
                resultBuilder.append('\n');
            }
        }
        if (failed) {
            throw new IllegalStateException(resultBuilder.toString());
        }
    }
}
