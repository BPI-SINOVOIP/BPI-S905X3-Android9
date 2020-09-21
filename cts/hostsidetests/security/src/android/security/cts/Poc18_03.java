/**
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

package android.security.cts;

import android.platform.test.annotations.SecurityTest;

public class Poc18_03 extends SecurityTestCase {

  /**
   *  b/71389378
   */
  @SecurityTest
  public void testPocCVE_2017_13253() throws Exception {
    String output = AdbUtils.runPoc("CVE-2017-13253", getDevice());
    assertNotMatchesMultiLine(".*OVERFLOW DETECTED.*",output);
  }
}
