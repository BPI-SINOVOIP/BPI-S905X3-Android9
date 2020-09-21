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
public class Poc17_04 extends SecurityTestCase {

  /**
   * b/32342065
   */
  @SecurityTest
  public void testPocCVE_2017_0553() throws Exception {
    // Error code of 139 represents segmentation fault
    getDevice().executeShellCommand("chmod +x /data/local/tmp/CVE-2017-0553");
    assertFalse("Segfault found",
        AdbUtils.runCommandGetExitCode("/data/local/tmp/CVE-2017-0553", getDevice())==139);
  }

  /**
   * b/72460737
   */
  @SecurityTest
  public void testPocCVE_2014_3145() throws Exception {
    assertFalse("VULNERABLE DEVICE DETECTED",
                AdbUtils.runPocCheckExitCode("CVE-2014-3145", getDevice(), 60));
  }
}
