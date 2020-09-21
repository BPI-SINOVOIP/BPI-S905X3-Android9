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

import com.android.tradefed.log.LogUtil.CLog;

import java.util.ArrayList;
import java.util.List;

/**
 * Utility used to parse(USER,PID and NAME) from the "ps" command output
 */
public class PsParser {

    private static final String LINE_SEPARATOR = "\\n";
    private static final String PROCESS_INFO_SEPARATOR = "\\s+";
    private static final String BAD_PID = "bad pid '-A'";

    /**
     * Parse username, process id and process name from the ps command output and convert to list of
     * ProcessInfo objects.
     *
     * @param psOutput output of "ps" command.
     * @return list of processInfo
     */
    public static List<ProcessInfo> getProcesses(String psOutput) {

        List<ProcessInfo> processesInfo = new ArrayList<ProcessInfo>();
        if (psOutput.isEmpty()) {
            return processesInfo;
        }
        String processLines[] = psOutput.split(LINE_SEPARATOR);

        /*
         * (ps -A || ps) command prints "bad pid '-A'" as first line before the ps header in
         * N and older builds so skip the first two lines.
         */
        int startLineNum = 1;
        if (processLines[0].equals(BAD_PID)) {
            startLineNum = 2;
        }

        /*
         * Sample output:
         * USER PID PPID VSZ    RSS   WCHAN      PC  S NAME
         * root   1   0  11140  1848  epoll_wait  0  S init
         *
         * Not collecting information other than USER,PID and NAME because they are
         * not always printed for all the processess.
         */
        for (int lineCount = startLineNum; lineCount < processLines.length; lineCount++) {
            String processInfoStr[] = processLines[lineCount].split(PROCESS_INFO_SEPARATOR);
            CLog.i(processLines[lineCount]);
            ProcessInfo psInfo = new ProcessInfo(processInfoStr[0],
                    Integer.parseInt(processInfoStr[1]), processInfoStr[processInfoStr.length - 1]);
            processesInfo.add(psInfo);
        }
        return processesInfo;
    }

}

