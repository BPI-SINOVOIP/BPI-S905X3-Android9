/*
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

package com.android.tradefed.util;

import com.android.tradefed.util.ProcessInfo;

import java.util.List;

import junit.framework.TestCase;

/** Simple unit test for {@link PsParser}. */
public class PsParserTest extends TestCase {

    /**
     * Test behavior when there are no lines to parse.
     */
    public void testEmptyPsOutput() {
        List<ProcessInfo> psInfo = PsParser.getProcesses("");
        assertEquals("ProcessInfo list is not empty when ps output is empty", 0, psInfo.size());
    }

    /**
     * Test valid "ps -A || ps" output for builds after N.
     */
    public void testNewerValidPsOutput() {
        String psOutput = "USER       PID  PPID     VSZ    RSS WCHAN              PC S NAME\n"
                + "root         1     0   11136   1828 epoll_wait     4d8064 S init\n"
                + "root         2     0       0      0 kthreadd            0 S [kthreadd]\n";
        List<ProcessInfo> psInfo = PsParser.getProcesses(psOutput);
        assertEquals("PsParser did not return expected number of processes info ", 2,
                psInfo.size());
        assertEquals("First process user name did not match", "root", psInfo.get(0).getUser());
        assertEquals("First process id did not match", 1, psInfo.get(0).getPid());
        assertEquals("First process name did not match", "init", psInfo.get(0).getName());
        assertEquals("Second process user name did not match", "root", psInfo.get(1).getUser());
        assertEquals("Second process id did not match", 2, psInfo.get(1).getPid());
        assertEquals("Second process name did not match", "[kthreadd]", psInfo.get(1).getName());
    }

    /**
     * Test valid "ps -A || ps" output for N and older builds.
     */
    public void testOlderValidPsOutput() {
        String psOutput = "bad pid '-A'\n"
                + "USER       PID  PPID     VSZ    RSS WCHAN              PC S NAME\n"
                + "root         1     0   11136   1828 epoll_wait     4d8064 S init\n"
                + "root         2     0       0      0 kthreadd            0 S [kthreadd]\n";
        List<ProcessInfo> psInfo = PsParser.getProcesses(psOutput);
        assertEquals("PsParser did not return expected number of processes info ", 2,
                psInfo.size());
        assertEquals("First process user name did not match", "root", psInfo.get(0).getUser());
        assertEquals("First process id did not match", 1, psInfo.get(0).getPid());
        assertEquals("First process name did not match", "init", psInfo.get(0).getName());
        assertEquals("Second process user name did not match", "root", psInfo.get(1).getUser());
        assertEquals("Second process id did not match", 2, psInfo.get(1).getPid());
        assertEquals("Second process name did not match", "[kthreadd]", psInfo.get(1).getName());
    }

    /**
     * Test if "ps -A || ps" output has missing data and returns the expected ps info
     */
    public void testMissingInfoPsOutput() {
        String psMissingInfo = "bad pid '-A'\n"
                + "USER       PID  PPID     VSZ    RSS WCHAN              PC S NAME\n"
                + "root         1     0   11136   1828      4d8064 S init\n"
                + "root         2            0      0 kthreadd            0 S [kthreadd]\n";
        List<ProcessInfo> psInfo = PsParser.getProcesses(psMissingInfo);
        assertEquals("PsParser did not return expected number of processes info ", 2,
                psInfo.size());
        assertEquals("First process user name did not match", "root", psInfo.get(0).getUser());
        assertEquals("First process id did not match", 1, psInfo.get(0).getPid());
        assertEquals("First process name did not match", "init", psInfo.get(0).getName());
        assertEquals("Second process user name did not match", "root", psInfo.get(1).getUser());
        assertEquals("Second process id did not match", 2, psInfo.get(1).getPid());
        assertEquals("Second process name did not match", "[kthreadd]", psInfo.get(1).getName());
    }

}
