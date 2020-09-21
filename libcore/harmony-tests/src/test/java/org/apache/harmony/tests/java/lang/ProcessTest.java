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

package org.apache.harmony.tests.java.lang;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import libcore.io.Libcore;
import java.util.concurrent.TimeUnit;

public class ProcessTest extends junit.framework.TestCase {
  // Test that failures to exec don't leave zombies lying around.
  public void test_55017() throws Exception {
    ArrayList<Process> children = new ArrayList<Process>();
    for (int i = 0; i < 256; ++i) {
      try {
        children.add(Runtime.getRuntime().exec(new String[] { "/system/bin/does-not-exist" }, null, null));
        System.gc();
      } catch (IOException expected) {
      }
    }
    assertEquals(0, children.size());

    String pid = Integer.toString(Libcore.os.getpid());
    boolean onDevice = new File("/system/bin").exists();
    String[] psCommand = onDevice ? new String[] { "ps", "--ppid", pid }
                                  : new String[] { "ps", "S", "--ppid", pid };
    Process ps = Runtime.getRuntime().exec(psCommand, null, null);
    int zombieCount = 0;
    for (String line : readAndCloseStream(ps.getInputStream()).split("\n")) {
      if (line.contains(" Z ") || line.contains(" Z+ ")) {
        ++zombieCount;
      }
    }
    assertEquals(0, zombieCount);
  }

  public void test_getOutputStream() throws Exception {
    String[] commands = { "cat", "-"};
    Process p = Runtime.getRuntime().exec(commands, null, null);
    OutputStream os = p.getOutputStream();
    // send data, and check if it is echoed back correctly
    String str1 = "Some data for testing communication between processes\n";
    String str2 = "More data that serves the same purpose.\n";
    String str3 = "Here is some more data.\n";
    os.write(str1.getBytes());
    try {
      Thread.sleep(1000);
    } catch (InterruptedException e) {
      e.printStackTrace();
    }
    os.write(str2.getBytes());
    os.write(str3.getBytes());
    os.close();

    String received = readAndCloseStream(p.getInputStream());
    assertEquals(str1 + str2 + str3, received);

    String stderr = readAndCloseStream(p.getErrorStream());
    assertEquals("", stderr);

    p.waitFor();
    p.destroy();
  }

  public void test_getErrorStream() throws Exception {
    String[] commands = { "cat", "--no-such-option"};
    Process p = Runtime.getRuntime().exec(commands, null, null);

    p.getOutputStream().close();

    String received = readAndCloseStream(p.getInputStream());
    assertEquals("", received);

    String stderr = readAndCloseStream(p.getErrorStream());
    assertTrue(stderr, stderr.contains("no-such-option"));

    p.waitFor();
    p.destroy();
  }

  private String readAndCloseStream(InputStream is) throws IOException {
    StringBuffer result = new StringBuffer();
    while (true) {
      int c = is.read();
      if (c == -1) {
        break;
      }
      result.append((char) c);
    }
    is.close();
    return result.toString();
  }

  public void test_exitValue() throws Exception {
    String[] commands = { "sh", "-c", "exit 0" };
    Process process = Runtime.getRuntime().exec(commands, null, null);
    process.waitFor();
    assertEquals(0, process.exitValue());

    String[] commandsNonZeroExit = { "sh", "-c", "exit 34" };
    process = Runtime.getRuntime().exec(commandsNonZeroExit, null, null);
    process.waitFor();
    assertEquals(34, process.exitValue());

    String[] commandsSleep = { "sleep", "3000" };
    process = Runtime.getRuntime().exec(commandsSleep, null, null);
    process.destroy();
    process.waitFor(); // destroy is asynchronous.
    assertTrue(process.exitValue() != 0);

    process = Runtime.getRuntime().exec(new String[] { "sleep", "3000" }, null, null);
    try {
      process.exitValue();
      fail();
    } catch(IllegalThreadStateException expected) {
    }
  }

  public void test_destroy() throws Exception {
    String[] commands = { "sh", "-c", "exit 0"};
    Process process = Runtime.getRuntime().exec(commands, null, null);
    process.destroy();
    process.destroy();
    process.destroy();
  }

  public void test_destroyForcibly() throws Exception {
    String[] commands = { "sh", "-c", "sleep 3000"};
    Process process = Runtime.getRuntime().exec(commands, null, null);
    assertNotNull(process.destroyForcibly());
    process.waitFor(); // destroy is asynchronous.
    assertTrue(process.exitValue() != 0);
  }

  public void test_isAlive() throws Exception {
    String[] commands = { "sh", "-c", "sleep 3000"};
    Process process = Runtime.getRuntime().exec(commands, null, null);
    assertTrue(process.isAlive());
    assertNotNull(process.destroyForcibly());
    process.waitFor(); // destroy is asynchronous.
    assertFalse(process.isAlive());
  }

  public void test_waitForTimeout() throws Exception {
    String[] commands = { "sh", "-c", "sleep 3000"};
    Process process = Runtime.getRuntime().exec(commands, null, null);
    assertFalse(process.waitFor(0, TimeUnit.MICROSECONDS));
    assertTrue(process.isAlive());
    assertFalse(process.waitFor(500, TimeUnit.MICROSECONDS));
    assertTrue(process.isAlive());
    assertNotNull(process.destroyForcibly());
    assertTrue(process.waitFor(2, TimeUnit.SECONDS));
    assertFalse(process.isAlive());
  }

  public void test_waitForTimeout_NPE() throws Exception {
    String[] commands = { "sh", "-c", "sleep 3000"};
    Process process = Runtime.getRuntime().exec(commands, null, null);
    try {
      process.waitFor(500, null);
      fail();
    } catch(NullPointerException expected) {}
    assertNotNull(process.destroyForcibly());
    process.waitFor();
  }

}
