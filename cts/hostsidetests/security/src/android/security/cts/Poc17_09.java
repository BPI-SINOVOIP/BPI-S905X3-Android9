/**
 * Copyright (C) 2016 The Android Open Source Project
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

package android.security.cts;

import android.platform.test.annotations.SecurityTest;

@SecurityTest
public class Poc17_09 extends SecurityTestCase {

    /**
     * b/63852675
     */
    @SecurityTest
    public void testPocCve_2017_6983() throws Exception {
      // Error code of 139 represents segmentation fault
      assertFalse("Segfault found",
        AdbUtils.runCommandGetExitCode("sqlite3 ':memory:' \"CREATE VIRTUAL TABLE a using fts3(b);"
                                                          + "INSERT INTO a values(x'efbeaddeefbeadde');"
                                                          + "SELECT optimize(b)  FROM a;\""
                                      , getDevice()
                                      )==139);
      assertFalse("Segfault found",
        AdbUtils.runCommandGetExitCode("sqlite3 ':memory:' \"CREATE VIRTUAL TABLE a using fts3(b);"
                                                          + "INSERT INTO a values(x'efbeaddeefbeadde');"
                                                          + "SELECT snippet(b)   FROM a;\""
                                      , getDevice()
                                      )==139);
      assertFalse("Segfault found",
        AdbUtils.runCommandGetExitCode("sqlite3 ':memory:' \"CREATE VIRTUAL TABLE a using fts3(b);"
                                                          + "INSERT INTO a values(x'efbeaddeefbeadde');"
                                                          + "SELECT offsets(b)   FROM a;\""
                                      , getDevice()
                                      )==139);
      assertFalse("Segfault found",
        AdbUtils.runCommandGetExitCode("sqlite3 ':memory:' \"CREATE VIRTUAL TABLE a using fts3(b);"
                                                          + "INSERT INTO a values(x'efbeaddeefbeadde');"
                                                          + "SELECT matchinfo(b) FROM a;\""
                                      , getDevice()
                                      )==139);
    }
 }
