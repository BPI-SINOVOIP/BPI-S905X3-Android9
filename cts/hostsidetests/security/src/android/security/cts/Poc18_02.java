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
public class Poc18_02 extends SecurityTestCase {

    /**
     * b/68953950
     */
     @SecurityTest
     public void testPocCVE_2017_13232() throws Exception {
       AdbUtils.runCommandLine("logcat -c" , getDevice());
       AdbUtils.runPocNoOutput("CVE-2017-13232", getDevice(), 60);
       String logcatOutput = AdbUtils.runCommandLine("logcat -d", getDevice());
       assertNotMatchesMultiLine(".*APM_AudioPolicyManager: getOutputForAttr\\(\\) "+
                                 "invalid attributes: usage=.{1,} content=.{1,} "+
                                 "flags=.{1,} tags=\\[.{256,}\\].*", logcatOutput);
     }
}
