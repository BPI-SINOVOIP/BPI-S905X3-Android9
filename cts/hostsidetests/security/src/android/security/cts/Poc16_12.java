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
public class Poc16_12 extends SecurityTestCase {

    //Criticals
    /**
     *  b/31606947
     */
    @SecurityTest
    public void testPocCVE_2016_8424() throws Exception {
        if(containsDriver(getDevice(), "/dev/nvmap")) {
            AdbUtils.runPoc("CVE-2016-8424", getDevice(), 60);
        }
    }

    /**
     *  b/31797770
     */
    @SecurityTest
    public void testPocCVE_2016_8425() throws Exception {
        if(containsDriver(getDevice(), "/dev/nvhost-vic")) {
            AdbUtils.runPoc("CVE-2016-8425", getDevice(), 60);
        }
    }

    /**
     *  b/31799206
     */
    @SecurityTest
    public void testPocCVE_2016_8426() throws Exception {
        if(containsDriver(getDevice(), "/dev/nvhost-gpu")) {
            AdbUtils.runPoc("CVE-2016-8426", getDevice(), 60);
        }
    }

    /**
     *  b/31799885
     */
    @SecurityTest
    public void testPocCVE_2016_8427() throws Exception {
        if(containsDriver(getDevice(), "/dev/nvhost-gpu") ||
              containsDriver(getDevice(), "/dev/nvhost-dbg-gpu")) {
            AdbUtils.runPoc("CVE-2016-8427", getDevice(), 60);
        }
    }

    /**
     *  b/31993456
     */
    @SecurityTest
    public void testPocCVE_2016_8428() throws Exception {
        if(containsDriver(getDevice(), "/dev/nvmap")) {
            AdbUtils.runPoc("CVE-2016-8428", getDevice(), 60);
        }
    }

    /**
     *  b/32160775
     */
    @SecurityTest
    public void testPocCVE_2016_8429() throws Exception {
        if(containsDriver(getDevice(), "/dev/nvmap")) {
            AdbUtils.runPoc("CVE-2016-8429", getDevice(), 60);
        }
    }

    /**
     *  b/32225180
     */
    @SecurityTest
    public void testPocCVE_2016_8430() throws Exception {
        if(containsDriver(getDevice(), "/dev/nvhost-vic")) {
            AdbUtils.runPoc("CVE-2016-8430", getDevice(), 60);
        }
    }

   /**
     *  b/32402179
     */
    @SecurityTest
    public void testPocCVE_2016_8431() throws Exception {
        if(containsDriver(getDevice(), "/dev/dri/renderD129")) {
            AdbUtils.runPoc("CVE-2016-8431", getDevice(), 60);
        }
    }

    /**
     *  b/32447738
     */
    @SecurityTest
    public void testPocCVE_2016_8432() throws Exception {
        if(containsDriver(getDevice(), "/dev/dri/renderD129")) {
            AdbUtils.runPoc("CVE-2016-8432", getDevice(), 60);
        }
    }

    /**
     *  b/32125137
     */
    @SecurityTest
    public void testPocCVE_2016_8434() throws Exception {
        if(containsDriver(getDevice(), "/dev/kgsl-3d0")) {
            // This poc is very verbose so we ignore the output to avoid using a lot of memory.
            AdbUtils.runPocNoOutput("CVE-2016-8434", getDevice(), 60);
        }
    }

    /**
     *  b/31668540
     */
    @SecurityTest
    public void testPocCVE_2016_8460() throws Exception {
        if(containsDriver(getDevice(), "/dev/nvmap")) {
            String result = AdbUtils.runPoc("CVE-2016-8460", getDevice(), 60);
            assertTrue(!result.equals("Vulnerable"));
        }
    }

    /**
     *  b/32659848
     */
    @SecurityTest
    public void testPoc32659848() throws Exception {
        String command =
            "echo 18014398509481980 > /sys/kernel/debug/tracing/buffer_size_kb";
        AdbUtils.runCommandLine(command, getDevice());
    }
}
