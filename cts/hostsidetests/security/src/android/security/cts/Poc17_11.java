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

@SecurityTest
public class Poc17_11 extends SecurityTestCase {

    /**
     * b/36075131
     */
    @SecurityTest
    public void testPocCVE_2017_0859() throws Exception {
        AdbUtils.runCommandLine("logcat -c", getDevice());
        AdbUtils.pushResource("/cve_2017_0859.mp4", "/sdcard/cve_2017_0859.mp4", getDevice());
        AdbUtils.runCommandLine("am start -a android.intent.action.VIEW " +
                                    "-d file:///sdcard/cve_2017_0859.mp4" +
                                    " -t audio/amr", getDevice());
        // Wait for intent to be processed before checking logcat
        Thread.sleep(5000);
        String logcat =  AdbUtils.runCommandLine("logcat -d", getDevice());
        assertNotMatches("[\\s\\n\\S]*Fatal signal 11 \\(SIGSEGV\\)" +
                         "[\\s\\n\\S]*>>> /system/bin/" +
                         "mediaserver <<<[\\s\\n\\S]*", logcat);
    }
}
